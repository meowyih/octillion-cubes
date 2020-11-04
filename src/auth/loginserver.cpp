#include <system_error>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <memory>
#include <chrono>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rand.h>

#include "jsonw/jsonw.hpp"

#include "auth/loginserver.hpp"
#include "auth/blowfish.hpp"

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/sslserver.hpp"
#include "server/dataqueue.hpp"

#include "world/event.hpp" // external event

octillion::LoginServer::LoginServer()
{    
    LOG_D(tag_) << "LoginServer";
    
    // read guest data from database
    deserialize_guest_data();
    
    // verify the guest data
    general_unused_guest_serial_ids();
}

void octillion::LoginServer::serialize_guest_data()
{
    std::ofstream ofs( "userdata", std::ofstream::out | std::ofstream::trunc );
    
    ofs << u8"[" << std::endl;
    
    for ( auto it = guest_data_.begin(); it != guest_data_.end(); it ++ )
    {
        JsonW juser;
        
        if ( it != guest_data_.begin() )
        {
            ofs << u8",";
        }
        
        juser[u8"id"] = (int)((*it).first);
        juser[u8"crypt"] = (*it).second.bcrypt_;
        juser[u8"salt"] = (*it).second.salt_;
        juser[u8"create"] = (*it).second.create_time_;
        ofs << juser.text() << std::endl;
    }
    
    ofs << u8"]" << std::endl;
}

void octillion::LoginServer::deserialize_guest_data()
{
    std::ifstream fin( "userdata" );
    JsonW jarray( fin );
    
    for (size_t i = 0; i < jarray.size(); i++)
    {
        octillion::LoginServer::User user;
        user.serial_id_ = jarray[i][u8"id"].integer();
        user.bcrypt_ = jarray[i][u8"crypt"].str();
        user.salt_ = jarray[i][u8"salt"].str();
        user.create_time_ = jarray[i][u8"create"].str();
        
        guest_data_.insert( std::pair<uint32_t, octillion::LoginServer::User>( user.serial_id_, user ));
    }
    
    LOG_D(tag_) << "deserialize_guest_data() read " << guest_data_.size() << " data(s).";
}

// get max_guest_serial_id_ and put all unused serial id into unused_guest_serial_ids_
void octillion::LoginServer::general_unused_guest_serial_ids()
{
    // verify the guest data
    max_guest_serial_id_ = 0;
    
    for ( auto it = guest_data_.begin(); it != guest_data_.end(); it ++ )
    {
        if ( (*it).second.serial_id_ > max_guest_serial_id_ )
        {
            max_guest_serial_id_ = (*it).second.serial_id_;
        }
    }
    
    LOG_D(tag_) << "max_guest_serial_id_ is " << max_guest_serial_id_;
    
    // verify the unused guest serial id used is true, unused is false
    if ( max_guest_serial_id_ > 0 )
    {
        int serial_id = 0;
        uint32_t unused_id_count = max_guest_serial_id_;
        std::vector<bool> serial_status( max_guest_serial_id_ + 1, false );
        
        for ( auto it = guest_data_.begin(); it != guest_data_.end(); it ++ )
        {
            serial_status.at((*it).second.serial_id_) = true;
            unused_id_count --;
        }
        
        unused_guest_serial_ids_.reserve( unused_id_count );
        
        // serial_id 0 is reserved
        for ( int i = 1; i < serial_status.size(); i ++ )
        {
            if ( serial_status.at(i) == false )
            {
                unused_guest_serial_ids_.push_back( i );
                LOG_D(tag_) << "unused id " << i;
            }
        }
    }
    
    it_unused_guest_serial_id_ = unused_guest_serial_ids_.begin();
    
    return;
}

octillion::LoginServer::~LoginServer()
{
    LOG_D(tag_) << "~LoginServer";
}

int octillion::LoginServer::recv( int fd, uint8_t* data, size_t datasize)
{
    JsonW jret;
    LOG_D(tag_) << "recv " << fd << " " << datasize << " bytes";
    
    rawdata_.feed( fd, data, datasize );
    
    // handle ready rawdata
    if ( rawdata_.size() > 0 )
    {
        std::vector<uint8_t> rawdata;
        int ret, rawfd;
        
        if ( OcError::E_SUCCESS != rawdata_.pop( rawfd, rawdata ))
        {
            LOG_D(tag_) << "Error: Fatal Error during rawdata_.pop";
            return -1;
        }
        
        ret = dispatch( rawfd, rawdata );
        
        rawdata_.remove( rawfd );
        
        return ret;
    }
    else
    {
        // send error message and close the fd
        jret[u8"result"] = u8"E_FATAL";        
        sendpacket( fd, jret.text() );
    }
    
    return 1;
}

void octillion::LoginServer::disconnect( int fd )
{
    LOG_D(tag_) << "disconnect " << fd;
}

void octillion::LoginServer::connect( int fd )
{
    LOG_D(tag_) << "connect " << fd;
}

int octillion::LoginServer::dispatch( int fd, std::vector<uint8_t>& data )
{
    octillion::Event event( fd, data );
    std::error_code err;
    
    if ( ! event.is_valid() )
    {
        LOG_D(tag_) << "dispatch() received invalid data";
        return 0;
    }
    
    if ( event.type_ == Event::TYPE_PLAYER_CREATE )
    {
        err = cmd_new( event.fd_ );
    }
    else if ( event.type_ == Event::TYPE_PLAYER_LOGIN )
    {
        err = cmd_login( fd, event.strparms_[0], event.strparms_[1] );
    }
    else if ( event.type_ == Event::TYPE_SERVER_VERIFY_TOKEN )
    {
        err = cmd_auth( fd, event.strparms_[0], event.strparms_[1], event.strparms_[2] );
    }
    else
    {
        LOG_D(tag_) << "dispatch() received invalid command: " << event.type_;
        return 0;
    }
   
    return 1;
}

std::error_code octillion::LoginServer::cmd_new( int fd )
{
    std::map<std::string,std::string>::iterator it;
    std::time_t time_now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    octillion::LoginServer::User user;

    JsonW jret;
    unsigned char buf[9];
    uint8_t digest[24];
    uint32_t unused_id = get_unused_serial_id();
    std::string username = serial_id_to_serial_name( unused_id );
    
    if ( RAND_bytes( buf,sizeof(buf) ) != 1 )
    {
        LOG_D(tag_) << "cmd_new ret E_FATAL";
        jret[u8"result"] = u8"E_FATAL";
        sendpacket( fd, jret.text() );
        return OcError::E_FATAL;
    }
    
    std::string passwd = base64( buf, sizeof(buf) );
    user.serial_id_ = unused_id;
    user.salt_ = get_16bytes_salt();
    user.bcrypt_ = blowfish( passwd, user.salt_ );
    user.create_time_ = std::ctime( &time_now );
        
    LOG_D(tag_) << "cmd_new add " << username << ":" << passwd 
        << " salt_:" << user.salt_ 
        << " time:" << user.create_time_ << "" 
        << " bcrypt:" << user.bcrypt_ << " into db";
        
    if ( user.serial_id_ == 0 || passwd.length() == 0 || 
        user.salt_.length() == 0 || user.bcrypt_.length() == 0 ||
        user.create_time_.length() == 0  )
    {
        LOG_D(tag_) << "cmd_new ret E_FATAL";
        jret[u8"result"] = u8"E_FATAL";
        sendpacket( fd, jret.text() );
        return OcError::E_FATAL;
    }
    
    user.create_time_.pop_back(); // remove the \n from ctime()
    guest_data_.insert( std::pair<uint32_t, octillion::LoginServer::User>( user.serial_id_, user ));
    max_guest_serial_id_ ++;    
    serialize_guest_data();
    
    jret[u8"result"] = u8"E_SUCCESS";
    jret[u8"id"] = username;
    jret[u8"password"] = passwd;
    sendpacket( fd, jret.text() );    
    return OcError::E_SUCCESS;
}

std::error_code octillion::LoginServer::cmd_login( int fd, std::string username, std::string passwd )
{
    JsonW jret;
    unsigned char key[128];
    uint32_t serial_id = serial_name_to_serial_id( username );
   
    auto it = guest_data_.find( serial_id );
    if ( it == guest_data_.end() )
    {
        LOG_D(tag_) << "cmd_login ret E_DB_NO_RECORD for serial_id:" << serial_id << " u:" << username;
        jret[u8"result"] = u8"E_DB_NO_RECORD";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_NO_RECORD;
    }

    if ( (*it).second.bcrypt_ == blowfish( passwd, (*it).second.salt_ ) )
    {
        int ret;
        Token token;
        token.ip = octillion::SslServer::get_instance().getip( fd );
        LOG_D(tag_) << "token.ip:" << token.ip;
        token.timestamp = (std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch())).count();
        ret = RAND_bytes(key,sizeof(key));
        
        if ( ret != 1 )
        {
            // error handler
            LOG_E(tag_) << "cmd_login failed to generate key";
            jret[u8"result"] = u8"E_FATAL";
            sendpacket( fd, jret.text() );
            return OcError::E_FATAL;
        }
        
        jret[u8"result"] = u8"E_SUCCESS";
        
        // encode token key
        token.key = base64(key, sizeof(key));
        
        jret[u8"token"] = token.key;
        
        // get game server ip address, and port
        jret[u8"ip"] = "127.0.0.1";
        jret[u8"port"] = "7000";
        
        sendpacket( fd, jret.text() );
        
        // remove the old token if any
        std::map<std::string,Token>::iterator ittoken;
        ittoken = tokens_.find( username );
        if ( ittoken != tokens_.end() )
        {
            tokens_.erase( username );
        }
        
        // insert new token
        tokens_.insert( std::pair<std::string, Token>( username, token ));
        
        LOG_D(tag_) << "cmd_login ret E_SUCCESS";
        return OcError::E_SUCCESS;
    }
    else
    {
        LOG_D(tag_) << "cmd_login ret E_DB_BAD_RECORD";
        jret[u8"result"] = u8"E_DB_BAD_RECORD";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_BAD_RECORD;
    }

    return OcError::E_SUCCESS;
}

std::error_code octillion::LoginServer::cmd_auth( 
            int fd, 
            std::string username, 
            std::string token,
            std::string ip
        )
{
    std::map<std::string,Token>::iterator it;
    JsonW jret;
    
    jret[u8"user"] = username; 
    
    if ( ! is_authserver( octillion::SslServer::get_instance().getip( fd )))
    {
        jret[u8"result"] = u8"E_FATAL";
        LOG_E(tag_) << "FATAL, auth request from unknown server";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_BAD_RECORD;
    }
    
    it = tokens_.find( username );
    if ( it == tokens_.end() )
    {
        jret[u8"result"] = u8"E_DB_BAD_RECORD";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_BAD_RECORD;
    }

    if ( (std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch())).count() 
            - (*it).second.timestamp> token_timeout_ )
    {
        tokens_.erase( it );
        LOG_D(tag_) << "user: " << username << " token timeout";        
        jret[u8"result"] = u8"E_FATAL";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_BAD_RECORD;
    }

    if ( (*it).second.key != token )
    {
        tokens_.erase( it );
        LOG_D(tag_) << "user: " << username << " token is wrong";
        jret[u8"result"] = u8"E_FATAL";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_BAD_RECORD;
    }

    if ( (*it).second.ip != ip )
    {
        tokens_.erase( it );
        LOG_D(tag_) << "user: " << username << " ip:" << (*it).second.ip << "-" << ip;
        jret[u8"result"] = u8"E_FATAL";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_BAD_RECORD;
    }

    tokens_.erase( it );
    jret[u8"id"] = (long long int)serial_name_to_serial_id( username );
    jret[u8"result"] = u8"E_SUCCESS"; 
    sendpacket( fd, jret.text() );    
    return OcError::E_SUCCESS;
}
    
bool octillion::LoginServer::is_authserver( std::string ip )
{
    LOG_D(tag_) << "authserer:" << ip << " is valid";
    return true;
}

void octillion::LoginServer::sendpacket( int fd, std::string rawdata )
{
    size_t rawdata_size;
    std::vector<uint8_t> packet;
    uint32_t nsize;
    
    LOG_D(tag_) << "sendpacket fd:" << fd << " data:" << rawdata;
    
    rawdata_size = strlen( rawdata.c_str() );
    packet.resize( rawdata_size + sizeof(uint32_t));
    nsize = ntohl( rawdata_size );    
    std::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
    std::memcpy( packet.data() + sizeof(uint32_t), rawdata.c_str(), rawdata_size );
    
    octillion::SslServer::get_instance().senddata( fd, packet.data(), packet.size(), true );
    
    return;
}

uint32_t octillion::LoginServer::get_unused_serial_id()
{
    if ( unused_guest_serial_ids_.size() > 0 &&
        it_unused_guest_serial_id_ != unused_guest_serial_ids_.end() )
    {
        uint32_t unused_id = (*it_unused_guest_serial_id_);
        it_unused_guest_serial_id_++;
        return unused_id;
    }
    
    return max_guest_serial_id_ + 1;
}

std::string octillion::LoginServer::blowfish( std::string password, std::string salt )
{
    uint8_t digest[24];
    if ( salt.length() != 16 )
    {
        return std::string();
    }
    ::blowfish_bcrypt( (void*) digest, (const char*)password.c_str(), password.length(), salt.c_str(), blow_fish_factor_ );
    return base64( digest, sizeof( digest ));
}

std::string octillion::LoginServer::get_16bytes_salt()
{
    std::string salt;
    unsigned char buf[3*6];
    
    if ( RAND_bytes( buf, sizeof( buf )) != 1 )
    {
        return salt;
    }
    
    salt = base64( buf, sizeof( buf ));
    salt.resize( 16 );
    
    return salt;
}

std::string octillion::LoginServer::serial_id_to_serial_name( uint32_t serial_id )
{
    std::stringstream stream;
    std::string id;
    
    stream << std::string( "guest#" ) <<  serial_id;
    stream >> id;
    
    return id;
}
    
uint32_t octillion::LoginServer::serial_name_to_serial_id( std::string serial_name )
{
    if ( serial_name.rfind( "guest#", 0 ) == 0 )
    {
        std::string strid = serial_name.substr(6);
        int id = 0;
        try
        {
            id = std::stoi( strid );
            return id;
        }
        catch (const std::invalid_argument& ex) 
        {
            LOG_E("LoginServer") << "Invalid guest id (invalid argument)" << serial_name;
            return 0;
        }
        catch (const std::out_of_range& ex)
        {
            LOG_E("LoginServer") << "Invalid guest id (out of range)" << serial_name;
            return 0;
        }
    }
    
    return 0;
}

std::string octillion::LoginServer::base64( uint8_t* data, size_t datasize )
{
    std::vector<uint8_t> base64;
    size_t bufsize, base64size;
    int padding;
    
    // data cannot be more than 128, this is not 
    // general base64 implementation
    if ( datasize > 128 || datasize == 0 )
    {
        return "";
    }
    
    switch ( datasize % 3 )
    {
        case 0: padding = 0; break;
        case 1: padding = 2; break;
        case 2: padding = 1; break;
        default: padding = 0;
    }

    bufsize = datasize + padding;     
    base64size = bufsize * 4 / 3;
    base64.resize( base64size );
    
    for ( int i = 0, j = 0; i < datasize; i = i + 3, j = j + 4 )
    {
        size_t tmp;
        tmp = data[i];
        tmp = i + 1 < datasize ? tmp << 8 | data[i+1] : tmp << 8;
        tmp = i + 2 < datasize ? tmp << 8 | data[i+2] : tmp << 8;
        
        base64[j] = b64str_[(tmp >> 18 ) & 0x3F];
        base64[j+1] = b64str_[(tmp >> 12 ) & 0x3F];
        
        if ( i + 1 < datasize ) 
        {
            base64[j+2] = b64str_[(tmp >> 6) & 0x3F];
        }
        else
        {
            base64[j+2] = '=';
        }
        
        if ( i + 2 < datasize ) 
        {
            base64[j+3] = b64str_[tmp & 0x3F];
        }
        else
        {
            base64[j+3] = '=';
        }
    }
 
    std::string retstr( (const char*)base64.data(), base64size );
    
    return retstr;
}
#include <system_error>
#include <string>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rand.h>

#include "jsonw/jsonw.hpp"

#include "loginserver.hpp"

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/sslserver.hpp"
#include "server/dataqueue.hpp"

octillion::LoginServer::LoginServer()
{    
    LOG_D(tag_) << "LoginServer";
}

octillion::LoginServer::~LoginServer()
{
    LOG_D(tag_) << "~LoginServer";
}

int octillion::LoginServer::recv( int fd, uint8_t* data, size_t datasize)
{
    LOG_D(tag_) << "recv " << fd << " " << datasize << " bytes";
    
    rawdata_.feed( fd, data, datasize );
    
    // handle ready rawdata
    if ( rawdata_.size() > 0 )
    {
        std::vector<uint8_t> rawdata;
        int ret, rawfd;
        
        rawdata.resize( rawdata_.peek() );
        if ( OcError::E_SUCCESS != rawdata_.pop( rawfd, rawdata.data(), rawdata.size() ))
        {
            LOG_D(tag_) << "Error: Fatal Error during rawdata_.pop";
            return -1;
        }
        
        ret = dispatch( rawfd, rawdata.data(), rawdata.size() );
        
        rawdata_.remove( rawfd );
        
        return ret;
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

int octillion::LoginServer::dispatch( int fd, uint8_t* data, size_t datasize )
{
    JsonW jobj( (const char*) data, datasize ); 
    JsonW jret;
    std::error_code err;

    LOG_D(tag_) << "(0) dispatch, fd:" << fd << " datasize:" << datasize;
    LOG_D(tag_) << "cmd:" << jobj[u8"cmd"].str();
    LOG_D(tag_) << "user:" << jobj[u8"user"].str();
    LOG_D(tag_) << "passwd:" << jobj[u8"passwd"].str();
    LOG_D(tag_) << "token:" << jobj[u8"token"].str();
    LOG_D(tag_) << "ip:" << jobj[u8"ip"].str();
    
    if ( jobj[u8"cmd"].str() == u8"new" )
    {  
        err = cmd_new( fd, jobj[u8"user"].str(), jobj[u8"passwd"].str() );
    }
    else if ( jobj[u8"cmd"].str() == u8"login" )
    {
        err = cmd_login( fd, jobj[u8"user"].str(), jobj[u8"passwd"].str() );
    }
    else if ( jobj[u8"cmd"].str() == u8"auth" )
    {
        err = cmd_auth( fd, jobj[u8"user"].str(), jobj[u8"token"].str(), jobj[u8"ip"].str() );
    }
    
    return 1;
}

std::error_code octillion::LoginServer::cmd_new( int fd, std::string username, std::string passwd )
{    
    // ALERT: MUST use correct algotithm to hash and store password!
    // salt (stored in db) + pepper (hard coded) + slow hash algorithm, 
    // Bcrypt with 12+ factor is the best choice,
    // SHA-512 is too fast and dangerous
    std::map<std::string,std::string>::iterator it;
    
    JsonW jret;

    it = password_.find( username );
    if ( it != password_.end() )
    {
        LOG_D(tag_) << "cmd_new ret E_DB_DUPLICATE_USERNAME";
        jret[u8"result"] = u8"E_DB_DUPLICATE_USERNAME";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_DUPLICATE_USERNAME;
    }

    LOG_D(tag_) << "cmd_new add " << username << ":" << passwd << " into db";
    password_.insert( std::pair<std::string, std::string>( username, passwd ));
    jret[u8"result"] = u8"E_SUCCESS";
    sendpacket( fd, jret.text() );    
    return OcError::E_SUCCESS;
}

std::error_code octillion::LoginServer::cmd_login( int fd, std::string username, std::string passwd )
{
    // ALERT: MUST use correct algotithm to hash and store password!
    std::map<std::string,std::string>::iterator it;
    JsonW jret;
    unsigned char key[128];

    it = password_.find( username );
    if ( it == password_.end() )
    {
        LOG_D(tag_) << "cmd_login ret E_DB_NO_RECORD";
        jret[u8"result"] = u8"E_DB_NO_RECORD";
        sendpacket( fd, jret.text() );
        return OcError::E_DB_NO_RECORD;
    }

    if ( (*it).second == passwd )
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
            OPENSSL_cleanse(key,sizeof(key));
            return OcError::E_FATAL;
        }
        
        jret[u8"result"] = u8"E_SUCCESS";
        
        // encode token key
        token.key = base64(key, sizeof(key));
        OPENSSL_cleanse(key,sizeof(key)); 
        
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

        OPENSSL_cleanse(key,sizeof(key));        
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
    
    if ( ! is_authserer( octillion::SslServer::get_instance().getip( fd )))
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
    jret[u8"result"] = u8"E_SUCCESS"; 
    sendpacket( fd, jret.text() );    
    return OcError::E_SUCCESS;
}
    
bool octillion::LoginServer::is_authserer( std::string ip )
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
    ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
    ::memcpy( packet.data() + sizeof(uint32_t), rawdata.c_str(), rawdata_size );
    
    octillion::SslServer::get_instance().senddata( fd, packet.data(), packet.size(), true );
    
    return;
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
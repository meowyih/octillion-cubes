#include <system_error>
#include <string>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rand.h>

#include "jsonw/jsonw.hpp"

#include "world/gameserver.hpp"

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/server.hpp"
#include "server/dataqueue.hpp"
#include "world/event.hpp"

#ifndef TEST_LOGIN_MECHANISM_ONLY  
#include "world/world.hpp"
#endif

octillion::GameServer::GameServer()
{
    LOG_D(tag_) << "GameServer";
    SslClient::get_instance().set_callback(this);
}

octillion::GameServer::~GameServer()
{
    SslClient::get_instance().force_stop();
    SslClient::get_instance().set_callback(NULL);
    LOG_D(tag_) << "~GameServer";
}

// recv data from end-users
int octillion::GameServer::recv( int fd, uint8_t* data, size_t datasize)
{
    JsonW jret;
    LOG_D(tag_) << "recv " << fd << " " << datasize << " bytes";

    rawdata_.feed( fd, data, datasize );
    
    // handle ready rawdata
    if ( rawdata_.size() > 0 )
    {
        std::vector<uint8_t> rawdata;
        int ret, rawfd, rawsize;
        
        rawdata.resize( rawdata_.peek() );
        if ( OcError::E_SUCCESS != rawdata_.pop( rawfd, rawdata.data(), rawdata.size() ))
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
        sendpacket( fd, jret.text(), true );
    }

    return 1;
}

void octillion::GameServer::disconnect( int fd )
{
    LOG_D(tag_) << "disconnect " << fd;
    
    // remove from both logining list and authorized list
    for ( auto it = loginsockets_.begin(); it != loginsockets_.end(); it ++ )
    {
        if ( (*it).second == fd )
        {
            loginsockets_.erase( it );
            break;
        }
    }

    if ( sockets_.find( fd ) != sockets_.end() )
    {
        // notify player disconnect        
#ifndef TEST_LOGIN_MECHANISM_ONLY
        octillion::Event event;
        event.type_ = octillion::Event::TYPE_PLAYER_DISCONNECT_WORLD;
        event.id_ = sockets_.at(fd);
        event.fd_ = fd;
        octillion::World::get_instance().add_event( event );
#endif // TEST_LOGIN_MECHANISM_ONLY
        
        sockets_.erase( fd );
    }
}

void octillion::GameServer::connect( int fd )
{
    LOG_D(tag_) << "connect " << fd;
}

int octillion::GameServer::dispatch( int fd, std::vector<uint8_t>& data )
{
    octillion::Event event( fd, data );
    
    if ( ! event.is_valid() )
    {
        LOG_D(tag_) << "dispatch() received invalid data";
        return 0;
    }
    
    if ( event.type_ == Event::TYPE_PLAYER_VERIFY_TOKEN )
    {
        if ( OcError::E_SUCCESS == cmd_login( fd, event.strparms_[0], event.strparms_[1] ))
        {
            return 1;
        }
        return 0;
    }

#ifndef TEST_LOGIN_MECHANISM_ONLY
    if ( octillion::World::get_instance().valid_event( event.type_ ) )
    {
        LOG_D(tag_) << "dispatch() " << event.type_ << " event to world";
        event.id_ = sockets_.at(fd);
        event.fd_ = fd;
        octillion::World::get_instance().add_event( event );
        return 1;
    }
#endif
        
    LOG_D(tag_) << "dispatch() received invalid command: " << event.type_;
    return 0;
}

std::error_code octillion::GameServer::cmd_login( int fd, std::string username, std::string token )
{
    JsonW jauth;

    std::map<std::string,int>::iterator it = loginsockets_.find( username );
    
    // get user ip
    std::string ip = octillion::Server::get_instance().getip( fd );
    
    if ( it != loginsockets_.end() )
    {
        LOG_W(tag_) << "overlapped login request, user:" << username;
        octillion::Server::get_instance().requestclosefd( (*it).second );
        loginsockets_.erase( username );
    }
    
    jauth["cmd"] = octillion::Event::TYPE_SERVER_VERIFY_TOKEN;
    jauth["user"] = username;
    jauth["token"] = token;
    jauth["ip"] = ip;
    
    LOG_D(tag_) << "cmd_login, send: " << jauth.text();
    
    loginsockets_.insert( std::pair<std::string,int>(username, fd) );
    sendpacket( fd, jauth.text(), false, true ); // send to login server

    return OcError::E_SUCCESS;
}

// recv data from login server
int octillion::GameServer::recv( int id, std::error_code error, uint8_t* data, size_t datasize)
{
    JsonW jret;
    int_fast32_t player_id;
    std::string username;
    auto it = loginsockets_.begin();
    
    // check if id (fd) exists in the loginsocket_
    for ( ; it != loginsockets_.end(); it ++ )
    {
        if ((*it).second == id )
        {
            username = (*it).first;
            break;
        }
    }
    
    // check if id (fd) exists in the loginsocket_
    if ( it == loginsockets_.end() )
    {
        // 1. login success
        // 2. something wierd happens, such as the fd was removed
        //    from loginsocket_ due to failed login
        // either way we simply ignore it
        return 0;
    }
    
    // login server returns fail and it is no disconnected event
    if ( error != OcError::E_SYS_STOP && error != OcError::E_SUCCESS )
    {
        LOG_D(tag_) << "recv from login server failed, err:" << error;

        // remove the login socket
        loginsockets_.erase( username );

        // return error message and request to close player's fd
        jret["result"] = "E_FATAL";
        jret["desc"] = "GS: Failed to access login lerver.";
        octillion::GameServer::sendpacket( id, jret.text(), true );
        return 0;
    }
    else if ( error == OcError::E_SYS_STOP || datasize == 0 )
    {
        // zombie event from login server, ignore it
        return 0;
    }
    
    JsonW jobj( (const char*) data +4, datasize - 4 );
    
    // check wrong token
    if ( jobj["result"].str() != u8"E_SUCCESS" )
    {
        jret["result"] = "E_FATAL";
        jret["desc"] = "token is wrong or expired";
        octillion::GameServer::sendpacket( id, jret.text(), true );
        loginsockets_.erase( username );
        return 0;
    }
    
    if ( username != jobj["user"].str() )
    {
        // username is different, might because of 
        // network lag w ran out of fd or hacker
        jret["result"] = "E_FATAL";
        jret["desc"] = "GS: Username dismatched";
        octillion::GameServer::sendpacket( id, jret.text(), true );
        loginsockets_.erase( username );
        return 0;
    }
    
    player_id = (int_fast32_t)(jobj["id"].integer());
    
    if ( id == 0 )
    {
        // username is different, might because of 
        // network lag w ran out of fd or hacker
        jret["result"] = "E_FATAL";
        jret["desc"] = "GS: login server error";
        octillion::GameServer::sendpacket( id, jret.text(), true );
        loginsockets_.erase( username );
        return 0;
    }
        
    // switch the socket from login list to connected list
    loginsockets_.erase( username );
    sockets_.insert( std::pair<int,int_fast32_t>(id, player_id) );
    
#ifdef TEST_LOGIN_MECHANISM_ONLY
    // if we are testing login function, there is no 'World' object, so
    // simply send a notification to user and close the socket
    
    jret["result"] = "E_SUCCESS";
    octillion::GameServer::sendpacket( id, jret.text(), true );
    loginsockets_.erase( username );

#else // ifndef TEST_LOGIN_MECHANISM_ONLY  

    // notify world there is a player login  
    octillion::Event event;
    event.type_ = octillion::Event::TYPE_PLAYER_CONNECT_WORLD;
    event.id_ = player_id;
    event.fd_ = id;
    octillion::World::get_instance().add_event( event );
    
#endif    
    return 0;
}

void octillion::GameServer::sendpacket( int fd, std::string rawdata, bool closefd, bool auth )
{
    size_t rawdata_size, packet_size;
    std::vector<uint8_t> packet;
    uint32_t nsize;

    LOG_D("GameServer") << "sendpacket fd:" << fd << " data:" << rawdata << " closefd?" << closefd;
    
    rawdata_size = strlen( rawdata.c_str() );
    packet.resize( rawdata_size + sizeof(uint32_t));
    nsize = ntohl( rawdata_size );    
    ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
    ::memcpy( packet.data() + sizeof(uint32_t), rawdata.c_str(), rawdata_size );
    
    if ( auth )
    {
        // send auth request to login server 
        SslClient::get_instance().write( fd, loginserver_addr, loginserver_port, packet.data(), packet.size() );
    }
    else
    {
        // send to client
        octillion::Server::get_instance().senddata( fd, packet.data(), packet.size(), closefd );
    }
    
    return;
}
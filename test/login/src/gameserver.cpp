#include <system_error>
#include <string>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rand.h>

#include "jsonw/jsonw.hpp"

#include "gameserver.hpp"

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/server.hpp"
#include "server/dataqueue.hpp"

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
        uint8_t* rawdata;
        int ret, rawfd, rawsize;
        
        rawsize = rawdata_.peek();
        rawdata = new uint8_t[rawsize];
        rawdata_.pop( rawfd, rawdata, rawsize );
        
        ret = dispatch( rawfd, rawdata, rawsize );
        
        delete [] rawdata;
        
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

    sockets_.erase( fd );
}

void octillion::GameServer::connect( int fd )
{
    LOG_D(tag_) << "connect " << fd;
}

int octillion::GameServer::dispatch( int fd, uint8_t* data, size_t datasize )
{
    JsonW jobj( (const char*) data, datasize ); 
    JsonW jret;
    std::error_code err;

    LOG_D(tag_) << "(0) dispatch, fd:" << fd << " datasize:" << datasize;
    LOG_D(tag_) << "cmd:" << jobj[u8"cmd"].str();
    LOG_D(tag_) << "user:" << jobj[u8"user"].str();
    LOG_D(tag_) << "token:" << jobj[u8"token"].str();
    
    if ( jobj[u8"cmd"].str() == u8"login" )
    {
        err = cmd_login( fd, jobj[u8"user"].str(), jobj[u8"token"].str() );
    }

    return 1;
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
    
    jauth["cmd"] = u8"auth";
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
        
    uint8_t *raw = new uint8_t[datasize];
    ::memset( raw, 0, datasize );
    ::memcpy( raw, data + 4, datasize - 4);
    
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
    
    jret["result"] = "E_SUCCESS";
    octillion::GameServer::sendpacket( id, jret.text() );
        
    // switch the socket from login list to connected list
    loginsockets_.erase( username );
    sockets_.insert( std::pair<int,std::string>(id, username) );
    
    return 0;
}

void octillion::GameServer::sendpacket( int fd, std::string rawdata, bool closefd, bool auth )
{
    size_t rawdata_size, packet_size;
    uint8_t* packet;
    uint32_t nsize;

    LOG_D(tag_) << "sendpacket fd:" << fd << " data:" << rawdata;
    
    rawdata_size = strlen( rawdata.c_str() );
    packet_size = rawdata_size + sizeof(uint32_t);
    nsize = ntohl( rawdata_size );    
    packet = new uint8_t[ packet_size ];
    ::memcpy( packet, &nsize, sizeof(uint32_t) );
    ::memcpy( packet + sizeof(uint32_t), rawdata.c_str(), rawdata_size );
    
    if ( auth )
    {
        // send auth request to login server 
        SslClient::get_instance().write( fd, loginserver_addr, loginserver_port, packet, packet_size );
    }
    else
    {
        // send to client
        octillion::Server::get_instance().senddata( fd, packet, packet_size, closefd );
    }
    
    
    delete [] packet;

    return;
}

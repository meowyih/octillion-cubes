#include <system_error>
#include <cstring>
#include <iostream>
#include <thread>
#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "ocerror.h"
#include "macrolog.h"
#include "coreserver.h"

octillion::CoreServer::CoreServer()
{   
    LOG_D(tag_) << "CoreServer()";
    
    is_running_ = false;
}

octillion::CoreServer::~CoreServer()
{
    // not to block the thread by core_thread_.join()
    // caller should use stop() before delete the CoreServer from memory
    core_thread_flag_ = false;
    core_thread_ = NULL;
    
    LOG_D(tag_) << "~CoreServer()";
}

std::error_code octillion::CoreServer::start( std::string port )
{
    std::error_code error;
    struct epoll_event event;
    
    port_ = port;
        
    LOG_D(tag_) << "start() enter, port:" << port;
        
    if ( is_running() )
    {
        LOG_E(tag_) << "start() leave, return E_SERVER_BUSY";
        return OcError::E_SERVER_BUSY;
    }
    
    error = init_server_socket();    
    if ( OcError::E_SUCCESS != error )
    {
        LOG_E(tag_) << "start() leave, return " << error;
        return error;
    }
    
    error = set_nonblocking( server_fd_ );
    if ( OcError::E_SUCCESS != error )
    {
        close( server_fd_ );
        LOG_E(tag_) << "start() leave, return " << error;
        return error;
    }
    
    if ( listen( server_fd_, SOMAXCONN ) == -1 )
    {
        close( server_fd_ );
        
        LOG_E(tag_) << "start() leave, return E_SYS_LISTEN" << 
            " message: " << strerror( errno );
        
        return OcError::E_SYS_LISTEN;
    }
    
    epoll_fd_ = epoll_create1(0);
    if ( epoll_fd_ == -1 )
    {
        close( server_fd_ );
        
        LOG_E(tag_) << "start() leave, return E_SYS_EPOLL_CREATE" << 
            " message: " << strerror( errno );
        
        return OcError::E_SYS_EPOLL_CREATE;
    }
    
    event.data.fd = server_fd_;
    event.events = EPOLLIN | EPOLLET;
    
    if ( epoll_ctl( epoll_fd_, EPOLL_CTL_ADD, server_fd_, &event ) == -1 )
    {
        close( server_fd_ );
        
        LOG_E(tag_) << "start() leave, return E_SYS_EPOLL_CTL" << 
            " message: " << strerror( errno );
            
        return OcError::E_SYS_EPOLL_CTL;
    }

    // enter epoll_wait() looping thread
    core_thread_flag_ = true;
    is_running_ = true;
    
    LOG_D(tag_) << "start() launch server thread";
    core_thread_ = new std::thread( &CoreServer::core_task, this );
    
    LOG_D(tag_) << "start() leave, return E_SUCCESS";
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::CoreServer::stop()
{
    LOG_D(tag_) << "stop() enter";
    
    // set the stop flag and wait it until finish
    core_thread_flag_ = false;
    
    if ( core_thread_ != NULL)
    {
        LOG_I(tag_) << "stop() wait server thread die";
        core_thread_->join();
        delete core_thread_;
        core_thread_ = NULL;
    }
    
    LOG_D(tag_) << "stop() leave";
    
    return OcError::E_SUCCESS;
}

void octillion::CoreServer::closesocket( int fd )
{
    LOG_D(tag_) << "closesocket() enter, fd: " << fd;
    close( fd );
    
    std::map<int, SSL*>::iterator iter = ssl_.find( fd );
    if ( iter != ssl_.end() )
    {
        SSL_free( iter->second );
        ssl_.erase( iter );
    }
    else
    {
        LOG_E(tag_) << "closesocket fd:" << fd << " does not exist in ssl_";
    }
}

std::error_code octillion::CoreServer::senddata( int socketfd, const void *buf, size_t len )
{
    ssize_t ret = send( socketfd, buf, len, 0 );
        
    if ( ret == -1 )
    {
        if ( errno == EAGAIN || errno == EWOULDBLOCK )
        {
            LOG_W(tag_) << "send() EAGAIN/EWOULDBLOCK fd:" << socketfd;
            return OcError::E_SYS_SEND_AGAIN;
        }
        else
        {
            LOG_E(tag_) << "send() failed fd:" << socketfd << " errno:" << errno << " msg:" << strerror( errno );
            return OcError::E_SYS_SEND;
        }
    }
    else if ( ret != len )
    {        
        // if only send partial data, recussively try again
        LOG_W(tag_) << "send() buffer insufficient send/total" << ret << "/" << len << " send again";
        return senddata( socketfd, (const void*)((uint8_t*)buf + ret), len - ret );
    }
    else
    {
        LOG_D( tag_ ) << "sendata, send " << ret << " bytes";
        return OcError::E_SUCCESS;
    }
}

void octillion::CoreServer::core_task()
{
    int ret;
    struct epoll_event event;
    struct epoll_event* events;
        
    events = new epoll_event[ kEpollBufferSize ]; 

    // init SSL
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
    
    if (!ctx) 
    {
        LOG_E(tag_) << "Unable to create SSL context";
        core_thread_flag_ = false;
    }
    
    SSL_CTX_set_ecdh_auto(ctx, 1);
    
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) 
    {
        LOG_E(tag_) << "Unable to set certificate";
        core_thread_flag_ = false;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, "cert.key", SSL_FILETYPE_PEM) <= 0 ) 
    {
        LOG_E(tag_) << "Unable to set private key";
        core_thread_flag_ = false;
    }
    
    // init server fd ssl
    SSL *ssl = SSL_new( ctx );
    SSL_set_accept_state( ssl );
    SSL_set_fd( ssl, server_fd_ );
    
    ssl_.insert( std::pair<int, SSL*>(server_fd_, ssl));
    
    // epoll while loop
    while( core_thread_flag_ )
    {
        LOG_D(tag_) << "core_task, epoll_wait() enter";
        ret = epoll_wait( epoll_fd_, events, kEpollBufferSize, kEpollTimeout );
        
        LOG_D(tag_) << "core_task, epoll_wait() ret events: " << ret;
                
        if ( ret == -1 )
        {
            LOG_E(tag_) << "core_task, epoll_wait() returns -1, errno: " << errno << " message: " << strerror( errno );
            break;
        }
        
        for ( int i = 0; i < ret; i ++ )
        {
            if (( events[i].events & EPOLLERR ) ||
                ( events[i].events & EPOLLHUP ) ||
               !(events[i].events & EPOLLIN)) 
            {
                // error occurred, disconnect this fd
                close( events[i].data.fd );
                
                LOG_D(tag_) << "core_task, close socket fd: " << events[i].data.fd;
                
                if ( callback_ != NULL )
                {
                    callback_->disconnect( events[i].data.fd );
                }

                std::map<int, SSL*>::iterator iter = ssl_.find( events[i].data.fd );
                if ( iter != ssl_.end() )
                {
                    SSL_free( iter->second );
                    ssl_.erase( iter );
                }
                else
                {
                    LOG_E(tag_) << "fd:" << events[i].data.fd << " does not exist in ssl_";
                }

                continue;
            }
            else if ( server_fd_ == events[i].data.fd )
            {
                // process all connection requests
                while( true )
                {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST],sbuf[NI_MAXSERV];

                    in_len = sizeof( in_addr );
                    
                    infd = accept( server_fd_, &in_addr, &in_len );
                    
                    if ( infd == -1 )
                    {
                        if (( errno == EAGAIN ) || ( errno == EWOULDBLOCK ))
                        {
                            // no more connection requests
                            break;
                        }
                        else
                        {
                            // error occurred
                            LOG_E(tag_) << "failed to accepted incoming connection, errno:" << errno <<
                                " message: " << strerror( errno );
                            break;
                        }
                    }
                    
                    ret = getnameinfo( &in_addr, in_len, 
                        hbuf, sizeof hbuf, sbuf, sizeof sbuf,
                        NI_NUMERICHOST | NI_NUMERICSERV);
                    
                    if ( ret == 0 )
                    {
                        // accepted connection fd: infd, host name: hbuf, service name: sbuf
                        LOG_D(tag_) << "core_task, accept socket fd: " << infd <<
                            " host: " << hbuf;
                    }
                                       
                    std::error_code error = set_nonblocking( infd );
                    if ( OcError::E_SUCCESS != error )
                    {
                        // fatal error occurred
                        core_thread_flag_ = false;
                        
                        LOG_E(tag_) << "core_task, failed to set socket to non-blocking, err: " << error;
                        
                        break;                        
                    }
                    
                    event.data.fd = infd;
                    event.events = EPOLLIN |EPOLLET;
                    ret = epoll_ctl( epoll_fd_, EPOLL_CTL_ADD, infd, &event );
                    
                    if( ret == -1 )
                    {
                        // fatal error occurred
                        core_thread_flag_ = false;
                        break;
                        
                        LOG_E(tag_) << "core_task, failed to set socket to non-blocking, fd: " << infd 
                            << " errno: " << errno
                            << " message: " << strerror( errno );
                    }

                    // ssl
                    SSL *ssl = SSL_new( ctx );
                    SSL_set_accept_state( ssl );
                    SSL_set_fd( ssl, infd );
                    
                    ssl_.insert( std::pair<int, SSL*>(infd, ssl));
                    
                    // callback, notify there is a new connection infd
                    if ( callback_ != NULL )
                    {
                        callback_->connect( infd );
                    }
                }
                
                continue;
            }
            else
            {
                // some data is ready for read, under EPOLLET mode, we read until there is no data
                std::map<int,SSL*>::iterator it = ssl_.find( events[i].data.fd );
                if ( it == ssl_.end() )
                {
                    LOG_E(tag_) << "fatal ssl error";
                    return;
                }

                SSL* ssl = it->second;
                if ( ! SSL_is_init_finished(ssl) )
                {
                    LOG_D(tag_) << "enter SSL_do_handshake";
                    SSL_do_handshake(ssl);
                    continue;
                }

                LOG_D(tag_) << "SSL established";

                while( 1 )
                {
                    ssize_t count;
                    char buf[128];
                    
                    // count = read( events[i].data.fd, buf, sizeof buf );
                    count = SSL_read( ssl, buf, sizeof buf );    

                    if ( count == -1 )
                    {
                        // if errno is EAGAIN, it means there is no more data
                        if ( errno != EAGAIN )
                        {
                            // error!
                            LOG_E(tag_) << "core_task, failed to set read socket: " << events[i].data.fd
                                << " message: " << strerror( errno );
                        }
                        break;
                    }
                    else if ( count == 0 )
                    {
                        // also, no more data
                        LOG_D(tag_) << "core_task, read socket 0 bytes";
                        break;
                    }
                    else
                    {
                        LOG_D(tag_) << "core_task, read socket: " << events[i].data.fd << " " << count << " bytes";
                        
                        if ( callback_ != NULL )
                        {
                            callback_->recv( events[i].data.fd, (uint8_t*)buf, (size_t)count );
                        }
                    }
                }
            }
        }
    }    
    
    if ( server_fd_ >= 0 )
    {
        LOG_I(tag_) << "core_task, close server fd: " << server_fd_;
        close( server_fd_ );
    }
    
    server_fd_ = -1;
    is_running_ = false;
    
    for (const auto& data : ssl_) 
    {
        LOG_D(tag_) << "core_task, recycle ssl_, fd:" << data.first;
        SSL_free( data.second );
    }
    
    SSL_CTX_free( ctx );
    
    delete [] events;  

    LOG_D(tag_) << "core_task leave";
}

std::error_code octillion::CoreServer::init_server_socket()
{
    int err;
    
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *rp;
    
    server_fd_ = -1;
    
    std::memset( &hints, 0, sizeof( struct addrinfo ));
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    err = getaddrinfo( NULL, port_.c_str(), &hints, &servinfo );
    
    if ( err != 0 )
    {
        LOG_E(tag_) << "init_server_socket, getaddrinfo() failed"
            << " errno: " << errno
            << " message: " << strerror( errno );
                            
        return OcError::E_SYS_GETADDRINFO;
    }
    
    for ( rp = servinfo; rp != NULL; rp = rp->ai_next )
    {
        server_fd_ = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
        
        if ( server_fd_ == -1 )
        {
            continue;
        }
        
        err = bind( server_fd_, rp->ai_addr, rp->ai_addrlen );
        
        if ( err == 0 )
        {
            break;
        }
        
        close( server_fd_ );
    }
    
    if ( rp == NULL )
    {
        server_fd_ = -1;
        freeaddrinfo( servinfo );
        
        LOG_E(tag_) << "init_server_socket, bind() failed"
            << " errno: " << errno
            << " message: " << strerror( errno );
        
        return OcError::E_SYS_BIND;
    }
    
    freeaddrinfo( servinfo );
        
    return OcError::E_SUCCESS;
}

std::error_code octillion::CoreServer::set_nonblocking( int fd )
{
    int flags, err;
    
    flags = fcntl( fd, F_GETFL, 0 );
    
    if ( flags == -1 )
    {
        return OcError::E_SYS_FCNTL;
    }
    
    flags |= O_NONBLOCK;
    err = fcntl( fd, F_SETFL, flags );
    
    if ( err == -1 )
    {
        LOG_E(tag_) << "set_nonblocking, fcntl() failed"
            " fd: " << fd << " errno: " << errno
            << " message: " << strerror( errno );
            
        return OcError::E_SYS_FCNTL;
    }
    
    return OcError::E_SUCCESS;
}

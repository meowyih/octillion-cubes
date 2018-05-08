#include <system_error>
#include <cstring>
#include <iostream>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include "ocerror.h"
#include "macrolog.h"
#include "coreserver.h"

octillion::CoreServer::CoreServer( std::string port )
{    
    is_running_ = false;
    port_ = port;
    server_fd_ = -1;
    epoll_fd_ = -1;
    epoll_timeout_ = 5 * 1000;
    epoll_buffer_size_ = 64;
    core_thread_ = NULL;
    callback_ = NULL;
    
    LOG_D() << "CoreServer(), port_: " << port_ 
        << " epoll_timeout_: " << epoll_timeout_ << "ms " 
        << " epoll_buffer_size_: " << epoll_buffer_size_;
}

octillion::CoreServer::~CoreServer()
{
    // not to block the thread by core_thread_.join()
    // caller should use stop() before delete the CoreServer from memory
    core_thread_flag_ = false;
    core_thread_ = NULL;
    
    LOG_D() << "~CoreServer()";
}

std::error_code octillion::CoreServer::start()
{
    std::error_code error;
    struct epoll_event event;
    
    LOG_D() << "CoreServer::start() enter";
        
    if ( is_running() )
    {
        LOG_E() << "CoreServer::start() leave, return E_SERVER_BUSY";
        return OcError::E_SERVER_BUSY;
    }
    
    error = init_server_socket();    
    if ( OcError::E_SUCCESS != error )
    {
        LOG_E() << "CoreServer::start() leave, return " << error;
        return error;
    }
    
    error = set_nonblocking( server_fd_ );
    if ( OcError::E_SUCCESS != error )
    {
        close( server_fd_ );
        LOG_E() << "CoreServer::start() leave, return " << error;
        return error;
    }
    
    if ( listen( server_fd_, SOMAXCONN ) == -1 )
    {
        close( server_fd_ );
        
        LOG_E() << "CoreServer::start() leave, return E_SYS_LISTEN" << 
            " message: " << strerror( errno );
        
        return OcError::E_SYS_LISTEN;
    }
    
    epoll_fd_ = epoll_create1(0);
    if ( epoll_fd_ == -1 )
    {
        close( server_fd_ );
        
        LOG_E() << "CoreServer::start() leave, return E_SYS_EPOLL_CREATE" << 
            " message: " << strerror( errno );
        
        return OcError::E_SYS_EPOLL_CREATE;
    }
    
    event.data.fd = server_fd_;
    event.events = EPOLLIN;
    
    if ( epoll_ctl( epoll_fd_, EPOLL_CTL_ADD, server_fd_, &event ) == -1 )
    {
        close( server_fd_ );
        
        LOG_E() << "CoreServer::start() leave, return E_SYS_EPOLL_CTL" << 
            " message: " << strerror( errno );
            
        return OcError::E_SYS_EPOLL_CTL;
    }
        
    // enter epoll_wait() looping thread
    core_thread_flag_ = true;
    is_running_ = true;
    
    LOG_D() << "CoreServer::start() launch server thread";
    core_thread_ = new std::thread( &CoreServer::core_task, this );
    
    LOG_D() << "CoreServer::start() leave, return E_SUCCESS";
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::CoreServer::stop()
{
    LOG_D() << "CoreServer::stop() enter";
    
    // set the stop flag and wait it until finish
    core_thread_flag_ = false;
    
    if ( core_thread_ != NULL)
    {
        LOG_D() << "CoreServer::stop() wait server thread die";
        core_thread_->join();
        core_thread_ = NULL;
    }
    
    LOG_D() << "CoreServer::stop() leave";
    
    return OcError::E_SUCCESS;
}

void octillion::CoreServer::core_task()
{
    int ret;
    struct epoll_event event;
    struct epoll_event* events;
        
    events = new epoll_event[ epoll_buffer_size_ ]; 
    
    while( core_thread_flag_ )
    {
        LOG_D() << "CoreServer::core_task, epoll_wait() enter";
        ret = epoll_wait( epoll_fd_, events, epoll_buffer_size_, epoll_timeout_ );
        
        LOG_D() << "CoreServer::core_task, epoll_wait() leave, return event size: " << ret;
                
        if ( ret == -1 )
        {
            LOG_E() << "CoreServer::core_task, epoll_wait() returns -1, errno: " << errno << " message: " << strerror( errno );
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
                
                LOG_D() << "CoreServer::core_task, close socket fd: " << events[i].data.fd;
                
                if ( callback_ != NULL )
                {
                    callback_->disconnect( events[i].data.fd );
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
                            LOG_E() << "failed to accepted incoming connection, errno:" << errno <<
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
                        LOG_D() << "CoreServer::core_task, accept socket fd: " << infd <<
                            "host: " << hbuf << " service:" << sbuf;
                    }
                                       
                    std::error_code error = set_nonblocking( infd );
                    if ( OcError::E_SUCCESS != error )
                    {
                        // fatal error occurred
                        core_thread_flag_ = false;
                        
                        LOG_E() << "CoreServer::core_task, failed to set socket to non-blocking, err: " << error;
                        
                        break;                        
                    }
                    
                    event.data.fd = infd;
                    event.events = EPOLLIN;
                    ret = epoll_ctl( epoll_fd_, EPOLL_CTL_ADD, infd, &event );
                    
                    if( ret == -1 )
                    {
                        // fatal error occurred
                        core_thread_flag_ = false;
                        break;
                        
                        LOG_E() << "CoreServer::core_task, failed to set socket to non-blocking, fd: " << infd 
                            << " errno: " << errno
                            << " message: " << strerror( errno );
                    }
                    
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
                // some data is ready for read 
                ssize_t count;
                char buf[512];
                
                count = read( events[i].data.fd, buf, sizeof buf );
                
                if ( count == -1 )
                {
                    // if errno is EAGAIN, it means there is no more data
                    if ( errno != EAGAIN )
                    {
                        // error!
                        LOG_E() << "CoreServer::core_task, failed to set read socket: " << events[i].data.fd
                            << " message: " << strerror( errno );
                    }
                }
                else if ( count == 0 )
                {
                    // also, no more data
                }
                else
                {
                    LOG_D() << "CoreServer::core_task, read socket: " << events[i].data.fd << " " << count << " bytes";
                    
                    if ( callback_ != NULL )
                    {
                        callback_->recv( events[i].data.fd, buf, (int)count );
                    }
                }
            }
        }
    }    
    
    if ( server_fd_ >= 0 )
    {
        LOG_I() << "CoreServer::core_task, close server fd: " << server_fd_;
        close( server_fd_ );
    }
    
    server_fd_ = -1;
    is_running_ = false;
    
    delete [] events;  

    LOG_D() << "CoreServer::core_task leave";
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
        LOG_E() << "CoreServer::init_server_socket, getaddrinfo() failed"
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
        
        LOG_E() << "CoreServer::init_server_socket, bind() failed"
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
        LOG_E() << "CoreServer::set_nonblocking, fcntl() failed"
            " fd: " << fd << " errno: " << errno
            << " message: " << strerror( errno );
            
        return OcError::E_SYS_FCNTL;
    }
    
    return OcError::E_SUCCESS;
}

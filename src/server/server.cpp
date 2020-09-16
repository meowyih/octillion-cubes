#include <system_error>
#include <cstring>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/server.hpp"

octillion::Server::Server()
{   
    LOG_D(tag_) << "Server()";
    
    is_running_ = false;
}

octillion::Server::~Server()
{   
    // not to block the thread by core_thread_.join()
    // caller should use stop() before delete the Server from memory
    core_thread_flag_ = false;
    core_thread_.reset();
    
    // clean up SSL_write retry waiting list
    // for (auto it = out_data_.begin(); it != out_data_.end(); ++it ) 
    // {
    //     LOG_W( tag_ ) << "~Server, remove waiting list fd:" << (*it).fd;
    //     delete [] (*it).data;
    // }
    out_data_.clear();
    
    LOG_D(tag_) << "~Server()";
}

std::error_code octillion::Server::start( std::string port )
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
        
    LOG_D(tag_) << "start() launch server thread";

    // note: make_unique is not available until c++17
    core_thread_ = std::make_unique<std::thread>( &Server::core_task, this );
    
    LOG_D(tag_) << "start() leave, return E_SUCCESS";
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::Server::stop()
{
    LOG_D(tag_) << "stop() enter";
    
    // set the stop flag and wait it until finish
    core_thread_flag_ = false;
    
    if ( core_thread_.get() != nullptr)
    {
        if ( core_thread_.get()->joinable() )
        {
            LOG_I(tag_) << "stop() wait server thread die";
            core_thread_.get()->join();
        }
        
        core_thread_.reset();
    }
    
    LOG_D(tag_) << "stop() leave";
    
    return OcError::E_SUCCESS;
}

void octillion::Server::core_task()
{
    int epollret, ret;
    struct epoll_event event;    
    // struct epoll_event* events;
    struct Socket socket;
    
    char recvbuf[512];
    
    is_running_ = true;
    
    std::unique_ptr<epoll_event[]> events 
        = std::make_unique<epoll_event[]>( kEpollBufferSize );
    
    // epoll while loop
    while( core_thread_flag_ )
    {
        // check if waiting list has fd need to be closed
        if (badfds_.size() > 0)
        {
            badfds_lock_.lock();

            for (auto it = badfds_.begin(); it != badfds_.end(); )
            {
                closesocket(*it);
                ++it;
            }

            badfds_.clear();
            badfds_lock_.unlock();
        }

            
        // write socket if writable and out_data_ have data
        out_data_lock_.lock();

        for (auto it = out_data_.begin(); it != out_data_.end(); ) 
        {
            int ret;
            
            std::map<int, Socket>::iterator itsocket = sockets_.find( (*it).fd );
            
            if( itsocket == sockets_.end() )
            {
                // something really bad happens
                LOG_E(tag_) << "Error, client fd " << (*it).fd 
                   << " in out_data_ does not exist in sockets_";
                requestclosefd( (*it).fd );
                ++it;
                continue;
            }
                        
            if ( ! itsocket->second.writable )
            {
                // socket is not writable
                LOG_W(tag_) << "core_task, socket " << (*it).fd << " is not writable";
                ++it;
                continue;
            }
            
            // ret = ::write( itsocket->second.fd, (*it).data, (*it).datalen );
            ret = ::write( itsocket->second.fd, (*it).data->data(), (*it).data->size() );
            
            // if ( ret == (*it).datalen )
            if ( ret == (*it).data->size() )
            {
                LOG_D( tag_ ) << "core_task, write done, fd:" << (*it).fd;
                // delete [] (*it).data;
                (*it).data.reset();                
                if ( (*it).closefd )
                {
                    requestclosefd( itsocket->second.fd );
                }                
                it = out_data_.erase(it);
            }
            else if ( ret >= 0 ) // partial write
            {
                // start to listen the EPOLLOUT event
                LOG_D(tag_) << "fd:" << (*it).fd << " start to listen EPOLLOUT";
                event.data.fd = (*it).fd;
                event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, (*it).fd, &event);
                itsocket->second.writable = false;
                ++it;
                
                if ( ret > 0 )
                {
                    // copy the remaining partial data
                    // size_t par_data_size = (*it).datalen - ret;                    
                    // uint8_t* par_data = new uint8_t[par_data_size];
                    // ::memcpy((void*)par_data, (*it).data + ret, par_data_size );
                    // delete [] (*it).data;
                    // (*it).data = par_data;
                    // (*it).datalen = par_data_size;
                    
                    std::shared_ptr<std::vector<uint8_t>> pdata
                        = std::make_shared<std::vector<uint8_t>>(
                        (*it).data->begin() + ret, (*it).data->end() );
                    
                    (*it).data.reset();
                    (*it).data = pdata;
                }
            }
            else 
            { 
                // something bad happen, remove the data and close the socket later
                LOG_W( tag_ ) << "core_task, senddata failed, fd:" << (*it).fd << 
                    " errno:" << errno << " " << strerror( errno );
                requestclosefd( (*it).fd );
                // delete [] (*it).data;
                (*it).data.reset();
                it = out_data_.erase(it);
            }
        }
        
        out_data_lock_.unlock();
                            
        LOG_D(tag_) << "epoll_wait() enter";
        epollret = epoll_wait( epoll_fd_, events.get(), kEpollBufferSize, kEpollTimeout );
        
        LOG_D(tag_) << "epoll_wait() ret events: " << epollret;
                
        if ( epollret == -1 )
        {
            LOG_E(tag_) << "epoll_wait() returns -1, errno: " << errno << " message: " << strerror( errno );
            break;
        }
        
        for ( int i = 0; i < epollret; i ++ )
        {   
            // debug purpose
            LOG_D(tag_) << "epoll event " << i << "/" << epollret 
                << " events:" << get_epoll_event( events[i].events )
                << " fd:" << events[i].data.fd;     
            
            // handle the bad event
            if (( events[i].events & EPOLLERR ) ||
                ( events[i].events & EPOLLHUP ) ||
               !((events[i].events & EPOLLIN) || (events[i].events & EPOLLOUT)))
            {
                // if the id is server id, it is fatal error
                if ( server_fd_ == events[i].data.fd )
                {
                    LOG_E(tag_) << "fatal error in core_task, error epoll event for is server fd";
                }
                else
                {
                    requestclosefd( events[i].data.fd ); 
                }
                continue;
            }
            
            // handle client fd's EPOLLOUT event
            if ( events[i].events & EPOLLOUT )
            {
                std::map<int, Socket>::iterator iter;
                iter = sockets_.find( events[i].data.fd );
                
                if( iter == sockets_.end() )
                {
                    // something really bad happens
                    LOG_E(tag_) << "Error, client fd " << events[i].data.fd 
                       << " does not exist in sockets_";
                    requestclosefd( events[i].data.fd );
                }
                else
                {
                    int ret;
                    LOG_D(tag_) << "set socket " << events[i].data.fd << " writable";
                    iter->second.writable = true;
                    
                    // stop listening the EPOLLOUT event
                    LOG_D(tag_) << "fd:" << events[i].data.fd << " stop listening EPOLLOUT";
                    event.data.fd = events[i].data.fd;
                    event.events = EPOLLIN | EPOLLET;
                    ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, events[i].data.fd, &event);
                    
                    if( ret == -1 )
                    {
                        // something really bad happens
                        LOG_E(tag_) << "Error, epoll_ctl mod EPOLLOUT failed, fd " << events[i].data.fd 
                           << " errno: " << errno
                           << " message: " << strerror( errno );
                        requestclosefd( events[i].data.fd );
                    }
                }
            }

            // handling remaining EPOLLIN event
            if ( ! ( events[i].events & EPOLLIN ))
            {
                continue;
            }

            if ( server_fd_ == events[i].data.fd )
            {
                // process all connection requests
                while( true )
                {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd, flags;
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

                    // set infd socket to non-blocking
                    if ( OcError::E_SUCCESS != set_nonblocking( infd ) )
                    {
                        requestclosefd( infd );
                        LOG_E(tag_) << "set_nonblocking( " << infd << ") failed";
                        continue;
                    }

                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    ret = epoll_ctl( epoll_fd_, EPOLL_CTL_ADD, infd, &event );
                    
                    if( ret == -1 )
                    {
                        // fatal error occurred
                        core_thread_flag_ = false;
                        LOG_E(tag_) << "core_task, failed to set socket to non-blocking, fd: " << infd 
                            << " errno: " << errno
                            << " message: " << strerror( errno );
                            
                        break;
                    }
                    else
                    {
                        socket.fd = infd;
                        socket.writable = true;
                        socket.s_addr = ((sockaddr_in*)&in_addr)->sin_addr.s_addr;
                        sockets_.insert( std::pair<int, Socket>(infd, socket) );
                        
                        // if SSL_accept is complete, call the callback
                        if ( callback_ != NULL )
                        {
                            callback_->connect( infd );
                        }
                        
                        LOG_D(tag_) << "socket accepted " << infd;
                    }
                }                
            }
            else
            {
                // some data is ready for read
                std::map<int,Socket>::iterator it = sockets_.find( events[i].data.fd );
                if ( it == sockets_.end() )
                {
                    LOG_E(tag_) << "fatal error, cannot find the socket in list";
                    return;
                }
                                
                // the client socket is readable, read the entire data
                while( true )
                {
                    // ET mode, read until no more data or error occurred              
                    ret = read( events[i].data.fd, recvbuf, sizeof recvbuf );
                    
                    if ( ret > 0 )
                    {
                        LOG_D(tag_) << "recv " << ret << " bytes";
                        if ( callback_ != NULL )
                        {
                            if ( callback_->recv( events[i].data.fd, (uint8_t*)recvbuf, (size_t)ret ) <= 0 )
                            {
                                LOG_W(tag_) << "recv fd: " << events[i].data.fd << " failed, closed it.";
                                closesocket( events[i].data.fd );
                                break;
                            }
                        }
                    }
                    else if ( ret == 0 )
                    {
                        // client disconnect
                        LOG_D(tag_) << "read, detect fd " << events[i].data.fd << " disconnected.";
                        closesocket( events[i].data.fd );
                        break;
                    }
                    else if ( errno == EAGAIN || errno == EWOULDBLOCK )
                    {
                        // no more data
                        LOG_D(tag_) << "read, no more data";   
                        break;
                    }
                    else
                    {
                        LOG_W(tag_) << "read failed, fd " << events[i].data.fd 
                           << " errno: " << get_errno_string()
                           << " message: " << strerror( errno );                        
                        closesocket( events[i].data.fd );
                        break;
                    }
                } // end of SSL_read while-loop

                LOG_D( tag_ ) << "end of SSL_read";
                
            } // end of event if-else block
        } // end of events for-loop
    } // end of epoll_wait while loop
    
    if ( server_fd_ >= 0 )
    {
        LOG_I(tag_) << "core_task, close server fd: " << server_fd_;
        close( server_fd_ );
    }
    
    server_fd_ = -1;
    is_running_ = false;  

    LOG_D(tag_) << "core_task leave";
}

std::error_code octillion::Server::init_server_socket()
{
    int err;
    int reuse = 0;
    
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
    
    LOG_D(tag_) << "create server socket " << server_fd_;
    
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

std::error_code octillion::Server::set_nonblocking( int fd )
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

std::error_code octillion::Server::senddata( int fd, const void *buf, size_t len, bool closefd )
{
    LOG_D( tag_ ) << "senddata_ts, add fd:" << fd << " datasize:" << len << " into out_data_";
    
    // copy into SSL_write waiting list
    out_data_lock_.lock();   
    
    DataBuffer buffer;
    buffer.fd = fd;
    buffer.closefd = closefd;
    
    // buffer.datalen = len;
    // buffer.data = new uint8_t[len];
    // ::memcpy((void*) buffer.data, (void*) buf, len );
    buffer.data = std::make_shared<std::vector<uint8_t>>( len, (uint8_t)0 );
    ::memcpy((void*) buffer.data->data(), (void*) buf, len );
    
    out_data_.push_back( buffer );    
    out_data_lock_.unlock();
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::Server::senddata( int fd, std::shared_ptr<std::vector<uint8_t>> data, bool closefd )
{
    LOG_D( tag_ ) << "senddata_ts, add fd:" << fd << " datasize:" << data.get()->size() << " into out_data_";
    
    // copy into SSL_write waiting list
    out_data_lock_.lock();   
    
    DataBuffer buffer;
    buffer.fd = fd;
    buffer.closefd = closefd;
    
    buffer.data = std::make_shared<std::vector<uint8_t>>( *data );
    
    out_data_.push_back( buffer );    
    out_data_lock_.unlock();
    
    return OcError::E_SUCCESS;
}

void octillion::Server::closesocket( int fd )
{
    LOG_D(tag_) << "closesocket() enter, fd: " << fd;
    close( fd );
    
    // clean up client socket
    std::map<int, Socket>::iterator iter = sockets_.find( fd );
    if ( iter != sockets_.end() )
    {
        sockets_.erase( iter );
    }
    else
    {
        LOG_E(tag_) << "closesocket fd:" << fd << " does not exist in sockets_";
    }
    
    // clean up waiting list for ::write
    out_data_lock_.lock();
    for (auto it = out_data_.begin(); it != out_data_.end(); ) 
    {
        if ((*it).fd == fd) 
        {
            // delete [] (*it).data;
            (*it).data.reset();
            it = out_data_.erase(it);
        } 
        else 
        {
            ++it;
        }
    }
    out_data_lock_.unlock();
    
    if ( callback_ != NULL )
    {
        callback_->disconnect( fd );
    }
}

std::string octillion::Server::get_epoll_event( uint32_t events )
{
    std::string str;
    if ( events & EPOLLERR )
        str.append( "EPOLLERR " );    
    if ( events & EPOLLHUP )
        str.append( "EPOLLHUP " );
    if ( events & EPOLLRDHUP )
        str.append( "EPOLLRDHUP " );
    if ( events & EPOLLIN )
        str.append( "EPOLLIN " );
    if ( events & EPOLLOUT )
        str.append( "EPOLLOUT " );
    if ( events & EPOLLPRI )
        str.append( "EPOLLPRI " );
    if ( events & EPOLLET )
        str.append( "EPOLLET " );
    if ( events & EPOLLONESHOT )
        str.append( "EPOLLONESHOT " );
    return str;
}

std::string octillion::Server::get_errno_string()
{
    if ( errno == EAGAIN ) return "EAGAIN";
    if ( errno == EWOULDBLOCK ) return "EWOULDBLOCK"; 
    if ( errno == EBADF ) return "EBADF"; 
    if ( errno == EFAULT ) return "EFAULT"; 
    if ( errno == EINTR ) return "EINTR"; 
    if ( errno == EINVAL ) return "EINVAL"; 
    if ( errno == EIO ) return "EIO"; 
    if ( errno == EISDIR ) return "EISDIR"; 
    return "OTHERS";
}

std::error_code octillion::Server::requestclosefd(int fd)
{
    LOG_D(tag_) << "requestclosefd fd:" << fd;

    badfds_lock_.lock();
    badfds_.push_back(fd);
    badfds_lock_.unlock();

    return OcError::E_SUCCESS;
}

std::string octillion::Server::getip( int fd )
{
    char str[INET_ADDRSTRLEN];
    std::map<int, Socket>::iterator it = sockets_.find( fd );
            
    if( it == sockets_.end() )
    {
        return std::string();
    }

    if ( inet_ntop(AF_INET, &(it->second.s_addr), str, INET_ADDRSTRLEN) == NULL )
    {
        return std::string();
    }        
    return std::string(str);
}
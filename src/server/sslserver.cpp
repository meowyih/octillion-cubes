#include <system_error>
#include <cstring>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>

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

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/sslserver.hpp"

octillion::SslServer::SslServer()
{   
    LOG_D(tag_) << "SslServer()";
    
    is_running_ = false;
}

octillion::SslServer::~SslServer()
{   
    // we don't block the thread by calling core_thread_.join()
    // caller should use stop() before delete the SslServer from memory
    core_thread_flag_ = false;
    core_thread_ = NULL;
    
    // clean up SSL_write waiting list
    out_data_.clear();
    
    LOG_D(tag_) << "~SslServer()";
}

std::error_code octillion::SslServer::start( std::string port, std::string key, std::string cert )
{
    std::error_code error;
    struct epoll_event event;
    
    port_ = port;
    key_ = key;
    cert_ = cert;
        
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
    core_thread_ = std::make_unique<std::thread>( &SslServer::core_task, this );
    
    LOG_D(tag_) << "start() leave, return E_SUCCESS";
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::SslServer::stop()
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

void octillion::SslServer::core_task()
{
    int epollret, ret;
    struct epoll_event event;
    // struct epoll_event* events;
    struct Socket socket;
    
    char recvbuf[512];
    
    is_running_ = true;
        
    std::unique_ptr<epoll_event[]> events 
        = std::make_unique<epoll_event[]>( kEpollBufferSize ); 

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
    
    if (SSL_CTX_use_certificate_file(ctx, cert_.c_str(), SSL_FILETYPE_PEM) <= 0) 
    {
        LOG_E(tag_) << "Unable to set certificate";
        core_thread_flag_ = false;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, key_.c_str(), SSL_FILETYPE_PEM) <= 0 ) 
    {
        LOG_E(tag_) << "Unable to set private key";
        core_thread_flag_ = false;
    }
    
    // ssl
    SSL *ssl = SSL_new( ctx );
    SSL_set_accept_state( ssl );
    SSL_set_fd( ssl, server_fd_ );
    server_ssl_ = ssl;
    
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
                        
            ret = SSL_write( itsocket->second.ssl, (*it).data->data(), (*it).data->size() );
            
            if ( ret > 0 )
            {
                LOG_D( tag_ ) << "core_task, SSL_write done, fd:" << (*it).fd;
                (*it).data.reset();                
                it = out_data_.erase(it);
            }
            else 
            {
                int sslerror = SSL_get_error( itsocket->second.ssl, ret );
                
                if ( sslerror == SSL_ERROR_WANT_READ  ||
                    sslerror == SSL_ERROR_WANT_WRITE )
                {
                    itsocket->second.writable = false;
                    
                    // start to listen the EPOLLOUT event
                    LOG_D(tag_) << "fd:" << (*it).fd << " start to listen EPOLLOUT";
                    event.data.fd = (*it).fd;
                    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    
                    if( epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, (*it).fd, &event) == -1 )
                    {
                        // something really bad happens
                        LOG_E(tag_) << "Error, epoll_ctl mod EPOLLOUT failed, fd " << (*it).fd 
                           << " errno: " << errno
                           << " message: " << strerror( errno );
                        requestclosefd( (*it).fd );
                        (*it).data.reset();                
                        it = out_data_.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                else
                {
                    // something bad happen, remove the data and close the socket later
                    LOG_W( tag_ ) << "core_task, senddata failed, fd:" << (*it).fd;
                    requestclosefd( (*it).fd );
                    (*it).data.reset();
                    it = out_data_.erase(it);
                    
                }
            }
            
            // disconnect the client in the next run if flag was set
            if ( (*it).disconnect )
            {
                LOG_D(tag_) << "close fd:" << (*it).fd << " after write";
                requestclosefd( (*it).fd );
                ++it;
            }
        }
        
        out_data_lock_.unlock();

        epollret = epoll_wait( epoll_fd_, events.get(), kEpollBufferSize, kEpollTimeout );
                
        if ( epollret == -1 )
        {
            LOG_E(tag_) << "epoll_wait() returns -1, errno: " << errno << " message: " << strerror( errno );
            break;
        }
        
        for ( int i = 0; i < epollret; i ++ )
        {   
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
                        break;
                        
                        LOG_E(tag_) << "core_task, failed to set socket to non-blocking, fd: " << infd 
                            << " errno: " << errno
                            << " message: " << strerror( errno );
                    }

                    // ssl
                    SSL *ssl = SSL_new( ctx );                   
                    SSL_set_accept_state( ssl );
                    SSL_set_fd( ssl, infd );
                                        
                    ret = SSL_accept(ssl);
                    
                    if ( ret == 1 ||
                       (SSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ || 
                        SSL_get_error(ssl, ret) == SSL_ERROR_WANT_WRITE ))
                    {
                        // create socket list no matter SSL_accept is complete or not                      
                        socket.fd = infd;
                        socket.writable = true;
                        socket.s_addr = ((sockaddr_in*)&in_addr)->sin_addr.s_addr;
                        socket.ssl = ssl;
                        sockets_.insert( std::pair<int, Socket>(infd, socket) );
                        
                        // if SSL_accept is complete, call the callback
                        if ( ret == 1 && callback_ != NULL )
                        {
                            callback_->connect( infd );
                        }
                    }
                    else
                    {
                        LOG_W(tag_) << "SSL_accept failed, fd:" << infd << " err:" << SSL_get_error(ssl, ret);
                        requestclosefd( infd );
                        continue;
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

                SSL* ssl = it->second.ssl;
                if ( ! SSL_is_init_finished(ssl) )
                {
                    // handshake should already be done during SSL_accept();
                    LOG_D(tag_) << "retry SSL_accept";
                    ret = SSL_accept(ssl);
                    
                    if ( ret == 1 )
                    {                    
                        // callback, notify there is a new connection infd
                        if ( callback_ != NULL )
                        {
                            callback_->connect( it->first );
                        }
                        
                        // in ET mode, server should continue the SSL_Read()
                    }
                    else if ( 
                       (SSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ || 
                        SSL_get_error(ssl, ret) == SSL_ERROR_WANT_WRITE ))
                    {
                        // SSL_accept will be done later;
                        LOG_D(tag_) << "SSL_accept retry not complete, fd:" << it->first;
                        continue;
                    }
                    else
                    {
                        LOG_W(tag_) << "SSL_accept failed, fd:" << it->first << " err:" << SSL_get_error(ssl, ret);
                        closesocket( it->first );
                        continue; 
                    }                    
                }
                                
                // the client socket is readable, read the entire data
                while( true )
                {
                    // ET mode, SSL_read until no more data or error occurred              
                    ret = SSL_read( ssl, recvbuf, sizeof recvbuf );
                    
                    if ( ret > 0 )
                    {
                        // we didn't set flag for partial read
                        if ( callback_ != NULL )
                        {
                            LOG_D(tag_) << "SSL_read before callback, out_data_.size()=" << out_data_.size();
                            if ( callback_->recv( events[i].data.fd, (uint8_t*)recvbuf, (size_t)ret ) <= 0 )
                            {
                                LOG_W(tag_) << "recv fd: " << events[i].data.fd << " failed, closed it.";
                                closesocket( events[i].data.fd );
                                break;
                            }
                            else
                            {
                                LOG_D(tag_) << "SSL_read after callback, out_data_.size()=" << out_data_.size();
                            }
                        }                    
                    }
                    else
                    {
                        int sslerror = SSL_get_error( ssl, ret );
                        
                        switch( sslerror )
                        {
                        case SSL_ERROR_WANT_READ:
                        case SSL_ERROR_WANT_WRITE:
                            LOG_D(tag_) << "SSL_read complete";
                            break;
                        case SSL_ERROR_SYSCALL:
                            LOG_D(tag_) << "SSL_read return SSL_ERROR_SYSCALL might because client disconnect";
                            closesocket( events[i].data.fd );
                            break;
                        default:
                            LOG_D(tag_) << "SSL_read return " << get_openssl_err(sslerror);
                            closesocket( events[i].data.fd );
                            break;
                        }
                        
                        break;
                    }
                } // end of SSL_read while-loop

                LOG_D( tag_ ) << "end of SSL_read, out_data_.size():" << out_data_.size();
                
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
    
    for (const auto& data : sockets_ ) 
    {
        LOG_D(tag_) << "core_task, recycle ssl_, fd:" << data.first;
        SSL_free( data.second.ssl );
    }
    
    SSL_free( server_ssl_ );
    
    SSL_CTX_free( ctx ); 

    LOG_D(tag_) << "core_task leave";
}

std::error_code octillion::SslServer::init_server_socket()
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

std::error_code octillion::SslServer::set_nonblocking( int fd )
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

std::error_code octillion::SslServer::senddata( int fd, const void *buf, size_t len, bool disconnect )
{
    LOG_D( tag_ ) << "senddata, add fd:" << fd << " datasize:" << len 
        << " disconnect:" << disconnect << " into out_data_";
    
    // copy into SSL_write waiting list
    out_data_lock_.lock();   
    
    DataBuffer buffer;
    buffer.fd = fd;
    buffer.disconnect = disconnect;
  
    buffer.data = std::make_shared<std::vector<uint8_t>>( len, (uint8_t)0 );
    ::memcpy((void*) buffer.data->data(), (void*) buf, len );

    out_data_.push_back( buffer );    
    out_data_lock_.unlock();
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::SslServer::senddata( int fd, const std::vector<uint8_t>& data, bool disconnect )
{
    LOG_D( tag_ ) << "senddata_ts, add fd:" << fd << " datasize:" << data.size() << " into out_data_";
    
    // copy into SSL_write waiting list
    out_data_lock_.lock();   
    
    DataBuffer buffer;
    buffer.fd = fd;
    buffer.disconnect = disconnect;
    
    buffer.data = std::make_shared<std::vector<uint8_t>>( data );
    
    out_data_.push_back( buffer );    
    out_data_lock_.unlock();
    
    return OcError::E_SUCCESS;
}

void octillion::SslServer::closesocket( int fd )
{
    LOG_D(tag_) << "closesocket() enter, fd: " << fd;
    close( fd );
    
    // clean up client socket
    std::map<int, Socket>::iterator iter = sockets_.find( fd );
    if ( iter != sockets_.end() )
    {
        SSL_free( iter->second.ssl );
        sockets_.erase( iter );
    }
    else
    {
        LOG_E(tag_) << "closesocket fd:" << fd << " does not exist in sockets_";
    }
    
    // clean up waiting list (SSL_write)
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

std::string octillion::SslServer::get_openssl_err( int sslerr )
{
    switch( sslerr )
    {
    case SSL_ERROR_WANT_READ:
        return "SSL_ERROR_WANT_READ";
    case SSL_ERROR_WANT_WRITE:
        return "SSL_ERROR_WANT_WRITE";
    case SSL_ERROR_WANT_CONNECT:
        return "SSL_ERROR_WANT_CONNECT";
    case SSL_ERROR_WANT_ACCEPT:
        return "SSL_ERROR_WANT_ACCEPT";
    case SSL_ERROR_WANT_X509_LOOKUP:
        return "SSL_ERROR_WANT_X509_LOOKUP";
    case SSL_ERROR_SYSCALL:
        return "SSL_ERROR_SYSCALL";
    case SSL_ERROR_SSL:
        return "SSL_ERROR_SSL";    
    case SSL_ERROR_ZERO_RETURN:
        return "SSL_ERROR_ZERO_RETURN";
    default:
        return "Unknown SSL error";
        break;
    }
}

std::string octillion::SslServer::get_epoll_event( uint32_t events )
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

std::error_code octillion::SslServer::requestclosefd(int fd)
{
    LOG_D(tag_) << "requestclosefd fd:" << fd;

    badfds_lock_.lock();
    badfds_.push_back(fd);
    badfds_lock_.unlock();

    return OcError::E_SUCCESS;
}

std::string octillion::SslServer::getip( int fd )
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
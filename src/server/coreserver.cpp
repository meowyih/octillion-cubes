#include <system_error>
#include <cstring>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>

#ifdef WIN32
// Currently no Win32 Implementation for coreserver
#else
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
#endif

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/coreserver.hpp"

#ifdef WIN32

// Currently no Win32 Implementation for coreserver
octillion::CoreServer::CoreServer() {};
octillion::CoreServer::~CoreServer() {};
std::error_code octillion::CoreServer::start(std::string port, std::string key, std::string cert) { return OcError::E_FATAL; }
std::error_code octillion::CoreServer::stop() { return OcError::E_FATAL; }
void octillion::CoreServer::closesocket(int fd) {}
void octillion::CoreServer::core_task() {}
std::error_code octillion::CoreServer::init_server_socket() { return OcError::E_FATAL; }
std::error_code octillion::CoreServer::senddata(int fd, const void *buf, size_t len, bool autoretry) { return OcError::E_FATAL; }
std::error_code octillion::CoreServer::set_nonblocking(int fd) { return OcError::E_FATAL; }

#else // linux


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
    
    // clean up SSL_write retry waiting list
    for (auto it = list_.begin(); it != list_.end(); ++it ) 
    {
        LOG_W( tag_ ) << "~CoreServer, remove waiting list fd:" << (*it).fd;
        delete [] (*it).data;
    }
    
    LOG_D(tag_) << "~CoreServer()";
}

std::error_code octillion::CoreServer::start( std::string port, std::string key, std::string cert )
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
    
    // clean up ssl
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
    
    // clean up waiting list (SSL_write)
    list_lock_.lock();
    for (auto it = list_.begin(); it != list_.end(); ) 
    {
        if ((*it).fd == fd) 
        {
            delete [] (*it).data;
            it = list_.erase(it);
        } 
        else 
        {
            ++it;
        }
    }
    list_lock_.unlock();
    
    if ( callback_ != NULL )
    {
        callback_->disconnect( fd );
    }
}

std::error_code octillion::CoreServer::senddata( int fd, const void *buf, size_t len, bool autoretry )
{           
    std::map<int,SSL*>::iterator it = ssl_.find( fd );
    if ( it == ssl_.end() )
    {
        LOG_E(tag_) << "senddata, fd:" << fd << " does not exist";
        return OcError::E_SYS_SEND;
    }

    SSL* ssl = it->second;
    
    // SSL_CTX partial data flag is disabled by default
    int ret = SSL_write( ssl, buf, len );
    
    if ( ret > 0 )
    {
        return OcError::E_SUCCESS;
    }
    else
    {
        int sslerror = SSL_get_error( server_ssl_, ret );
        
        switch( sslerror )
        {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            // errors that should retry again later
            LOG_W( tag_ ) << "SSL_write retry error, err:" << sslerror;
            
            if ( autoretry )
            {
                LOG_W( tag_ ) << "senddata, add fd:" << fd << " datasize:" << len << " into list_";
                
                // copy into SSL_write waiting list
                list_lock_.lock();   
                
                DataBuffer buffer;
                buffer.fd = fd;
                buffer.datalen = len;
                buffer.data = new uint8_t[len];
                memcpy((void*) buffer.data, (void*) buf, len ); 
                list_.push_back( buffer );
                
                list_lock_.unlock();
            }
            
            return OcError::E_SYS_SEND_AGAIN;
            
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
            // errors that cannot retry
            LOG_E( tag_ ) << "SSL_write non-retry error, err:" << sslerror;
            return OcError::E_SYS_SEND;

        default:
            // unknown error
            LOG_E( tag_ ) << "SSL_write unknown error, err:" << sslerror;
            return OcError::E_FATAL;
        }
    }  
}

void octillion::CoreServer::core_task()
{
    int epollret, ret;
    struct epoll_event event;
    struct epoll_event* events;
    
    is_running_ = true;
        
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
    
    ssl_.insert( std::pair<int, SSL*>(server_fd_, ssl));    
    server_ssl_ = ssl;
    
    // epoll while loop
    while( core_thread_flag_ )
    {
        LOG_D(tag_) << "core_task, epoll_wait() enter";
        epollret = epoll_wait( epoll_fd_, events, kEpollBufferSize, kEpollTimeout );
        
        LOG_D(tag_) << "core_task, epoll_wait() ret events: " << epollret;
                
        if ( epollret == -1 )
        {
            LOG_E(tag_) << "core_task, epoll_wait() returns -1, errno: " << errno << " message: " << strerror( errno );
            break;
        }

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
        
        // check if waiting list has data need to be re-send
        if ( list_.size() > 0 )
        {
            list_lock_.lock();

            // retry senddata
            for (auto it = list_.begin(); it != list_.end(); ) 
            {
                std::error_code stderr;
                
                stderr = senddata( (*it).fd, (void*)(*it).data, (size_t)(*it).datalen, false );
                if (stderr == OcError::E_SUCCESS ) 
                {
                    LOG_W( tag_ ) << "core_task, retry senddata done, fd:" << (*it).fd;
                    delete [] (*it).data;
                    it = list_.erase(it);
                }
                else if ( stderr == OcError::E_SYS_SEND || stderr == OcError::E_FATAL )
                {
                    LOG_W( tag_ ) << "core_task, retry senddata failed, fd:" << (*it).fd;
                    delete [] (*it).data;
                    it = list_.erase(it);
                }
                else // OcError::E_SYS_SEND_AGAIN
                {
                    LOG_W( tag_ ) << "core_task, retry senddata, retry again, fd:" << (*it).fd;
                    ++it;
                }
            }
            
            list_lock_.unlock();
        }
        
        for ( int i = 0; i < epollret; i ++ )
        {   
            // debug purpose
            LOG_D(tag_) << "epoll event " << i << "/" << epollret << " val:" << events[i].events;       
            if ( events[i].events & EPOLLERR )
            {
                LOG_D(tag_) << "EPOLLERR fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLHUP )
            {
                LOG_D(tag_) << "EPOLLHUP fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLRDHUP )
            {
                LOG_D(tag_) << "EPOLLRDHUP fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLIN )
            {
                LOG_D(tag_) << "EPOLLIN fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLOUT )
            {
                LOG_D(tag_) << "EPOLLOUT fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLPRI )
            {
                LOG_D(tag_) << "EPOLLPRI fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLET )
            {
                LOG_D(tag_) << "EPOLLET fd:" << events[i].data.fd;
            }
            
            if ( events[i].events & EPOLLONESHOT )
            {
                LOG_D(tag_) << "EPOLLONESHOT fd:" << events[i].data.fd;
            }
            
            // handling each event
            if (( events[i].events & EPOLLERR ) ||
                ( events[i].events & EPOLLHUP ) ||
               !(events[i].events & EPOLLIN)) 
            {
                // error occurred, disconnect this fd
                if (events[i].events & EPOLLERR)
                {
                    LOG_D(tag_) << "core_task, EPOLLERR close socket fd: " << events[i].data.fd;
                    int       error = 0;
                    socklen_t errlen = sizeof(error);
                    if (getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
                    {
                        printf("error = %s\n", strerror(error));
                    }
                }
                else if (events[i].events & EPOLLHUP)
                {
                    LOG_D(tag_) << "core_task, EPOLLHUP close socket fd: " << events[i].data.fd;
                }
                else if (!(events[i].events & EPOLLIN))
                {
                    LOG_D(tag_) << "core_task, ! EPOLLIN close socket fd: " << events[i].data.fd;
                }
                else
                {
                    LOG_D(tag_) << "core_task, unknown EPOLL event, close socket fd: " << events[i].data.fd;
                } 
                closesocket( events[i].data.fd ); 
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
                
                char buf[512];
                while( true )
                {
                    // SSL_read until no more data or error occurred                    
                    ret = SSL_read( ssl, buf, sizeof buf );
                    
                    if ( ret > 0 )
                    {
                        // we didn't set flag for partial read
                        if ( callback_ != NULL )
                        {
                            if ( callback_->recv( events[i].data.fd, (uint8_t*)buf, (size_t)ret ) <= 0 )
                            {
                                LOG_I(tag_) << "recv fd: " << events[i].data.fd << " failed, closed it.";
                                closesocket( events[i].data.fd );
                                break;
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
                            // continue reading, there is more data
                            LOG_D( tag_ ) << "SSL_read SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE, err:" << sslerror;
                            break;
                        case SSL_ERROR_WANT_CONNECT:
                        case SSL_ERROR_WANT_ACCEPT:
                        case SSL_ERROR_WANT_X509_LOOKUP:
                            // errors that should retry again later
                            LOG_W( tag_ ) << "SSL_read retry error, err:" << sslerror;
                            break;
                            
                        case SSL_ERROR_SYSCALL:
                        case SSL_ERROR_SSL:
                            // errors that cannot retry
                            LOG_E( tag_ ) << "SSL_read non-retry error, err:" << sslerror;
                            closesocket( events[i].data.fd );
                            break;

                        default:
                            // unknown error
                            LOG_E( tag_ ) << "SSL_read unknown error, err:" << sslerror;
                            closesocket( events[i].data.fd );
                            break;
                        }
                        
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

#endif // ifdef linux

std::error_code octillion::CoreServer::requestclosefd(int fd)
{
    LOG_D(tag_) << "requestclosefd fd:" << fd;

    badfds_lock_.lock();
    badfds_.push_back(fd);
    badfds_lock_.unlock();

    return OcError::E_SUCCESS;
}

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
#include "server/sslclient.hpp"

octillion::SslClient::SslClient()
{   
    LOG_D(tag_) << "SslClient()";
    
    // init SSL
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    
    wait_for_stop_ = false;
    
    // prepare ctx
    ctx_ = SSL_CTX_new(SSLv23_client_method());
}

octillion::SslClient::~SslClient()
{   
    LOG_D(tag_) << "~SslClient()";
    
    wait_for_stop_ = true;
    
    for (auto it = threads_.begin(); it != threads_.end(); ++it )
    {
        std::thread* thread = it->second;
        
        if ( thread->joinable() )
            thread->join();
        
        delete thread;
    }
    
    // free ctx
    LOG_D(tag_) << "~SslClient() SSL_CTX_free";
    SSL_CTX_free( ctx_ );
}

void octillion::SslClient::core_task(
    int id,
    std::string hostname, 
    std::string port, 
    uint8_t* data, 
    size_t datasize )
{
    int ret;
    int fd = 0;
    int flags;
    uint8_t buf[512];
    struct hostent *host;
    struct sockaddr_in addr;
    SSL *ssl = NULL;
    long int connect_start;
    
    LOG_D(tag_) << "core_task enter, id:" << id;
    
    // connect
    if (( host = ::gethostbyname( hostname.c_str() )) == NULL )
    {
        // error handler
        LOG_E(tag_) << "gethostbyname(" << hostname << ") failed id:" << id;
        callback( id, OcError::E_FATAL, NULL, 0 );
        delete [] data;
        return;
    }
    
    fd = ::socket( PF_INET, SOCK_STREAM, 0 );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( std::stoi( port ));
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
   
    // set non-blocking
    flags = fcntl( fd, F_GETFL, 0 );    
    if ( flags == -1 )
    {
        // error handler
        LOG_E(tag_) << "fcntl F_GETFL failed";
        ::close(fd);
        callback( id, OcError::E_FATAL, NULL, 0 );
        delete [] data;
        return;
    }
    
    flags |= O_NONBLOCK;
    flags = fcntl( fd, F_SETFL, flags );    
    if ( flags == -1 )
    {
        // error handler
        LOG_E(tag_) << "fcntl F_SETFL failed id:" << id;
        ::close(fd);
        callback( id, OcError::E_FATAL, NULL, 0 );
        delete [] data;
        return;
    }
        
    // blocking connect
    connect_start = (long int)(time(NULL));
    while( ! wait_for_stop_ )
    {
        if ( (long int)(time(NULL)) - connect_start > timeout_ )
        {
            // timeout!
            LOG_E(tag_) << "SSL_connect timeout in " << timeout_ << "sec, id:" << id;
            SSL_free(ssl);
            ::close( fd );
            callback( id, OcError::E_SYS_TIMEOUT, NULL, 0 );
            delete [] data;
            return;
        }
        
        ret = ::connect( fd, (struct sockaddr*)&addr, sizeof addr);
        if ( ret == 0 )
        {
            LOG_D(tag_) << "socket connect complete (ret:0) id:" << id;
            break;
        }
        else
        {
            if ( errno == EISCONN )
            {
                LOG_D(tag_) << "socket connect complete (errno:EISCONN) id:" << id;
                break;
            }
            else if ( errno == EINPROGRESS || errno == EAGAIN || errno == EALREADY )
            {
                // LOG_D(tag_) << "socket connect waiting (errno:EINPROGRESS/EAGAIN/EALREADY) id:" << id;
                continue;
            }
            else
            {
                // error 
                LOG_E(tag_) << "connect failed, err:" <<  strerror(errno) << " id:" << id;
                ::close(fd);
                callback( id, OcError::E_FATAL, NULL, 0 );
                delete [] data;
                return;
            }
        }
    }
    
    if ( wait_for_stop_ )
    {
        LOG_D(tag_) << "stop thread id:" << id;
        ::close(fd);
        callback( id, OcError::E_FATAL, NULL, 0 );
        delete [] data;
        return;
    }

    // prepare SSL
    ssl = SSL_new(ctx_);
    SSL_set_connect_state( ssl );
    SSL_set_fd(ssl, fd);

    // SSL_connect    
    while( ! wait_for_stop_ )
    {        
        ret = SSL_connect(ssl);
        if ( ret == 1 )
        {
            break;
        }
        else if ( ret == 0 )
        {
            // disconnect by client
            LOG_E(tag_) << "SSL_connect was disconnected id:" << id;
            SSL_free(ssl);
            ::close( fd );
            callback( id, OcError::E_FATAL, NULL, 0 );
            delete [] data;
            return;
        }
        else
        {
            switch ( SSL_get_error( ssl, ret ) )
            {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    continue;
                default:
                    LOG_E(tag_) << "SSL_connect failed";
                    SSL_free(ssl);
                    ::close( fd );
                    callback( id, OcError::E_FATAL, NULL, 0 );
                    delete [] data;
                    return;
            }
        }
    }
        
    // SSL_write
    while( ! wait_for_stop_ )
    {
        ret = SSL_write( ssl, data, datasize );
        if ( ret > 0 )
        {
            break; // done
        }
        else
        {
            switch ( SSL_get_error( ssl, ret ) )
            {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    continue;
                default:
                    LOG_E(tag_) << "SSL_write failed id:" << id;
                    SSL_free(ssl);
                    ::close( fd );
                    callback( id, OcError::E_FATAL, NULL, 0 );
                    delete [] data;
                    return;
            }
        }
    }
    
    while( ! wait_for_stop_ )
    {
        ret = SSL_read( ssl, buf, sizeof( buf ));
        if ( ret > 0 )
        {
            if ( callback( id, OcError::E_SUCCESS, buf, ret ) == 0 )
            {
                LOG_D(tag_) << "SSL_read, callback request stop, id:" << id;
                break;
            }
            continue;
        }
        else
        {
            int sslerror = SSL_get_error( ssl, ret );
            
            if ( sslerror == SSL_ERROR_WANT_READ || sslerror == SSL_ERROR_WANT_WRITE )
            {
                continue;
            }
            else if ( sslerror == SSL_ERROR_SYSCALL )
            {
                LOG_D(tag_) << "SSL_read disconnected by server (SSL_ERROR_SYSCALL) id:" << id;
                break;
            }
            else
            {
                LOG_D(tag_) << "SSL_read sslerror=" << sslerror << " id:" << id;;
                break;
            }
        }
    }
    
    delete [] data;
    SSL_free(ssl);
    ::close( fd ); 
    
    callback( id, OcError::E_SYS_STOP, NULL, 0 );
    
    LOG_D(tag_) << "core_task, leave id:" << id;
}

std::error_code octillion::SslClient::write( 
    int id,
    std::string hostname, 
    std::string port, 
    uint8_t* data, 
    size_t datasize )
{
    // WARNING: data should be deleted inside thread!   
    uint8_t* data_copy = new uint8_t[datasize];
    ::memcpy( (void*)data_copy, data, datasize );  

    std::thread* core_thread = new std::thread( 
        &octillion::SslClient::core_task, 
        this,
        id, 
        hostname, 
        port, 
        data_copy, 
        datasize );
    
    threads_.insert( std::pair<int, std::thread*>(id, core_thread) );

    return OcError::E_SUCCESS;
}

int octillion::SslClient::callback( int id, std::error_code error, uint8_t* data, size_t datasize) 
{
    if ( callback_ != NULL )
    {
        return callback_->recv( id, error, data, datasize );
    }
    else
    {
        return 1;
    }
}
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
    core_thread_ = NULL;
    is_running_ = false;
    core_thread_flag_ = false;
    data_ = NULL;
    
    // init SSL
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    
    // prepare ctx
    ctx_ = SSL_CTX_new(SSLv23_client_method());
}

octillion::SslClient::~SslClient()
{   
    LOG_D(tag_) << "~SslClient()";
    
    if ( core_thread_ != NULL )
    {        
        if ( core_thread_->joinable() )
        {
            LOG_I(tag_) << "core_thread_ is not NULL wait server thread die";
            core_thread_->join();
        }
        delete core_thread_;
        core_thread_ = NULL;
    }
    
    if ( data_ != NULL )
    {
        LOG_D(tag_) << "~SslClient() delete [] data_";
        delete [] data_;
        data_ = NULL;
    }
    
    // free ctx
    LOG_D(tag_) << "~SslClient() SSL_CTX_free";
    SSL_CTX_free( ctx_ );
}

bool octillion::SslClient::is_running()
{
    if ( is_running_ )
        return true;
    
    if ( core_thread_ == NULL )
        return false;
    
    if ( core_thread_->joinable() )
    {
        core_thread_->join();
    }
    
    return core_thread_->joinable();
}

std::error_code octillion::SslClient::write( 
    std::string hostname, 
    std::string port, 
    uint8_t* data, 
    size_t datasize )
{
    if ( is_running() )
    {
        // previous thread is still running
        LOG_I(tag_) << "write failed, is_running() is true";
        return OcError::E_FATAL;
    }
    
    if ( core_thread_ != NULL )
    {
        // delete the old dead thread
        LOG_I(tag_) << "delete core_thread_";
        delete core_thread_;
        core_thread_ = NULL;
    }
    
    // copy the data and server info, then start the sending thread
    hostname_ = hostname;
    port_ = port;
    is_running_ = true;
    wait_for_stop_ = false;
    
    if ( data_ != NULL )
    {
        LOG_I(tag_) << "delete [] data_";
        delete [] data_;
        data_ = NULL;
    }
    
    datasize_ = datasize;    
    data_ = new uint8_t[datasize_];
    ::memcpy( (void*)data_, data, datasize_ );    

    core_thread_ = new std::thread( &SslClient::core_task, this );
    return OcError::E_SUCCESS;
}

void octillion::SslClient::core_task()
{
    int ret;
    int fd = 0;
    int flags;
    uint8_t buf[512];
    struct hostent *host;
    struct sockaddr_in addr;
    SSL *ssl = NULL;
    
    LOG_D(tag_) << "core_task, enter";
    
    // connect
    if (( host = ::gethostbyname( hostname_.c_str() )) == NULL )
    {
        // error handler
        LOG_E(tag_) << "gethostbyname(" << hostname_ << ") failed";
        is_running_ = false;
        return;
    }
    
    fd = ::socket( PF_INET, SOCK_STREAM, 0 );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( std::stoi( port_ ));
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
        
    // blocking connect
    ret = ::connect( fd, (struct sockaddr*)&addr, sizeof addr);
    if ( ret == 0 )
    {
        LOG_D(tag_) << "socket connect complete";
    }
    else
    {
        // error 
        LOG_E(tag_) << "connect failed, err:" <<  strerror(errno);
        is_running_ = false;
        return;
    }
    
    // set non-blocking
    flags = fcntl( fd, F_GETFL, 0 );    
    if ( flags == -1 )
    {
        // error handler
        LOG_E(tag_) << "fcntl F_GETFL failed";
        is_running_ = false;
        return;
    }
    
    flags |= O_NONBLOCK;
    flags = fcntl( fd, F_SETFL, flags );    
    if ( flags == -1 )
    {
        // error handler
        LOG_E(tag_) << "fcntl F_SETFL failed";
        is_running_ = false;
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
            LOG_E(tag_) << "SSL_connect was disconnected";
            SSL_free(ssl);
            ::close( fd );
            is_running_ = false;
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
                    is_running_ = false;
                    return;
            }
        }
    }
        
    // SSL_write
    while( ! wait_for_stop_ )
    {
        ret = SSL_write( ssl, data_, datasize_ );
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
                    LOG_E(tag_) << "SSL_write failed";
                    SSL_free(ssl);
                    ::close( fd );
                    is_running_ = false;
                    return;
            }
        }
    }
    
    while( ! wait_for_stop_ )
    {
        ret = SSL_read( ssl, buf, sizeof( buf ));
        if ( ret > 0 )
        {
            if ( callback_ != NULL ) 
            {
                callback_->recv(buf, ret);
            }
            continue;
        }
    }
    
    SSL_free(ssl);
    ::close( fd );
    is_running_ = false; 
    
    LOG_D(tag_) << "core_task, leave";
}


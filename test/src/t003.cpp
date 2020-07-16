
#include <cstdlib>
#include <cstring>
#include <string>

// tcp and openssl
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

// error
#include "error/macrolog.hpp"
#include "error/ocerror.hpp"


#include "t003.hpp"

const std::string octillion::T003::tag_ = "T003";

octillion::T003::T003()
{
    LOG_D(tag_) << "T003()";
    data_ = NULL;
    is_data_ready_ = false;
}

octillion::T003::~T003()
{
    LOG_D(tag_) << "~T003()";
    
    if ( data_ != NULL ) 
    {
        delete [] data_;
    }
}

// virtual function from SslServerCallback that handlers all incoming events
void octillion::T003::connect( int fd ) 
{
    LOG_D(tag_) << "connect, fd=" << fd;
    client_fd_ = fd;
}

int octillion::T003::recv( int fd, uint8_t* data, size_t datasize)
{   
    // echo data back to client
    LOG_D(tag_) << "enter recv cb: read " << datasize << " bytes";
    data_ = new uint8_t[datasize];
    datasize_ = datasize;
    ::memcpy( data_, data, datasize_ );
    is_data_ready_ = true;
    octillion::SslServer::get_instance().senddata( fd, data, datasize );
    LOG_D(tag_) << "leave recv";
    return 1;
}

void octillion::T003::disconnect( int fd )
{
    client_fd_ = 0;
    LOG_D(tag_) << "disconnect, fd=" << fd;
}

void octillion::T003::test() 
{
    const int thread_size = 128;
    std::thread* core_thread_;
    
    core_thread_ = new std::thread( &T003::core_task, this );
    #if 0
    // wait server to send data back
    while ( true )
    {        
        if ( is_data_ready_ == true ) 
        {
            // send data from server using different thread
            if ( octillion::SslServer::get_instance().senddata( client_fd_, data_, datasize_ )
                != OcError::E_SUCCESS )
            {
                LOG_E(tag_) << "Failed, senddata_fs should return E_SUCCESS";
                abort();
            }
            
            LOG_D(tag_) << "break while loop";
            break;
        }
    }
    #endif
    
    core_thread_->join();
    delete core_thread_;
    
    LOG_E(tag_) << "Passed";
}

void octillion::T003::core_task()
{
    int     err;
    int     sock;
    struct  sockaddr_in server_addr;
    
    short int       s_port = 8888;
    const char      *s_ipaddr = "127.0.0.1";
    
    int datasize = 400;
    char junkdata[2000];
    char recvdata[2000];
    
    // ssl library has already init by server side
    SSL     *ssl;
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    
    if (!ctx) 
    {
        LOG_E(tag_) << "Unable to create SSL context";
        abort();
    }
    
    // init tcp socket
    sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    memset (&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family      = AF_INET; 
    server_addr.sin_port        = htons(s_port);       /* Server Port number */ 
    server_addr.sin_addr.s_addr = inet_addr(s_ipaddr); /* Server IP */
    
    err = ::connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
 
    if ( err == -1 ) 
    {
        LOG_E(tag_) << "Unable to connect";
        abort();
    }
        
    // ssl 
    ssl = SSL_new(ctx);
    
    if ( ssl == NULL ) 
    {
        LOG_E(tag_) << ssl << "failed in SSL_new";
        abort();
    }
    
    SSL_set_fd(ssl, sock);
    err = SSL_connect(ssl);
    
    if ( err != 1 ) 
    {
        LOG_E(tag_) << "failed in SSL_connect, err=" << err << " ssl err=" << SSL_get_error(ssl, err) << " SSL_ERROR_SYSCALL " << SSL_ERROR_SYSCALL;
        
        if ( SSL_get_error(ssl, err) == SSL_ERROR_SYSCALL ) 
        {
            LOG_E(tag_) << "errno " << errno;
        }
        // abort
        ::close(sock);
        SSL_free(ssl);   
        SSL_CTX_free(ctx);
        return;
    }
    
    // prepare test data
    for ( int i = 0; i < sizeof junkdata; i ++ ) 
    {
        junkdata[i] = i % 0xFF;
    }
    
    // send to server
    err = SSL_write(ssl, junkdata, datasize);
    
    if ( err != datasize )
    {
        LOG_E(tag_) << "failed in SSL_write, ret " << err;
        abort();
    }
    
    #if 0
    // check echo recv
    err = SSL_read( ssl, recvdata, sizeof recvdata );
    
    if ( err < 0 )
    {
        LOG_E(tag_) << "SSL_read ssl ret " << SSL_get_error(ssl, err);;
        abort();
    }
    else
    {
        for ( int i = 0; i < err; i ++ ) 
        {
            if ( recvdata[i] != (char)(i % 0xFF) )
            {
                LOG_E(tag_) << "SSL_read recv byte " << i << " is wrong " << (int)recvdata[i];
                abort();
            }
        }
    }
    #endif
    // close
    err = SSL_shutdown(ssl);
    
    if ( err == 0 ) 
    {
        unsigned int x = 0;
        
        do
        {
            x++;
            // NOTE, SSL_shutdown usually return -1 at the second try,
            // it is normal since the server already disconnected our socket.
            err = SSL_shutdown(ssl);
        } while ( err == 0 );
        
        LOG_D(tag_) << "SSL_shutdown completely, retry " << x << " times";
    }
    else if ( err < 0 ) 
    {
        LOG_W(tag_) << "failed in SSL_shutdown, err=" << err;
        abort();
    }
    
    err = ::close(sock);
    
    if ( err == -1 ) 
    {
        LOG_E(tag_) << "failed in close";
        abort();
    }
    
    SSL_free(ssl);   
    SSL_CTX_free(ctx);  
}


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


#include "t002.hpp"

const std::string octillion::T002::tag_ = "T002";

octillion::T002::T002()
{
    LOG_D(tag_) << "T002()";
}

octillion::T002::~T002()
{
    LOG_D(tag_) << "~T002()";
}


// virtual function from SslServerCallback that handlers all incoming events
void octillion::T002::connect( int fd ) 
{
    LOG_D(tag_) << "connect, fd=" << fd;
}

int octillion::T002::recv( int fd, uint8_t* data, size_t datasize)
{
    for ( int i = 0; i < datasize; i ++ )
    {
        if ( data[i] != ( i % 255 ))
        {
            LOG_E(tag_) << "failed on server side recv, datasize " << datasize << " idx-" << i << " " << data[i];
            abort();
        }
    }
    
    LOG_D(tag_) << "read " << datasize << " bytes";
    return 1;
}

void octillion::T002::disconnect( int fd )
{
    LOG_D(tag_) << "disconnect, fd=" << fd;
}

void octillion::T002::testMultipleConn()
{
    // create 2000 sockets to connect the server
    const int socksize = 400;
    bool      allclosed;
    int     retry = 0;
    int     err;
    int     sock[socksize];
    struct  sockaddr_in server_addr;
    
    short int       s_port = 8888;
    const char      *s_ipaddr = "127.0.0.1";
    
    char junkdata[512];
    
    // ssl library has already init by server side
    SSL     *ssl[socksize];
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    
    if (!ctx) 
    {
        LOG_E(tag_) << "Unable to create SSL context";
        abort();
    }
    
    memset (&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family      = AF_INET; 
    server_addr.sin_port        = htons(s_port);       /* Server Port number */ 
    server_addr.sin_addr.s_addr = inet_addr(s_ipaddr); /* Server IP */
    
    // init tcp socket
    for ( int i = 0; i < socksize; i ++ )
    {
        sock[i] = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
        err = ::connect(sock[i], (struct sockaddr*) &server_addr, sizeof(server_addr));
 
        if ( err == -1 ) 
        {
            LOG_E(tag_) << "Unable to connect";
            abort();
        }
        
        // ssl 
        ssl[i] = SSL_new(ctx);
        
        if ( ssl == NULL ) 
        {
            LOG_E(tag_) << ssl << "failed in SSL_new";
            abort();
        }
    
        SSL_set_fd(ssl[i], sock[i]);
        err = SSL_connect(ssl[i]);
        
        if ( err != 1 ) 
        {
            LOG_E(tag_) << "failed in SSL_connect";
            abort();
        }
    }
    
    // prepare test data
    for ( int i = 0; i < sizeof junkdata; i ++ ) 
    {
        junkdata[i] = i % 255;
    }
    
    for ( int i = 0; i < socksize; i ++ ) 
    {
        SSL_write( ssl[i], junkdata, i );
    }
    
    // shutdown all the ssl
    for ( int i = 0; i < socksize; i ++ ) 
    {
        err = SSL_shutdown(ssl[i]);
        if ( err > 0 )
        {
            err = ::close(sock[i]);                
            SSL_free(ssl[i]);
            ssl[i] = NULL;
        }
        else if ( err == 0 ) 
        {
            // retry later
        }
        else 
        {
            // error 
            LOG_W(tag_) << "failed in 1st trial SSL_shutdown, err=" << err << " idx-" << i << " sslerr=" << SSL_get_error(ssl[i], err);
            abort();
        }
    }

    // retry to shutdown the ssl
    do
    {
        allclosed = true;
        for ( int i = 0; i < socksize; i ++ )
        {
            if ( ssl[i] == NULL )
                continue;
            
            allclosed = false;
            
            err = SSL_shutdown(ssl[i]);
            
            if ( err > 0 || ( err < 0 &&  SSL_get_error(ssl[i], err) == SSL_ERROR_SYSCALL )) 
            {
                // SSL_ERROR_SYSCALL means server side close the socket
                err = ::close(sock[i]);                
                SSL_free(ssl[i]);
                ssl[i] = NULL;
                
                if ( err == -1 ) 
                {
                    LOG_E(tag_) << "failed in close socket[" << i << "]";
                    abort();
                }
            }
            else if ( err == 0 ) // retry later
            {
                LOG_D(tag_) << "retry shutdown";
                retry ++;
                continue;
            }
            else
            {
                LOG_W(tag_) << "failed in SSL_shutdown, err=" << err << " idx-" << i << " sslerr=" << SSL_get_error(ssl[i], err);
                abort();
            }                
        }
    } while ( ! allclosed );
    
    LOG_D(tag_) << "SSL_shutdown retry " << retry << " times";
    
    SSL_CTX_free(ctx);
    
    LOG_D(tag_) << "passed";    
}

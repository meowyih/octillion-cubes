
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


#include "t001.hpp"

const std::string octillion::T001::tag_ = "T001";

octillion::T001::T001()
{
    LOG_D(tag_) << "T001()";
}

octillion::T001::~T001()
{
    LOG_D(tag_) << "~T001()";
}


// virtual function from SslServerCallback that handlers all incoming events
void octillion::T001::connect( int fd ) 
{
    LOG_D(tag_) << "connect, fd=" << fd;
}

int octillion::T001::recv( int fd, uint8_t* data, size_t datasize)
{
    for ( int i = 0; i < datasize; i ++ )
    {
        if ( datasize != 464 && datasize < 512 && data[i] != ( i % 255 ))
        {
            LOG_E(tag_) << "failed on server side recv, datasize " << datasize << " idx-" << i << " " << data[i];
            abort();
        }
    }
    return 1;
}

void octillion::T001::disconnect( int fd )
{
    LOG_D(tag_) << "disconnect, fd=" << fd;
}

void octillion::T001::testSingleConn()
{
    int     err;
    int     sock;
    struct  sockaddr_in server_addr;
    
    short int       s_port = 8888;
    const char      *s_ipaddr = "127.0.0.1";
    
    char junkdata[2000];
    
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
        LOG_E(tag_) << "failed in SSL_connect";
        abort();
    }
    
    // prepare test data
    for ( int i = 0; i < sizeof junkdata; i ++ ) 
    {
        junkdata[i] = i % 255;
    }
    
    err = SSL_write(ssl, junkdata, 1);
    
    if ( err != 1 )
    {
        LOG_E(tag_) << "failed in SSL_write, ret " << err;
        abort();
    }
    
    err = SSL_write(ssl, junkdata, 3);    
    if ( err != 3 )
    {
        LOG_E(tag_) << "failed in SSL_write, ret " << err;
        abort();
    }
    
    err = SSL_write(ssl, junkdata, 4);    
    if ( err != 4 )
    {
        LOG_E(tag_) << "failed in SSL_write, ret " << err;
        abort();
    }

    err = SSL_write(ssl, junkdata, 500);    
    if ( err != 500 )
    {
        LOG_E(tag_) << "failed in SSL_write, ret " << err;
        abort();
    }

    err = SSL_write(ssl, junkdata, 2000);    
    if ( err != 2000 )
    {
        LOG_E(tag_) << "failed in SSL_write, ret " << err;
        abort();
    }
    
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
    
    LOG_D(tag_) << "passed";    
}

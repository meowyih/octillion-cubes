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

int main()
{
    int     err;
    int     sock;
    SSL     *ssl;
    struct  sockaddr_in server_addr;
    
    short int       s_port = 8888;
    const char      *s_ipaddr = "127.0.0.1";
    
    // Register the available ciphers and digests
    SSL_library_init ();
    
    // Register the error strings for libcrypto & libssl
    SSL_load_error_strings ();
    
    // init SSL
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    
    if ( ctx == NULL )
    {
        std::cout << "err: SSL_CTX is NULL";
        abort();
    }
    
    // create socket
    // init tcp socket
    sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    memset (&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family      = AF_INET; 
    server_addr.sin_port        = htons(s_port);       /* Server Port number */ 
    server_addr.sin_addr.s_addr = inet_addr(s_ipaddr); /* Server IP */
    
    err = ::connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
 
    if ( err == -1 ) 
    {
        std::cout << "Unable to connect" << std::endl;
        abort();
    }
    
    // ssl 
    ssl = SSL_new(ctx);
    
    if ( ssl == NULL ) 
    {
        std::cout << ssl << "failed in SSL_new" << std::endl;
        abort();
    }
    
    std::cout << " connected, before SSL_connect " << std::endl;
    
    SSL_set_connect_state( ssl );
    SSL_set_fd(ssl, sock);
    err = SSL_connect(ssl);
    
    if ( err != 1 ) 
    {
        std::cout << "failed in SSL_connect, err=" << err 
            << " errno=" << errno
            << " ssl err=" << SSL_get_error(ssl, err) 
            << " SSL_ERROR_SYSCALL " << SSL_ERROR_SYSCALL << std::endl;
        
        if ( SSL_get_error(ssl, err) == SSL_ERROR_SYSCALL ) 
        {
            std::cout << "errno " << errno << std::endl;
        }
        // abort
        ::close(sock);
        SSL_free(ssl);   
        SSL_CTX_free(ctx);
        return 0;
    }
    
    std::cout << " connected, before SSL_write " << std::endl;
    
    // write something and read the echo
    char recvdata[1000];
    ::memset( recvdata, 0, 1000 );
    err = SSL_write(ssl, "my abc data", 600);
    
    std::cout << " connected, after SSL_write " << std::endl;
    
    if ( err != 600 )
    {
        std::cout << "failed in SSL_write, ret " << err << std::endl;
        abort();
    }
    
    std::cout << std::endl << " before SSL_read " << std::endl;
    
    // check echo recv
    err = SSL_read( ssl, recvdata, sizeof recvdata );
    
    if ( err <= 0 )
    {
        std::cout << "failed in SSL_read, ret " << err << std::endl;
        abort();
    }
    else
    {
        std::cout << "SSL_read return " << err << std::endl;
    }
    
    std::cout << recvdata << std::endl;
        
    ::close(sock);
    SSL_free(ssl);   
    SSL_CTX_free(ctx);
    
    std::cout << "end of client.cpp" << std::endl;

    return 0;
}
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

int main()
{
    int     err;
    int     sock;
    struct  sockaddr_in server_addr;
    
    short int       s_port = 8888;
    const char      *s_ipaddr = "127.0.0.1";    
    
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
    else 
    {
        std::cout << "connect:" << err << std::endl;
    }
#if 0        
    std::cout << " connected, before write " << std::endl;
    
    // write something and read the echo
    char recvdata[1000];
    ::memset( recvdata, 0, 1000 );
    err = write(sick, "my abc data", 600);
    
    std::cout << " connected, after SSL_write " << std::endl;
    
    if ( err != 600 )
    {
        std::cout << "failed in SSL_write, ret " << err << std::endl;
        abort();
    }
    
    std::cout << std::endl << " before read " << std::endl;
    
    // check echo recv
    err = read( ssl, recvdata, sizeof recvdata );
    
    if ( err <= 0 )
    {
        std::cout << "failed in read, ret " << err << std::endl;
        abort();
    }
    else
    {
        std::cout << "read return " << err << std::endl;
    }
    
    std::cout << recvdata << std::endl;
#endif        
    ::close(sock);
    
    std::cout << "end of client.cpp" << std::endl;

    return 0;
}
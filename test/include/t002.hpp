#ifndef OCTILLION_T002_HEADER
#define OCTILLION_T002_HEADER

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

// core server
#include "server/sslserver.hpp"

namespace octillion
{
    class T002;    
}

class octillion::T002 : public octillion::SslServerCallback
{
    private:
        const static std::string tag_;

    public:
        T002();
        ~T002();	

    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
		
	public:
        void testMultipleConn();
};

#endif // OCTILLION_T002_HEADER
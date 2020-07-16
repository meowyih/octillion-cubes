#ifndef OCTILLION_T001_HEADER
#define OCTILLION_T001_HEADER

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
    class T001;    
}

class octillion::T001 : public octillion::SslServerCallback
{
    private:
        const static std::string tag_;

    public:
        T001();
        ~T001();	

    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
		
	public:
        void testSingleConn();
};

#endif // OCTILLION_T001_HEADER
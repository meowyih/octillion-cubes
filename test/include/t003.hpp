#ifndef OCTILLION_T003_HEADER
#define OCTILLION_T003_HEADER

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
    class T003;    
}

class octillion::T003 : public octillion::SslServerCallback
{
    private:
        const static std::string tag_;

    public:
        T003();
        ~T003();	

    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
		
	public:
        void test();
        
    private:
        void core_task();
        int  client_fd_;
        uint8_t *data_;
        size_t datasize_;
        bool is_data_ready_;
        
};

#endif // OCTILLION_T003_HEADER
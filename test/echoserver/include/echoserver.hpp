#ifndef OCTILLION_TEST_ECHO_SERVER_HEADER
#define OCTILLION_TEST_ECHO_SERVER_HEADER

#include <system_error>
#include <string>

#include "server/sslserver.hpp"

namespace octillion
{
    class EchoServer;
}

class octillion::EchoServer : public octillion::SslServerCallback
{
    private:
        const std::string tag_ = "EchoServer";

    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;        
};


#endif // OCTILLION_TEST_ECHO_SERVER_HEADER
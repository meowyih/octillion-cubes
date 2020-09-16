#ifndef OCTILLION_TEST_GAME_SERVER_HEADER
#define OCTILLION_TEST_GAME_SERVER_HEADER

#include <system_error>
#include <string>
#include <map>

#include "server/sslclient.hpp"
#include "server/server.hpp"
#include "server/dataqueue.hpp"

namespace octillion
{
    class GameServer;
}

class octillion::GameServer : public octillion::ServerCallback, public octillion::SslClientCallback
{
    private:
        const std::string tag_ = "GameServer";
        const std::string loginserver_addr = "127.0.0.1";
        const std::string loginserver_port = "8888";
        
    public:
        GameServer();
        ~GameServer();
    
    public:
        void sendpacket( int fd, std::string loading, bool closefd = false, bool auth = false );
        
        // virtual function from SslServerCallback that handles all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
        
        // virtual function for SslClientCallback that handles result from login server 
        virtual int recv( int id, std::error_code error, uint8_t* data, size_t datasize) override;

    private:    
        // return 0 to close the fd
        int dispatch( int fd, uint8_t* data, size_t datasize );
        std::error_code cmd_login( int fd, std::string username, std::string token );
        
    private:
        std::string ipaddress( int fd );
        
    private:
        octillion::DataQueue rawdata_;

        std::map<std::string,int> loginsockets_; // socket that try to login
        std::map<int,std::string> sockets_; // authorized socket
};

#endif // OCTILLION_TEST_GAME_SERVER_HEADER
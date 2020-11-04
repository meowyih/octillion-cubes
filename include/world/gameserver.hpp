#ifndef OCTILLION_GAME_SERVER_HEADER
#define OCTILLION_GAME_SERVER_HEADER

#include <system_error>
#include <string>
#include <map>

#include "server/sslclient.hpp"
#include "server/server.hpp"
#include "server/dataqueue.hpp"
#include "world/event.hpp"

namespace octillion
{
    class GameServer;
}

class octillion::GameServer : public octillion::ServerCallback, public octillion::SslClientCallback
{
    private:
        const std::string tag_ = "GameServer";
        constexpr static const char* loginserver_addr = "127.0.0.1";
        constexpr static const char* loginserver_port = "8888";
        
    public:
        GameServer();
        ~GameServer();
    
    public:
        // send data to player or login server during the login phase
        static void sendpacket( int fd, std::string loading, bool closefd = false, bool auth = false );
        
        // virtual function from SslServerCallback that handles all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
        
        // virtual function for SslClientCallback that handles result from login server 
        virtual int recv( int id, std::error_code error, uint8_t* data, size_t datasize) override;

    private:    
        // return 0 to close the fd
        int dispatch( int fd, std::vector<uint8_t>& data );
        std::error_code cmd_login( int fd, std::string username, std::string token );
        
    private:
        std::string ipaddress( int fd );
        
    private:
        octillion::DataQueue rawdata_;

        std::map<std::string,int> loginsockets_; // socket that try to login
        std::map<int,int_fast32_t> sockets_; // authorized socket to user id
};

#endif // OCTILLION_GAME_SERVER_HEADER
#ifndef OCTILLION_TEST_LOGIN_SERVER_HEADER
#define OCTILLION_TEST_LOGIN_SERVER_HEADER

#include <system_error>
#include <string>
#include <map>

#include "server/sslserver.hpp"
#include "server/dataqueue.hpp"

namespace octillion
{
    class UserDatabase;
    class LoginServer;
}

class octillion::LoginServer : public octillion::SslServerCallback
{
    private:
        const std::string tag_ = "LoginServer";
        
        const uint64_t token_timeout_ = 30 * 1000; // 30 sec
        
    public:
        LoginServer();
        ~LoginServer();

    public:
        // virtual function from SslServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;
        
    private:    
        // return 0 to close the fd
        int dispatch( int fd, uint8_t* data, size_t datasize );
        void sendpacket( int fd, std::string loading );
        std::error_code cmd_new( int fd, std::string username, std::string passwd );
        std::error_code cmd_login( int fd, std::string username, std::string passwd );
        std::error_code cmd_auth( 
            int fd, 
            std::string username, 
            std::string token,
            std::string ip
        );        
        bool is_authserer( std::string ip );
        
    private:
        char b64str_[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string base64( uint8_t* data, size_t datasize );
        std::string ipaddress( int fd );

    private:
        octillion::DataQueue rawdata_;
        
    private:
        // ALERT: MUST use correct algotithm to hash and store password here!
        std::map<std::string, std::string> password_;
        
    private:
        // client socket list
        struct Token
        {
            std::string   ip;
            uint64_t      timestamp;
            std::string   key;            
        };
        std::map<std::string,Token> tokens_;
};


#endif // OCTILLION_TEST_LOGIN_SERVER_HEADER
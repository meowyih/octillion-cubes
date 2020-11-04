#ifndef OCTILLION_AUTH_LOGIN_SERVER_HEADER
#define OCTILLION_AUTH_LOGIN_SERVER_HEADER

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
        
        const static uint64_t token_timeout_ = 30 * 1000; // 30 sec
        const static int      blow_fish_factor_ = 12;
        
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
        int dispatch( int fd, std::vector<uint8_t>& data );
        void sendpacket( int fd, std::string loading );
        std::error_code cmd_new( int fd );
        std::error_code cmd_login( int fd, std::string username, std::string passwd );
        std::error_code cmd_auth( 
            int fd, 
            std::string username, 
            std::string token,
            std::string ip
        );        
        
        // return true if ip is correct game server ip
        bool is_authserver( std::string ip );
        
        // predefined id tool
        uint32_t get_unused_serial_id();  

    public:
        std::string blowfish( std::string password, std::string salt );
        std::string get_16bytes_salt();
        std::string serial_id_to_serial_name( uint32_t serial_id );        
        uint32_t serial_name_to_serial_id( std::string serial_name );    
        
    private:
        char b64str_[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string base64( uint8_t* data, size_t datasize );
        std::string ipaddress( int fd );

    private:
        octillion::DataQueue rawdata_;
        
    private:
        struct User
        {
            uint32_t    serial_id_; // serial_id_ 0 is reserved
            std::string bcrypt_; // encrypt password
            std::string salt_;
            std::string create_time_;
        };
        
        std::map<uint32_t, octillion::LoginServer::User> guest_data_;
        std::vector<uint32_t>::iterator it_unused_guest_serial_id_;
        std::vector<uint32_t> unused_guest_serial_ids_; // used is true, unused is false
        uint32_t max_guest_serial_id_;

        void general_unused_guest_serial_ids(); 
        void serialize_guest_data();
        void deserialize_guest_data();
        
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


#endif // OCTILLION_AUTH_LOGIN_SERVER_HEADER
#include <cstring>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>
#include <memory>
#include <thread>

#include <signal.h>

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "jsonw/jsonw.hpp"
#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/sslclient.hpp"
#include "server/dataqueue.hpp"
#include "world/event.hpp"

volatile sig_atomic_t flag = 0;

std::string g_username;
std::string g_password;
std::string g_ip;
std::string g_port;
std::string g_token;

void gameloop();

void my_function(int sig)
{
    std::cout << "stopping the client..." << std::endl;
    octillion::SslClient::get_instance().force_stop();
    flag = 1;
}

class ClientCallbackNewUser : public octillion::SslClientCallback
{
    private:
        octillion::DataQueue dqueue_;
        
    public:       
        int recv( int id, std::error_code error, uint8_t* data, size_t datasize) override 
        {
            int tmp;
            std::vector<uint8_t> rawdata_arr;
            
            if ( error == OcError::E_FATAL )
            {
                std::cout << "id:" << id << " recv error: E_FATAL" << std::endl;
                return 0;
            }
            else if ( error == OcError::E_SYS_STOP )
            {
                std::cout << "id:" << id << " recv error: E_SYS_STOP" << std::endl;
                return 0;
            }
            else if ( error == OcError::E_SYS_TIMEOUT )
            {
                std::cout << "id:" << id << " recv error: E_SYS_TIMEOUT" << std::endl;
                return 0;
            }
            else if ( error != OcError::E_SUCCESS )
            {
                std::cout << "id:" << id << " recv error: others" << std::endl;
                return 0;
            }
            
            dqueue_.feed( id, data, datasize );
            
            if ( dqueue_.size() == 0 )
            {
                std::cout << "id: " << id << " incomplete data, " << datasize;
                return 1;
            }
            else if ( dqueue_.size() > 1 )
            {
                std::cout << "id: " << id << " recv more than one data from login server";
            }
            
            dqueue_.pop( tmp, rawdata_arr );
                       
            std::string raw( (const char*)rawdata_arr.data(), rawdata_arr.size() );            
            JsonW jret( (const char*)rawdata_arr.data(), rawdata_arr.size() );
            std::shared_ptr<JsonW> jid = jret.get( u8"id" );
            std::shared_ptr<JsonW> jpassword = jret.get( u8"password" );
            std::shared_ptr<JsonW> jresult = jret.get( u8"result" );
            
            if ( jid != nullptr && jresult != nullptr && jpassword != nullptr )
            {
                g_username = jid->str();
                g_password = jpassword->str();
                std::string sres = jresult->str();
                
                if ( sres != "E_SUCCESS" )
                {
                    std::cout << "bad result: " << raw;
                }
                else
                {
                    std::cout << std::endl 
                        << "username:" << g_username 
                        << " password:" << g_password << std::endl;
                }
                
                return 0;
            }
            
            std::cout << "bad result: " << raw;

            return 0; 
        }
};

class ClientCallbackLogin : public octillion::SslClientCallback
{
    private:
        octillion::DataQueue dqueue_;
        
    public:       
        int recv( int id, std::error_code error, uint8_t* data, size_t datasize) override 
        {
            int tmp;
            std::vector<uint8_t> rawdata_arr;
            
            if ( error == OcError::E_FATAL )
            {
                std::cout << "id:" << id << " recv error: E_FATAL" << std::endl;
                return 0;
            }
            else if ( error == OcError::E_SYS_STOP )
            {
                std::cout << "id:" << id << " recv error: E_SYS_STOP" << std::endl;
                return 0;
            }
            else if ( error == OcError::E_SYS_TIMEOUT )
            {
                std::cout << "id:" << id << " recv error: E_SYS_TIMEOUT" << std::endl;
                return 0;
            }
            else if ( error != OcError::E_SUCCESS )
            {
                std::cout << "id:" << id << " recv error: others" << std::endl;
                return 0;
            }
            
            dqueue_.feed( id, data, datasize );
            
            if ( dqueue_.size() == 0 )
            {
                std::cout << "id: " << id << " incomplete data, " << datasize;
                return 1;
            }
            else if ( dqueue_.size() > 1 )
            {
                std::cout << "id: " << id << " recv more than one data from login server";
            }
            
            dqueue_.pop( tmp, rawdata_arr );
                       
            std::string raw( (const char*)rawdata_arr.data(), rawdata_arr.size() );            
            JsonW jret( (const char*)rawdata_arr.data(), rawdata_arr.size() );
            
            if ( ! jret.valid() )
            {
                std::cout << "bad result: " << raw;
                return 0;
            }
            
            std::shared_ptr<JsonW> jip = jret.get( u8"ip" );
            std::shared_ptr<JsonW> jport = jret.get( u8"port" );
            std::shared_ptr<JsonW> jtoken = jret.get( u8"token" );
            std::shared_ptr<JsonW> jresult = jret.get( u8"result" );
            
            if ( jresult == nullptr || jresult->str() != "E_SUCCESS" )
            {
                std::cout << "bad result: " << raw;
                return 0;
            }
            else if ( jip != nullptr && jport != nullptr && jtoken != nullptr )
            {
                g_ip = jip->str();
                g_port = jport->str();
                g_token = jtoken->str();
                std::cout << "recv ip:" << g_ip << " port:" << g_port << " token:" << g_token;
                return 0;
            }
            
            std::cout << "bad result: " << raw;
            
            return 0; 
        }
};

int main()
{
    ClientCallbackNewUser cb_newuser;
    ClientCallbackLogin   cb_login;
    
    // send data to game server via socket (non-ssl)
    int sockfd = 0, n;
    struct sockaddr_in addr;
    struct hostent *host;
    
    while ( true )
    {
        // command menu
        // (1) create new account
        // (2) login
        int choose;
        std::string str;
        std::cout << "(1) new id/token" << std::endl;
        std::cout << "(2) login for token" << std::endl;
        std::cout << "(3) login game server" << std::endl;
        std::cout << "(4) X INC" << std::endl;
        std::cout << "(5) Y INC" << std::endl;
        std::cout << "(6) Z INC" << std::endl;
        std::cout << "(7) X DEC" << std::endl;
        std::cout << "(8) Y DEC" << std::endl;
        std::cout << "(9) Z DEC" << std::endl;
        std::cout << " >>> ";
        std::cin >> choose;
        
        // create new account
        if ( choose == 1 )
        {
            size_t rawdata_size;
            std::vector<uint8_t> packet;
            uint32_t nsize;
            
            ClientCallbackNewUser callback;
    
            JsonW jobj;
            
            std::cout << "create new player" << std::endl;
            
            jobj[u8"cmd"] = octillion::Event::TYPE_PLAYER_CREATE;
            
            rawdata_size = strlen( jobj.text().c_str() );   
            nsize = htonl( rawdata_size );    
            packet.resize( rawdata_size + sizeof(uint32_t) );
            ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
            ::memcpy( packet.data() + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
            
            octillion::SslClient::get_instance().set_callback( &cb_newuser );   
            
            // send id password to login ssl server
            octillion::SslClient::get_instance().write( 
                10, "127.0.0.1", "8888", packet.data(), 
                rawdata_size + sizeof(uint32_t) );           
        }
        else if ( choose == 2 )
        {
            size_t rawdata_size;
            std::vector<uint8_t> packet;
            uint32_t nsize;
            
            ClientCallbackLogin callback;
    
            JsonW jobj;
            
            std::cout << "login for token with username:" << g_username << " password:" << g_password; 
            
            jobj[u8"cmd"] = octillion::Event::TYPE_PLAYER_LOGIN;
            jobj[u8"user"] = g_username;
            jobj[u8"passwd"] = g_password;
            
            rawdata_size = strlen( jobj.text().c_str() );   
            nsize = htonl( rawdata_size );    
            packet.resize( rawdata_size + sizeof(uint32_t) );
            ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
            ::memcpy( packet.data() + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
            
            octillion::SslClient::get_instance().set_callback( &cb_login );   
            
            // send id password to login ssl server
            octillion::SslClient::get_instance().write( 
                10, "127.0.0.1", "8888", packet.data(), 
                rawdata_size + sizeof(uint32_t) );      
        }
        else
        {
            int event_type;
                            
            std::error_code err;
            size_t rawdata_size;
            std::vector<uint8_t> packet;
            uint32_t nsize;
            JsonW jobj;
            
            if ( sockfd == 0 )
            {
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
            
                host = ::gethostbyname(g_ip.c_str());
                ::memset((char *) &addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons( std::stoi( g_port ));
                addr.sin_addr.s_addr = *(long*)(host->h_addr);
                
                std::cout << "connecting to " << g_ip << ":" << g_port << std::endl;    
                if ( connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0 )
                {
                    std::cout << "Successfully connect to game server." << std::endl;
                }
                else
                {
                    std::cout << "Failed to connect to game server." << std::endl;
                    sockfd = 0;
                    continue;
                }
            }
            
            switch( choose )
            {
            case 3:
                event_type = octillion::Event::TYPE_PLAYER_VERIFY_TOKEN;
                break;
            case 4:
                event_type = octillion::Event::TYPE_CMD_MOVE_X_INC;
                break;
            case 5:
                event_type = octillion::Event::TYPE_CMD_MOVE_Y_INC;
                break;
            case 6:
                event_type = octillion::Event::TYPE_CMD_MOVE_Z_INC;
                break;
            case 7:
                event_type = octillion::Event::TYPE_CMD_MOVE_X_DEC;
                break;
            case 8:
                event_type = octillion::Event::TYPE_CMD_MOVE_Y_DEC;
                break;
            case 9:
                event_type = octillion::Event::TYPE_CMD_MOVE_Z_DEC;
                break;                
            default:
                return 0;
            }
            
            // connect to game server
            jobj[u8"cmd"] = event_type;
            
            if ( event_type == octillion::Event::TYPE_PLAYER_VERIFY_TOKEN )
            {
                jobj[u8"user"] = g_username;
                jobj[u8"token"] = g_token;
            }

            rawdata_size = strlen( jobj.text().c_str() );   
            nsize = htonl( rawdata_size );    
            packet.resize( rawdata_size + sizeof(uint32_t) );
            ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
            ::memcpy( packet.data() + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
            
            n = ::write(sockfd, packet.data(), packet.size());
            
            std::cout << "write " << n << " bytes." << std::endl;
            
            uint8_t recvdata[512];
            ::memset( recvdata, 0 , 512 );
            n = ::read(sockfd, recvdata, 512 );
            std::cout << "read " << n << " bytes.\n";
            std::cout << ( recvdata + 4 ) << std::endl;
        }
    }
        
    return 0;
}
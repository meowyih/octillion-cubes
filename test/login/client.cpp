#include <cstring>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

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

volatile sig_atomic_t flag = 0;

std::string g_username;

void my_function(int sig)
{
    std::cout << "stopping the client..." << std::endl;
    octillion::SslClient::get_instance().force_stop();
    flag = 1;
}

class ClientCallback : public octillion::SslClientCallback
{
    public:       
        int recv( int id, std::error_code error, uint8_t* data, size_t datasize) override 
        {
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
            
            std::vector<uint8_t> buffer( datasize - 4, 0 );
            
            ::memcpy((void*)buffer.data(), data + 4, datasize - 4 );
            std::string raw( (const char*)buffer.data(), datasize - 4 );
            
            octillion::JsonW jret( (const char*)buffer.data(), datasize - 4 );
            octillion::JsonW* jtoken = jret.get( u8"token" );
            if ( jtoken != NULL )
            {
                octillion::JsonW jobj;
                std::error_code err;
                size_t rawdata_size;
                // uint8_t* packet;
                std::vector<uint8_t> packet;
                uint32_t nsize;
                
                std::string ip, port, token;
                ip = jret.get( u8"ip" )->str();
                port = jret.get( u8"port" )->str();
                token = jtoken->str();
                std::cout << "ip:" << ip << ":" << port << " token:" << token << std::endl;
                
                // connect to game server
                jobj[u8"cmd"] = u8"login";
                jobj[u8"user"] = g_username;
                jobj[u8"token"] = token;
                
                rawdata_size = strlen( jobj.text().c_str() );
                std::cout << "rawdata_size: " << rawdata_size << std::endl;
                nsize = htonl( rawdata_size );    
                // packet = new uint8_t[rawdata_size + sizeof(uint32_t)];
                // ::memcpy( packet, &nsize, sizeof(uint32_t) );
                // ::memcpy( packet + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
                
                packet.resize( rawdata_size + sizeof(uint32_t) );
                ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
                ::memcpy( packet.data() + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
                
                // send data to game server via socket (non-ssl)
                int sockfd, n;
                struct sockaddr_in addr;
                struct hostent *host;
                
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                
                host = ::gethostbyname(ip.c_str());
                ::memset((char *) &addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons( std::stoi( port ));
                addr.sin_addr.s_addr = *(long*)(host->h_addr);
                connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
                n = ::write(sockfd, packet.data(), packet.size());
                
                uint8_t recvdata[512];
                ::memset( recvdata, 0 , 512 );
                n = ::read(sockfd, recvdata, 512 );
                std::cout << "read " << n << " bytes.\n";
                std::cout << ( recvdata + 4 ) << std::endl;
            }
            else
            {
                std::cout << "id:" << id << " recv size:" << raw << " data:" << raw << std::endl;
            }
                        
            flag = 1;
            
            return 0; 
        }
};

int main()
{
    ClientCallback callback;
    octillion::SslClient::get_instance().set_callback( &callback );   
    
    while ( true )
    {
        // command menu
        // (1) create new account
        // (2) login
        int choose;
        std::string str;
        std::cout << "(1) new" << std::endl;
        std::cout << "(2) login" << std::endl;
        std::cout << " >>> ";
        std::cin >> choose;
        
        // create new account
        if ( choose == 1 )
        {
            std::error_code err;
            size_t rawdata_size;
            std::vector<uint8_t> packet;
            uint32_t nsize;
    
            octillion::JsonW jobj;
            
            std::string id, password;
            
            std::cout << "create new player" << std::endl;
            std::cout << "id: ";
            std::cin >> id;
            std::cout << id << std::endl;
            std::cout << "password: ";
            std::cin >> password;
            std::cout << password << std::endl;
            
            jobj[u8"cmd"] = "new";
            jobj[u8"user"] = id;
            jobj[u8"passwd"] = password;
            
            rawdata_size = strlen( jobj.text().c_str() );   
            nsize = htonl( rawdata_size );    
            packet.resize( rawdata_size + sizeof(uint32_t) );
            ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
            ::memcpy( packet.data() + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
            
            flag = 0;
            
            // send id password to login ssl server
            octillion::SslClient::get_instance().write( 
                10, "127.0.0.1", "8888", packet.data(), 
                rawdata_size + sizeof(uint32_t) );           
        }
        else if ( choose == 2 )
        {
            std::error_code err;
            size_t rawdata_size;
            std::vector<uint8_t> packet;
            uint32_t nsize;
    
            octillion::JsonW jobj;
            
            std::string id, password;
            
            std::cout << "Login" << std::endl;
            std::cout << "id: ";
            std::cin >> id;
            std::cout << id << std::endl;
            g_username = id;
            std::cout << "password: ";
            std::cin >> password;
            std::cout << password << std::endl;
            
            jobj[u8"cmd"] = "login";
            jobj[u8"user"] = id;
            jobj[u8"passwd"] = password;
            
            rawdata_size = strlen( jobj.text().c_str() );   
            nsize = htonl( rawdata_size );    
            packet.resize( rawdata_size + sizeof(uint32_t) );
            ::memcpy( packet.data(), &nsize, sizeof(uint32_t) );
            ::memcpy( packet.data() + sizeof(uint32_t), jobj.text().c_str(), rawdata_size );
            
            flag = 0;
            
            octillion::SslClient::get_instance().write( 
                10, "127.0.0.1", "8888", packet.data(), 
                rawdata_size + sizeof(uint32_t) );    
        }
        else
        {
            return 0;
        }
    }        
        
    return 0;
}

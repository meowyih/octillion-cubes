#include <cstring>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

#include "error/ocerror.hpp"
#include "echoserver.hpp"
#include "error/macrolog.hpp"
#include "server/sslclient.hpp"

volatile sig_atomic_t flag = 0;

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
            
            char* strbuf = new char[datasize+1];
            strbuf[datasize] = 0;            
            ::memcpy((void*)strbuf, data, datasize );
            std::cout << "id:" << id << " client recv:" << strbuf << std::endl;
            delete [] strbuf;
            return 0; 
        }
};


int main ()
{    
    std::error_code err;
    
    flag = 0;
    
    std::cout << "client main start" << std::endl;

    signal(SIGINT, my_function);

    ClientCallback *callback = new ClientCallback();
    
    octillion::SslClient::get_instance().set_callback( callback );
    
    for ( int i = 0; i < 100; i++ )
    {
        octillion::SslClient::get_instance().write( i, "127.0.0.1", "8888", (uint8_t*)"1234567", 7 );    
    }
    
    while ( flag == 0 )
    {
    }
    
    std::cout << "client main stop" << std::endl;
    
    octillion::SslClient::get_instance().set_callback( NULL );
    delete callback;

    return 0;
}

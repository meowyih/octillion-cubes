
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

#include "error/ocerror.hpp"
#include "server/coreserver.hpp"
#include "world/world.hpp"
#include "server/rawprocessor.hpp"
#include "error/macrolog.hpp"

volatile sig_atomic_t flag = 0;

void my_function(int sig)
{
    std::cout << "stopping the server..." << std::endl;
    flag = 1;
    
}

int main ()
{    
    std::error_code err;
    
    LOG_I() << "main start";

    signal(SIGINT, my_function); 
    
    // don't forget to change iptables
    // iptables -A ufw-user-input -p tcp -m tcp --dport 8888 -j ACCEPT
    // iptables -A ufw-user-output -p tcp -m tcp --dport 8888 -j ACCEPT
    octillion::World::get_instance();
    octillion::CoreServer::get_instance();
    
    octillion::RawProcessor* rawprocessor = new octillion::RawProcessor();
    
    octillion::CoreServer::get_instance().set_callback( rawprocessor );
    
    err = octillion::CoreServer::get_instance().start( "8888", "cert/cert.key", "cert/cert.pem" );
        
    if ( err != OcError::E_SUCCESS )
    {
        std::cout << err << std::endl;
        return 1;
    }
    
    flag = 0;
    uint64_t lastms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
    while( 1 )
    {
        uint64_t nowms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
        
        if ( nowms - lastms > 1000 )
        {
            err = octillion::World::get_instance().tick(); 
            lastms = nowms;
        }

        if ( err == OcError::E_WORLD_FREEZED )
        {
            break;
        }
        
        if ( flag == 1 )
        {
            break;
        }
    }
    
    if ( octillion::CoreServer::get_instance().is_running() )
    {
        octillion::CoreServer::get_instance().stop();
    }

    delete rawprocessor;

    return 0;
}


#include <iostream>
#include <string>
#include <system_error>
#include <thread>

#include <signal.h>

#include "error/ocerror.hpp"
#include "server/coreserver.hpp"
#include "world/world.hpp"
#include "server/rawprocessor.hpp"
#include "server/coreserver_cb_sample.hpp"
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
    octillion::CoreServerCbSample* cscb = new octillion::CoreServerCbSample();
    
    octillion::CoreServer::get_instance().set_callback( cscb );
    
    err = octillion::CoreServer::get_instance().start( "8888", "cert/cert.key", "cert/cert.pem" );
        
    if ( err != OcError::E_SUCCESS )
    {
        std::cout << err << std::endl;
        return 1;
    }
    
    flag = 0;
    while( 1 )
    {
        if ( flag == 1 )
        {
            break;
        }
    }
    
    octillion::CoreServer::get_instance().stop();
    delete cscb;
    delete rawprocessor;

    return 0;
}

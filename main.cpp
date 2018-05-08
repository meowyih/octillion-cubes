
#include <iostream>
#include <string>
#include <system_error>
#include <thread>

#include <signal.h>

#include "ocerror.h"
#include "coreserver.h"
#include "rawprocessor.h"
#include "macrolog.h"

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
    octillion::CoreServer cs( "8888" );
    octillion::RawProcessor* rawprocessor = new octillion::RawProcessor();
    
    cs.set_callback( rawprocessor );
    
    err = cs.start();
        
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
    
    cs.stop();
    delete rawprocessor;

    return 0;
}
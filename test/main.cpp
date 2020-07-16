
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

#include "t001.hpp"
#include "t002.hpp"
#include "t003.hpp"
#include "error/ocerror.hpp"
#include "server/sslserver.hpp"
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
    
    err = octillion::SslServer::get_instance().start( "8888", "../cert/cert.key", "../cert/cert.pem" );
        
    if ( err != OcError::E_SUCCESS )
    {
        std::cout << err << std::endl;
        return 1;
    }
    
    // octillion::T001* test001 = new octillion::T001();    
    // octillion::SslServer::get_instance().set_callback( test001 );
    // test001->testSingleConn();
    // test001->testSingleConn();
    // test001->testSingleConn();
    
    // octillion::T002* test002 = new octillion::T002();    
    // octillion::SslServer::get_instance().set_callback( test002 );
    
    // test002->testMultipleConn();
    // test002->testMultipleConn();
    
    octillion::T003* test003 = new octillion::T003();    
    octillion::SslServer::get_instance().set_callback( test003 );
    // test003->test();
    
    flag = 0;
    uint64_t lastms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
    
    std::cout << "enter the loop" << std::endl;
    while( 1 )
    {
        uint64_t nowms = (std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch())).count();
        
        if ( nowms - lastms > 1000 )
        {
            lastms = nowms;
        }
        
        if ( flag == 1 )
        {
            std::cout << "break" << std::endl;
            break;
        }
    }
    std::cout << "leave the loop" << std::endl;
    
    if ( octillion::SslServer::get_instance().is_running() )
    {
        std::cout << "octillion::SslServer::get_instance().stop()" << std::endl;
        octillion::SslServer::get_instance().stop();
    }
    else
    {
        std::cout << "server is not running" << std::endl;
    }

    // delete test001;
    // delete test002;
    delete test003;

    return 0;
}

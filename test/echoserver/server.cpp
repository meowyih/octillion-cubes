
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"
#include "server/sslserver.hpp"

#include "echoserver.hpp"

volatile sig_atomic_t flag = 0;

void my_function(int sig)
{
    std::cout << "stopping the server..." << std::endl;
    flag = 1;
}

int main ()
{    
    std::error_code err;
    
    signal(SIGINT, my_function);
    
    octillion::EchoServer* echoserver = new octillion::EchoServer();
    
    octillion::SslServer::get_instance().set_callback( echoserver );
    err = octillion::SslServer::get_instance().start( "8888", "../../cert/cert.key", "../../cert/cert.pem" );
    
    while( flag == 0 )
    {
    }
    
    octillion::SslServer::get_instance().set_callback( NULL );
    octillion::SslServer::get_instance().stop();

    return 0;
}

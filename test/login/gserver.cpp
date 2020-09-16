
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"
#include "server/server.hpp"

#include "gameserver.hpp"

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
    
    octillion::GameServer* gameserver = new octillion::GameServer();
    
    octillion::Server::get_instance().set_callback( gameserver );
    err = octillion::Server::get_instance().start( "7000" );
    
    while( flag == 0 )
    {
    }
    
    octillion::Server::get_instance().set_callback( NULL );
    octillion::Server::get_instance().stop();
    
    delete gameserver;

    return 0;
}

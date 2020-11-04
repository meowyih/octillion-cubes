#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#include <signal.h>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"
#include "server/server.hpp"

#include "world/world.hpp"
#include "world/gameserver.hpp"

volatile sig_atomic_t world_end = 1;
volatile sig_atomic_t flag = 0;

void my_function(int sig)
{
    std::cout << "stopping the server..." << std::endl;
    flag = 1;
}

void start_world();

int main ()
{    
    std::error_code err;
    
    signal(SIGINT, my_function);
    
    // start the game server
    octillion::GameServer* gameserver = new octillion::GameServer();
    
    octillion::Server::get_instance().set_callback( gameserver );
    err = octillion::Server::get_instance().start( "7000" );
    
    // start the world thread    
    start_world();
    
    // wait until someone hit ctrl-c
    while( flag == 0 )
    {
    }
    
    // now we wait the end of the world  
    while( world_end == 0 )
    {
    }
    
    octillion::Server::get_instance().set_callback( NULL );
    octillion::Server::get_instance().stop();
        
    delete gameserver;

    return 0;
}

// a world that jump tick for every 500ms
void start_world()
{
    world_end = 0;
    
    std::chrono::high_resolution_clock::time_point last =
        std::chrono::high_resolution_clock::now();
    
    while( flag == 0 )
    {
        std::chrono::high_resolution_clock::time_point now =
            std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> diff = now - last;
        
        if ( diff.count() > 500 )
        {            
            octillion::World::get_instance().tick();
            last = now;
        }
    }
    
    world_end = 1;
}

#include <chrono>

#include "world/worldmap.hpp"
#include "world/world.hpp"
#include "world/event.hpp"

int main()
{
    int count = 0;
    
    std::chrono::high_resolution_clock::time_point last =
        std::chrono::high_resolution_clock::now();
        
    while( true )
    {
        octillion::Event event; 
        std::chrono::high_resolution_clock::time_point now =
            std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::milli> diff = now - last;
        
        if ( diff.count() > 500 )
        {            
            octillion::World::get_instance().tick();
            last = now;
            
            count++;

            // test
            switch( count )
            {
            case 1:
                event.type_ = octillion::Event::TYPE_PLAYER_CONNECT_WORLD;
                event.id_ = 1;
                octillion::World::get_instance().add_event( event );
                event.id_ = 2;
                octillion::World::get_instance().add_event( event );
                event.type_ = octillion::Event::TYPE_CMD_MOVE_Z_DEC;
                octillion::World::get_instance().add_event( event );
                break;
            case 2:
                event.type_ = octillion::Event::TYPE_CMD_MOVE_X_INC;
                event.id_ = 1;
                octillion::World::get_instance().add_event( event );
                event.id_ = 2;
                event.type_ = octillion::Event::TYPE_PLAYER_DISCONNECT_WORLD;
                octillion::World::get_instance().add_event( event );
                break;
            case 3:
                event.type_ = octillion::Event::TYPE_CMD_MOVE_Z_INC;
                event.id_ = 1;
                octillion::World::get_instance().add_event( event );
                break;
            }
        }
    }

    return 0;
}
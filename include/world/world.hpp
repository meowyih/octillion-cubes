#ifndef OCTILLION_WORLD_HEADER
#define OCTILLION_WORLD_HEADER

#include <string>
#include <map>
#include <mutex>

#include "world/worldmap.hpp"
#include "world/player.hpp"
#include "world/event.hpp"

#include "jsonw/jsonw.hpp"

namespace octillion
{
    class World;
}

class octillion::World
{
private:
    const std::string tag_ = "World";

public:
    //Singleton
    static World& get_instance()
    {
        static World instance;
        return instance;
    }

    // avoid accidentally copy
    World(World const&) = delete;
    void operator = (World const&) = delete;
    
public:
    // return true if World can handle this type of event
    bool valid_event( int type );
    
    // add an event into World's event queue
    bool add_event( const octillion::Event& event );

    // run one tick    
    void tick();
   
protected:
    // event handler
    void event_connect( const octillion::Event& event );
    void event_disconnect( const octillion::Event& event );
    void event_move( const octillion::Event& event );
    
    // send data back to gameserver with player's id
    void event_to_json( const octillion::Event& event, JsonW& json );    
    void send( const octillion::Event& event );
    void send( std::vector<octillion::Event>& events );
private:
    World();
    ~World();
    
private:
    octillion::WorldMap map_;
    std::queue<octillion::Event> equeue_;
    std::map<int_fast32_t,std::shared_ptr<octillion::Player>> players_;
    
private:
    std::mutex mutex_;
};

#endif

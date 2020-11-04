#include <queue>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/world.hpp"
#include "world/event.hpp"
#include "world/player.hpp"

#include "jsonw/jsonw.hpp"

#ifndef TEST_WORLD_WITH_NO_GAMESERVER
#include "world/gameserver.hpp"
#endif

octillion::World::World()
{
    if ( map_.load_external_data_file( "../../data/" ) == false )
    {
        LOG_E(tag_) << "failed to load map from json" << std::endl;
    }
}

octillion::World::~World()
{
}

void octillion::World::event_connect( const octillion::Event& event )
{
    octillion::Event pevent;
    pevent.id_ = event.id_;
    pevent.fd_ = event.fd_;
    
    LOG_D(tag_) << "user " << event.id_ << " connected";

    // load player data
    if ( players_.find( event.id_ ) != players_.end() )
    {
        // players is inside the game
        pevent.type_ = octillion::Event::TYPE_PLAYER_ERR_ALREADY_LOGIN;
        send( pevent );
        LOG_W(tag_) << "warning: connecting user " << event.id_ << " is already in the world";
        return;
    }
    
    // TODO: store and load data in permanent storage
    
    std::shared_ptr<octillion::Player> pc = std::make_shared<octillion::Player>();
    pc->loc_ = map_.reborn_;
    pc->id_  = event.id_;
    pc->fd_  = event.fd_;
    
    players_.insert( std::pair<int_fast32_t,std::shared_ptr<octillion::Player>>(event.id_, pc));
    
    pevent.type_ = octillion::Event::TYPE_PLAYER_LOGIN;
    pevent.player_ = pc;
    send( pevent );
}

void octillion::World::event_disconnect( const octillion::Event& event )
{
    LOG_D(tag_) << "user " << event.id_ << " disconnected";   
    
    if ( players_.find( event.id_ ) == players_.end() )
    {
        LOG_W(tag_) << "warning: disconnecting user " << event.id_ << " is not in the world";
        return;
    }
    
    players_.erase( event.id_ );
}

void octillion::World::event_move( const octillion::Event& event )
{
    octillion::Event pevent;    
    int dir = event.type_ - octillion::Event::TYPE_CMD_MOVE_OFFSET;
    std::shared_ptr<octillion::Cube> c_from = event.player_.lock()->loc_;
    std::shared_ptr<octillion::Cube> c_to = c_from->find( map_.cubes_, dir );
    
    pevent.id_ = event.id_;
    pevent.fd_ = event.fd_;
    
    if ( c_to == nullptr || c_from->exits_[dir] == 0 )
    {
        // it should not happen since client already did the checking
        pevent.type_ = octillion::Event::TYPE_ERROR_NO_EXIT;
        send( pevent );
        return;
    }
    
    event.player_.lock()->loc_ = c_to;
    
    LOG_D(tag_) << "event_move p" << event.id_ << " move to " << event.player_.lock()->loc_->title();
    
    // send new position to end-user
    pevent.type_ = octillion::Event::TYPE_PLAYER_LOCATION;
    pevent.player_ = event.player_;
    
    send( pevent );
}

// return true if World can handle such event
bool octillion::World::valid_event( int type )
{
    switch( type )
    {
        case octillion::Event::TYPE_PLAYER_CONNECT_WORLD:
        case octillion::Event::TYPE_PLAYER_DISCONNECT_WORLD:
        case octillion::Event::TYPE_CMD_MOVE_X_INC: 
        case octillion::Event::TYPE_CMD_MOVE_Y_INC: 
        case octillion::Event::TYPE_CMD_MOVE_Z_INC: 
        case octillion::Event::TYPE_CMD_MOVE_X_DEC:
        case octillion::Event::TYPE_CMD_MOVE_Y_DEC:
        case octillion::Event::TYPE_CMD_MOVE_Z_DEC:        
            return true;
        default:
            return false;
    }
}

bool octillion::World::add_event( const octillion::Event& event )
{
    mutex_.lock();    
    equeue_.push( event );    
    mutex_.unlock();
    
    return true;
}

void octillion::World::tick()
{   
    mutex_.lock();
    
    // pop the front event and handle it
    while( ! equeue_.empty() )
    {
        octillion::Event event = equeue_.front();
        equeue_.pop();
        
        // player's cmd event need to check player id first
        if ( event.id_ != 0 &&
             event.type_ != octillion::Event::TYPE_PLAYER_CONNECT_WORLD &&
             event.type_ != octillion::Event::TYPE_PLAYER_DISCONNECT_WORLD )
        {
            if ( players_.find( event.id_ ) == players_.end() )
            {
                // error, player id does not connected yet
                JsonW json;
                octillion::Event err( octillion::Event::TYPE_ERROR_PLAYER_LOST );
                event_to_json( event, json );
                
                LOG_E(tag_) << "error: user " << event.id_ << " is not in the world. cmd:" 
                    << event.type_ << " " << json.text();
                    
#ifndef TEST_WORLD_WITH_NO_GAMESERVER
                octillion::GameServer::sendpacket( event.fd_, json.text(), true );
#endif                
                continue;
            }
            else
            {
                // no need to check weak ptr status in the event handler
                event.player_ = players_.at( event.id_ );
            }
        }
        
        switch( event.type_ )
        {
            case octillion::Event::TYPE_PLAYER_CONNECT_WORLD:
                event_connect( event );
                break;
            case octillion::Event::TYPE_PLAYER_DISCONNECT_WORLD:
                event_disconnect( event );
                break;
            case octillion::Event::TYPE_CMD_MOVE_X_INC:
            case octillion::Event::TYPE_CMD_MOVE_Y_INC:
            case octillion::Event::TYPE_CMD_MOVE_Z_INC:
            case octillion::Event::TYPE_CMD_MOVE_X_DEC:
            case octillion::Event::TYPE_CMD_MOVE_Y_DEC:
            case octillion::Event::TYPE_CMD_MOVE_Z_DEC:
                event_move( event );
                break;
            default:
                LOG_W(tag_) << "unhandled event type " << event.type_;
        }
    }
    
    mutex_.unlock();
}

void octillion::World::event_to_json( const octillion::Event& event, JsonW& json )
{
    json["type"] = event.type_;
    
    if ( event.type_ == octillion::Event::TYPE_PLAYER_LOGIN ||
         event.type_ == octillion::Event::TYPE_PLAYER_LOCATION )
    {
        JsonW jloc;    
        
        jloc.add("x", (int)event.player_.lock()->loc_->loc().x());
        jloc.add("y", (int)event.player_.lock()->loc_->loc().y());
        jloc.add("z", (int)event.player_.lock()->loc_->loc().z());
        
        json["loc"] = jloc;
    }
    
}

// send data back to gameserver with player's id
void octillion::World::send( const octillion::Event& event )
{
    JsonW json;

    bool disconnect = false;
    if ( event.type_ == octillion::Event::TYPE_PLAYER_ERR_ALREADY_LOGIN )
    {
        disconnect = true;
    }
    
    event_to_json( event, json );

#ifndef TEST_WORLD_WITH_NO_GAMESERVER
    LOG_D(tag_) << "send id:" << event.id_ << " data:" << json.text();
    octillion::GameServer::sendpacket( event.fd_, json.text(), disconnect );
#endif
}

void octillion::World::send( std::vector<octillion::Event>& events )
{
#ifndef TEST_WORLD_WITH_NO_GAMESERVER 
    LOG_D(tag_) << "send " << events.size() << "event(s)";
#endif
}
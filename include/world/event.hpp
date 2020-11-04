#ifndef OCTILLION_EVENT_HEADER
#define OCTILLION_EVENT_HEADER

#include <memory>
#include <vector>
#include <string>

#include "jsonw/jsonw.hpp"
#include "world/player.hpp"

namespace octillion
{
    class Event;
}

class octillion::Event
{
public:
    const std::string tag_ = "Event";
public:
	const static int RANGE_NONE = 0;
	const static int RANGE_PRIVATE = 1;
	const static int RANGE_CUBE = 2;
	const static int RANGE_AREA = 3;
	const static int RANGE_WORLD = 4;

	const static int TYPE_UNKNOWN = 0;

    // login server and game server event
	const static int TYPE_PLAYER_CREATE = 1;
	const static int TYPE_PLAYER_LOGIN = 2;
    const static int TYPE_PLAYER_VERIFY_TOKEN = 3;
    const static int TYPE_SERVER_VERIFY_TOKEN = 4;
	const static int TYPE_PLAYER_LOGOUT = 5;
    
    const static int TYPE_PLAYER_ERR_ALREADY_LOGIN = 6;
    
    // game login/logout event
    const static int TYPE_PLAYER_CONNECT_WORLD = 8;
    const static int TYPE_PLAYER_DISCONNECT_WORLD = 9;
    
    // move event
	const static int TYPE_PLAYER_ARRIVE = 10;
	const static int TYPE_PLAYER_ARRIVE_PRIVATE = 11;
	const static int TYPE_PLAYER_LEAVE = 12;
	const static int TYPE_PLAYER_DEAD = 13;
	const static int TYPE_PLAYER_REBORN = 14;
	const static int TYPE_PLAYER_REBORN_PRIVATE = 15;
	const static int TYPE_PLAYER_ATTACK = 16;
    const static int TYPE_PLAYER_LOCATION = 17;

	const static int TYPE_MOB_REBORN = 20;
	const static int TYPE_MOB_DEAD = 21;
	const static int TYPE_MOB_ARRIVE = 22;
	const static int TYPE_MOB_LEAVE = 23;

	const static int TYPE_MOB_ATTACK = 24;
    
    // player move command
    const static int TYPE_CMD_MOVE_OFFSET = 100;
    const static int TYPE_CMD_MOVE_X_INC = TYPE_CMD_MOVE_OFFSET + octillion::Cube::X_INC;
    const static int TYPE_CMD_MOVE_Y_INC = TYPE_CMD_MOVE_OFFSET + octillion::Cube::Y_INC;
    const static int TYPE_CMD_MOVE_Z_INC = TYPE_CMD_MOVE_OFFSET + octillion::Cube::Z_INC;
    const static int TYPE_CMD_MOVE_X_DEC = TYPE_CMD_MOVE_OFFSET + octillion::Cube::X_DEC;
    const static int TYPE_CMD_MOVE_Y_DEC = TYPE_CMD_MOVE_OFFSET + octillion::Cube::Y_DEC;
    const static int TYPE_CMD_MOVE_Z_DEC = TYPE_CMD_MOVE_OFFSET + octillion::Cube::Z_DEC;
    
    // error
    const static int TYPE_ERROR_FATAL       = 900;
    const static int TYPE_ERROR_PLAYER_LOST = 901;
    const static int TYPE_ERROR_NO_EXIT     = 902;

	const static int TYPE_JSON_SIMPLE = 1;
	const static int TYPE_JSON_SIMPLE_WITH_LOC = 2;
	const static int TYPE_JSON_DETAIL = 3;
	const static int TYPE_JSON_DETAIL_WITH_LOC = 4;
    
public:
    // empty event
    Event();
    
    Event( int type ) { type_ = type; }
    
    // external event from network with fd and raw data
    Event( int fd, std::vector<uint8_t>& data ); 

    ~Event();
    
    bool is_valid() { return valid_; }

public:
    int type_ = TYPE_UNKNOWN;
    std::vector<std::string> strparms_;
    
    // for external event only
    bool valid_ = false;
    int  fd_ = 0;
    int  id_ = 0;
    
    std::weak_ptr<octillion::Player> player_;
};
#endif
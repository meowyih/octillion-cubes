#ifndef OCTILLION_COMMAND_HEADER
#define OCTILLION_COMMAND_HEADER

#include <string>
#include <vector>

#include "world/cube.hpp"
#include "jsonw/jsonw.hpp"
// [reserved pcid]
// input - fd
// output - available pcid
//
// [roll]
// input - fd, gender, class 
// output - attributes (con, men, luc, cha)
//
// [roll name]
// input - fd
// output - ascii name
//
// [confirm roll result (complete pc creation)]
// input - fd, nickname, password
// output - user information, cubes
// 
// [login]
// input - pcid, password
// output - user information, cubes

namespace octillion
{
    class Command;
}

class octillion::Command
{
private:
    const std::string tag_ = "Command";
    
public:
    // create player and login
    const static int UNKNOWN = 0;
    const static int CONNECT = 10;
    const static int DISCONNECT = 11;
    const static int VALIDATE_USERNAME = 13;
    const static int CONFIRM_USER = 17;
    const static int LOGIN = 19;
    const static int LOGOUT = 20;

	const static int GET_SERVER_VERSION = 900;
	const static int GET_GLOBAL_DATA_STAMP = 901;
	const static int GET_GLOBAL_DATA = 902;
	const static int GET_AREA_DATA = 903;

    // movement
    const static int MOVE_NORMAL = 40;

    // TODO: only admin can freeze the entire world
    const static int FREEZE_WORLD = 2999;

public:
    // command error
    const static int E_CMD_SUCCESS = 0;
    const static int E_CMD_UNKNOWN_COMMAND = 100;
    const static int E_CMD_BAD_FORMAT = 101;
    const static int E_CMD_TOO_COMMON_NAME = 102;
    const static int E_CMD_WRONG_USERNAME_PASSWORD = 103;
	const static int E_CMD_FILE_IO_ERROR = 104;
        
public:  
    Command( int fd, int cmd );
    Command( int fd, std::vector<uint8_t>& data );

    //destructor
    ~Command();
    
    int fd() { return fd_; }
    int cmd() { return cmd_; }
    bool valid() { return valid_; }
    
private:
    int fd_;
    int cmd_;    
    JsonW json_;

public:
    std::vector<std::string> strparms_;
    std::vector<uint_fast32_t> uiparms_;

    bool valid_;
};

#endif

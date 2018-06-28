#ifndef OCTILLION_COMMAND_HEADER
#define OCTILLION_COMMAND_HEADER

#include <string>

#include "world/cube.hpp"

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

#include <vector>

#include "jsonw/jsonw.hpp"

namespace octillion
{
    class Command;
}

class octillion::Command
{
private:
    const std::string tag_ = "Command";
    
public:
    // login
    const static int UNKNOWN = 0;
    const static int VALIDATE_USERNAME = 13;
    const static int CONFIRM_USER = 17;
    const static int LOGIN = 19;
    const static int LOGOUT = 20;

    // TODO: only admin can freeze the entire world
    const static int FREEZE_WORLD = 2999;

public:
    // command error
    const static int E_CMD_SUCCESS = 0;
    const static int E_CMD_UNKNOWN_COMMAND = 100;
    const static int E_CMD_BAD_FORMAT = 101;
    const static int E_CMD_TOO_COMMON_NAME = 102;
    const static int E_CMD_WRONG_USERNAME_PASSWORD = 103;
        
public:  
    // set pcid to 0 if unknown
    Command( uint32_t fd, uint8_t* data, size_t datasize );

    //destructor
    ~Command();
    
    uint32_t cmd() { return cmd_; }
    bool valid() { return valid_; }
    
private:
    uint32_t fd_;
    uint32_t cmd_;
    
    JsonTextW* json_ = NULL;

public:
    std::vector<std::string> strparms_;
    std::vector<int> uiparms_;

    bool valid_;
};

#endif

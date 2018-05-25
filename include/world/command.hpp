#ifndef OCTILLION_COMMAND_HEADER
#define OCTILLION_COMMAND_HEADER

#include <string>

#include "world/cube.hpp"

// [reserved pcid]
// input - fd
// output - available pcid
//
// [roll]
// input - pcid, nickname, password
// output - users information
//
// [confirm roll result (complete pc creation)]
// input - pcid
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
    // login
    const static uint32_t UNKNOWN = 0;
    const static uint32_t RESERVED_PCID = 10;
    const static uint32_t ROLL_CHARACTER = 11;
    const static uint32_t CONFIRM_CHARACTER = 12;
    const static uint32_t LOGIN = 13;
        
public:
    Command() {};
    Command( uint32_t pcid ) { pcid_ = pcid; valid_ = false; }
    Command( uint32_t pcid, uint32_t cmd ) { pcid_ = pcid; cmd_ = cmd; valid_ = false; }
    Command( uint32_t pcid, uint32_t cmd, CubePosition loc ) { pcid_ = pcid; cmd_ = cmd; loc_ = loc; valid_ = false; }
    
    // set pcid to 0 if unknown
    Command( uint32_t pcid, uint8_t* data, size_t datasize );
    
    uint32_t cmd() { return cmd_; }
    bool valid() { return valid_; }

public:  
    static size_t format( uint8_t* buf, size_t buflen, uint32_t cmd, uint32_t argv = 0 );
    
public:
    uint32_t pcid_;
    uint32_t cmd_;
    CubePosition loc_;
    
private:
    bool valid_;
};

#endif

#ifndef OCTILLION_COMMAND_HEADER
#define OCTILLION_COMMAND_HEADER

#include <string>

#include "world/cube.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

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
    const static int CONNECT = 10;
    const static int DISCONNECT = 11;
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
    Command( uint32_t fd, uint32_t cmd );
    Command( uint32_t fd, uint8_t* data, size_t datasize );

    //destructor
    ~Command();
    
    int fd() { return fd_; }
    uint32_t cmd() { return cmd_; }
    bool valid() { return valid_; }
    
private:
    int fd_;
    uint32_t cmd_;    
    JsonTextW* json_ = NULL;

public:
    std::vector<std::string> strparms_;
    std::vector<int> uiparms_;

    bool valid_;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

        static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

#endif

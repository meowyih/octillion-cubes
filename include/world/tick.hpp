#ifndef OCTILLION_TICK_HEADER
#define OCTILLION_TICK_HEADER

#include <cstdint>
#include <system_error>

// Tick is a object that can keep itself 'state' and
// go next state by member function 'tick()'
namespace octillion
{
    class Tick;
    class TickCallback;
}

class octillion::TickCallback
{
public:
    virtual void tickcallback( 
        uint32_t type, 
        uint32_t param1,
        uint32_t param2 ) = 0;
};

// Tick definition
class octillion::Tick
{
public:
    // pure virtual function that can "tick()"
    virtual std::error_code tick() = 0;
};


#endif


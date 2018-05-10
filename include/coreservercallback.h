
#ifndef OCTILLION_CORE_SERVER_CALLBACK_HEADER
#define OCTILLION_CORE_SERVER_CALLBACK_HEADER

#include <cstdint>

namespace octillion
{
    class CoreServerCallback;
}

class octillion::CoreServerCallback
{
    public:
        ~CoreServerCallback() {}
        
    public:
        // pure virtual function that handlers all incoming event
        virtual void connect( int fd ) = 0;
        virtual void recv( int fd, uint8_t* data, size_t datasize) = 0;
        virtual void disconnect( int fd ) = 0;
};

#endif // OCTILLION_CORE_SERVER_CALLBACK_HEADER
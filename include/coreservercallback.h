
#ifndef OCTILLION_CORE_SERVER_CALLBACK_HEADER
#define OCTILLION_CORE_SERVER_CALLBACK_HEADER

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
        virtual void recv( int fd, char* data, int datasize) = 0;
        virtual void disconnect( int fd ) = 0;
};

#endif // OCTILLION_CORE_SERVER_CALLBACK_HEADER
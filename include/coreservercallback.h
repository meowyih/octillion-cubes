
#ifndef CORE_SERVER_CALLBACK_HEADER
#define CORE_SERVER_CALLBACK_HEADER

class CoreServerCallback
{
    public:
        ~CoreServerCallback() {}
        
    public:
        // pure virtual function that handlers all incoming event
        virtual void connect( int fd ) = 0;
        virtual void recv( int fd, char* data, int datasize) = 0;
        virtual void disconnect( int fd ) = 0;
};

#endif // CORE_SERVER_CALLBACK_HEADER
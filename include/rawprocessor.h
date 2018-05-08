

#ifndef RAW_PROCESSOR_HEADER
#define RAW_PROCESSOR_HEADER

#include "coreservercallback.h"

class RawProcessor : public CoreServerCallback
{
    public:
        RawProcessor();
        ~RawProcessor();
        
    public:
        // virtual function from CoreServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual void recv( int fd, char* data, int datasize) override;
        virtual void disconnect( int fd ) override;
        
    private:
        void encrypt( unsigned char* data, int datasize, unsigned char* key, int keysize );
        void decrypt( unsigned char* data, int datasize, unsigned char* key, int keysize );
};

#endif // RAW_PROCESSOR_HEADER
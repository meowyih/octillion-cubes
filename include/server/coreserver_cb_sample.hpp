#ifndef OCTILLION_CORESERVER_CB_SAMPLE_HEADER
#define OCTILLION_CORESERVER_CB_SAMPLE_HEADER

#include <map>
#include <cstdint>
#include <string>

#include "server/coreserver.hpp"

namespace octillion
{
    class CoreServerCbSample;    
}

class octillion::CoreServerCbSample : public octillion::CoreServerCallback
{
    private:
        const std::string tag_ = "CoreServerCbSample";

    public:
        CoreServerCbSample();
        ~CoreServerCbSample();
        
    public:
        // virtual function from CoreServerCallback that handlers all incoming events
        virtual void connect( int fd ) override;
        virtual int recv( int fd, uint8_t* data, size_t datasize) override;
        virtual void disconnect( int fd ) override;        
};

#endif
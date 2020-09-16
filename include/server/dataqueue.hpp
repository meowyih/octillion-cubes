#ifndef OCTILLION_DATA_QUEUE_HEADER
#define OCTILLION_DATA_QUEUE_HEADER

#include <cstring>
#include <list>
#include <map>
#include <system_error>

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class DataQueue;
}

class octillion::DataQueue
{
    private:
        const std::string tag_ = "DataQueue";

    public:
        DataQueue();
        ~DataQueue();
        
    public:
        size_t size();        
        size_t peek();
        std::error_code pop( int& fd, uint8_t* buf, size_t buflen );
        std::error_code feed( int fd, uint8_t* buf, size_t buflen );
        
        void remove( int fd );
        
    private:        
        // caller needs to make sure buflen > sizeof uint32
        uint32_t read_uint32( uint8_t* buf );

    private:      
        struct DataBlock
        {
            int    fd;
            uint8_t* data;
            size_t datasize;
        };  
        std::list<DataBlock> queue_;
        std::map<int,DataBlock> workspace_;
};

#endif //OCTILLION_DATA_QUEUE_HEADER
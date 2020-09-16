#include <netinet/in.h>

#include "server/dataqueue.hpp"
#include "error/macrolog.hpp"
#include "error/ocerror.hpp"

#include "t004.hpp"

const std::string octillion::T004::tag_ = "T004";

octillion::T004::T004()
{
    uint8_t* anchor;
    uint32_t size_1 = 100, size_2 = 50, tmp;
    LOG_D(tag_) << "T004()";
    data_ = NULL;
    
    datasize_ = sizeof( uint32_t ) + size_1 + sizeof( uint32_t ) + size_2;
    data_ = new uint8_t[ datasize_ + 1 ];
    anchor = data_;
    
    tmp = htonl( size_1 );
    memcpy((void*) anchor, (void*) &tmp, sizeof( uint32_t ));
    anchor = anchor + sizeof( uint32_t );
    
    for ( int i = 0; i < size_1; i ++ )
    {
        *anchor = (uint8_t)(i%0xFF);
        anchor++;
    }
    
    tmp = htonl( size_2 );
    memcpy((void*) anchor, (void*) &tmp, sizeof( uint32_t ));
    anchor = anchor + sizeof( uint32_t );
    
    for ( int i = 0; i < size_2; i ++ )
    {
        *anchor = (uint8_t)(i%0xFF);
        if ( i != size_2 - 1 )
            anchor++;
    }
}

octillion::T004::~T004()
{
    LOG_D(tag_) << "~T004()";
    if ( data_ != NULL )
    {
        delete [] data_;
    }
}

void octillion::T004::test() 
{
    LOG_I(tag_) << "queue size:" << octillion::DataQueue::get_instance().size();
  
    // test 1 - feed 1 byte at a time
    for ( int i = 0; i < datasize_; i ++ )
    {
        octillion::DataQueue::get_instance().feed( 1, data_ + i, 1 );
    }
    
    while( octillion::DataQueue::get_instance().size() > 0 )
    {
        int fd;
        uint8_t *buf;
        size_t next_size;
        next_size = octillion::DataQueue::get_instance().peek();
        LOG_I(tag_) << "item size:" << next_size;
        buf = new uint8_t[next_size];
        octillion::DataQueue::get_instance().pop( fd, buf, next_size );
        
        if ( ! valid( buf, next_size ))
        {
            LOG_E(tag_) << "Failed: data validation return false";
            break;
        }
        
        delete [] buf;
        LOG_I(tag_) << "queue size:" << octillion::DataQueue::get_instance().size();
    }
   
    // test 2 - feed all at once
    octillion::DataQueue::get_instance().feed( 1, data_, datasize_ );
    
    while( octillion::DataQueue::get_instance().size() > 0 )
    {
        int fd;
        uint8_t *buf;
        size_t next_size;
        next_size = octillion::DataQueue::get_instance().peek();
        LOG_I(tag_) << "item size:" << next_size;
        buf = new uint8_t[next_size];
        octillion::DataQueue::get_instance().pop( fd, buf, next_size );
        
        if ( ! valid( buf, next_size ))
        {
            LOG_E(tag_) << "Failed: data validation return false";
            break;
        }
        delete [] buf;
        LOG_I(tag_) << "queue size:" << octillion::DataQueue::get_instance().size();
    }
}

bool octillion::T004::valid( uint8_t* data, size_t datasize )
{
    for ( int i = 0; i < datasize; i ++ )
    {
        if ( data[i] != (uint8_t)(i%0xFF) )
        {
            return false;
        }
    }
    
    return true;
}
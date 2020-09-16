#include <queue>
#include <netinet/in.h>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/dataqueue.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

octillion::DataQueue::DataQueue()
{   
    LOG_D(tag_) << "DataQueue()";
}

octillion::DataQueue::~DataQueue()
{   
    LOG_D(tag_) << "~DataQueue()";
    
    for ( auto it = queue_.begin(); it != queue_.end(); it++ )
    {
        delete [] (*it).data;
    }
        
    for (auto it = workspace_.begin(); it != workspace_.end(); it++ ) 
    {
        delete [] (*it).second.data;
    }
}

size_t octillion::DataQueue::size()
{
    return queue_.size();
}

size_t octillion::DataQueue::peek()
{
    if ( queue_.empty() )
        return 0;
    
    return queue_.front().datasize;
}

std::error_code octillion::DataQueue::pop( int& fd, uint8_t* buf, size_t buflen )
{
    if ( buflen < queue_.front().datasize )
        return OcError::E_FATAL;
    
    std::memcpy( buf, queue_.front().data, queue_.front().datasize );
    fd = queue_.front().fd;
    delete [] (queue_.front().data);
    queue_.pop_front();
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::DataQueue::feed( int fd, uint8_t* buf, size_t buflen )
{
    DataBlock block;
    std::map<int, DataBlock>::iterator iter = workspace_.find( fd );
    
    if ( iter == workspace_.end() )
    {
        // fd does not exist in workspace, create block
        block.fd = fd;
        block.datasize = buflen;
        block.data = new uint8_t[block.datasize];
        std::memcpy( (void*)block.data, buf, buflen );
    }
    else
    {       
        // fd data exists, concatenate old and new data
        block.fd = fd;
        block.datasize = iter->second.datasize + buflen;
        block.data = new uint8_t[block.datasize];
        std::memcpy( (void*)block.data, 
            iter->second.data, 
            iter->second.datasize );
        std::memcpy( (void*)(block.data + iter->second.datasize ),
            (void*) buf, buflen );
            
        // delete the old block in workspace
        delete [] (iter->second).data;
        workspace_.erase( iter );        
    }
    
    // process block.data
    while( true )
    {
        uint32_t datasize;
        DataBlock dataqueue_block;
        
        if ( block.datasize < sizeof( uint32_t ))
        {
            // not enough data in block,
            // insert the new block in workspace and return
            workspace_.insert(std::pair<int, DataBlock>(fd, block));
            return OcError::E_SUCCESS;
        }
        
        datasize = read_uint32( block.data );
       
        // datasize range checking
        if ( datasize == 0 )
        {
            LOG_E(tag_) << "recv invalid datasize: " << datasize;
            delete [] block.data;
            return OcError::E_FATAL;
        }
        
        if ( block.datasize < ( datasize + sizeof( uint32_t )))
        {
            // not enough data in block,
            // insert the new block in workspace and return
            workspace_.insert(std::pair<int, DataBlock>(fd, block));
            return OcError::E_SUCCESS;        
        }
        
        // we got enough data for dataqueue, extract it
        dataqueue_block.fd = fd;
        dataqueue_block.data = new uint8_t[datasize];
        dataqueue_block.datasize = datasize;
        std::memcpy( 
            (void*) dataqueue_block.data, 
            (void*) (block.data + sizeof( uint32_t )),
            datasize );
            
        queue_.push_back(dataqueue_block);
        
        if ( block.datasize == ( datasize + sizeof( uint32_t )))
        {
            delete [] block.data;
            return OcError::E_SUCCESS;
        }
        else
        {
            size_t newdatasize = 
                block.datasize - 
                ( dataqueue_block.datasize + sizeof( uint32_t ));
                
            uint8_t* newdata = new uint8_t[newdatasize];
            
            std::memcpy(
                (void*) newdata,
                (void*) ( block.data + 
                        ( dataqueue_block.datasize + sizeof( uint32_t ))),
                newdatasize );
                
            delete [] block.data;
            block.data = newdata;
            block.datasize = newdatasize;
        }
    }
}

uint32_t octillion::DataQueue::read_uint32( uint8_t* buf )
{
    uint32_t netlong;
    std::memcpy( (void*)&netlong, buf, sizeof( uint32_t ));    
    return ntohl( netlong );
}

void octillion::DataQueue::remove( int fd )
{
    for ( auto it = queue_.begin(); it != queue_.end(); it++ )
    {
        if ( (*it).fd == fd )
        {
            delete [] (*it).data;
            it = queue_.erase( it );
        }
    }
    
    workspace_.erase( fd );
}
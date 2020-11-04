#include <queue>
#include <netinet/in.h>
#include <vector>
#include <algorithm> // for copy() and assign() 
#include <iterator> // for back_inserter 

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/dataqueue.hpp"

octillion::DataQueue::DataQueue()
{   
    LOG_D(tag_) << "DataQueue()";
}

octillion::DataQueue::~DataQueue()
{   
    LOG_D(tag_) << "~DataQueue()";
    
    for ( auto it = queue_.begin(); it != queue_.end(); it++ )
    {
        (*it).dataptr.reset();
    }
        
    for (auto it = workspace_.begin(); it != workspace_.end(); it++ ) 
    {
        (*it).second.dataptr.reset();
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
    
    return queue_.front().dataptr.get()->size();
}

std::error_code octillion::DataQueue::pop( int& fd, uint8_t* buf, size_t buflen )
{
    if ( buflen < queue_.front().dataptr.get()->size() )
        return OcError::E_FATAL;
    
    std::memcpy( buf, queue_.front().dataptr.get()->data(), queue_.front().dataptr.get()->size() );
    fd = queue_.front().fd;
    queue_.front().dataptr.reset();
    queue_.pop_front();
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::DataQueue::pop( int& fd, std::vector<uint8_t>& buf )
{
    buf.clear();
    
    std::copy( queue_.front().dataptr.get()->begin(), 
               queue_.front().dataptr.get()->end(), 
               back_inserter( buf ) ); 
    
    fd = queue_.front().fd;
    queue_.front().dataptr.reset();
    queue_.pop_front();
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::DataQueue::feed( int fd, std::vector<uint8_t>& buf )
{
    return feed( fd, buf.data(), buf.size() );
}

std::error_code octillion::DataQueue::feed( int fd, uint8_t* buf, size_t buflen )
{
    DataBlock block;
    std::map<int, DataBlock>::iterator iter = workspace_.find( fd );
    
    if ( iter == workspace_.end() )
    {
        // fd does not exist in workspace, create block
        block.fd = fd;
        block.dataptr = std::make_shared<std::vector<uint8_t>>( buflen );
        std::memcpy( (void*) block.dataptr.get()->data(), buf, buflen );
    }
    else
    {       
        // fd data exists, concatenate old and new data
        block.fd = fd;
        
        block.dataptr = 
            std::make_shared<std::vector<uint8_t>>( iter->second.dataptr.get()->size() + buflen );
        std::memcpy( (void*)block.dataptr.get()->data(), 
            iter->second.dataptr.get()->data(), 
            iter->second.dataptr.get()->size() );
        std::memcpy( (void*)(block.dataptr.get()->data() + iter->second.dataptr.get()->size() ),
            (void*) buf, buflen );
                    
        // delete the old block in workspace
        (iter->second).dataptr.reset();
        workspace_.erase( iter );        
    }
    
    // process block.data
    while( true )
    {
        uint32_t datasize;
        DataBlock dataqueue_block;
        
        if ( block.dataptr.get()->size() < sizeof( uint32_t ))
        {
            // not enough data in block,
            // insert the new block in workspace and return
            workspace_.insert(std::pair<int, DataBlock>(fd, block));
            return OcError::E_SUCCESS;
        }
        
        datasize = read_uint32( block.dataptr.get()->data() );
       
        // datasize range checking
        if ( datasize == 0 )
        {
            LOG_E(tag_) << "recv invalid datasize: " << datasize;
            block.dataptr.reset();
            return OcError::E_FATAL;
        }
        
        if ( block.dataptr.get()->size() < ( datasize + sizeof( uint32_t )))
        {
            // not enough data in block,
            // insert the new block in workspace and return
            workspace_.insert(std::pair<int, DataBlock>(fd, block));
            return OcError::E_SUCCESS;        
        }
        
        // we got enough data for dataqueue, extract it
        dataqueue_block.fd = fd;
        dataqueue_block.dataptr = std::make_shared<std::vector<uint8_t>>( datasize );
        std::memcpy( 
            (void*) dataqueue_block.dataptr.get()->data(), 
            (void*) (block.dataptr.get()->data() + sizeof( uint32_t )),
            datasize );
            
        queue_.push_back(dataqueue_block);
        
        if ( block.dataptr.get()->size() == ( datasize + sizeof( uint32_t )))
        {
            block.dataptr.reset();
            return OcError::E_SUCCESS;
        }
        else
        {
            size_t newdatasize =
                block.dataptr.get()->size() -
                ( dataqueue_block.dataptr.get()->size() + sizeof( uint32_t ));
                
            std::shared_ptr<std::vector<uint8_t>> new_dataptr =
                std::make_shared<std::vector<uint8_t>>( newdatasize );
                
            std::memcpy(
                (void*) new_dataptr.get()->data(),
                (void*) ( block.dataptr.get()->data() + 
                        ( dataqueue_block.dataptr.get()->size() + sizeof( uint32_t ))),
                newdatasize );
            
            block.dataptr.reset();
            block.dataptr = new_dataptr;
        }
    }
}

uint32_t octillion::DataQueue::read_uint32( uint8_t* buf )
{
    uint32_t netlong;
    std::memcpy( (void*)&netlong, buf, sizeof( uint32_t ));    
    return ntohl( netlong );
}

uint32_t octillion::DataQueue::write_uint32( uint32_t size )
{
    return htonl( size );
}

void octillion::DataQueue::remove( int fd )
{
    for ( auto it = queue_.begin(); it != queue_.end(); it++ )
    {
        if ( (*it).fd == fd )
        {
            (*it).dataptr.reset();;
            it = queue_.erase( it );
        }
    }
    
    workspace_.erase( fd );
}
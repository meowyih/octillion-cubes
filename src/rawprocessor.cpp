#include <cstdint>
#include <cstring>
#include <map>

#include <netinet/in.h>

#include "coreserver.h"
#include "rawprocessor.h"
#include "macrolog.h"
#include "ocerror.h"

octillion::RawProcessorClient::RawProcessorClient()
{
    LOG_D(tag_) << "RawProcessorClient()";
    headersize_ = 0;
    keysize_ = 0;
    datasize_ = 0;
    dataanchor_ = 0;
    data_ = NULL;
}

octillion::RawProcessorClient::~RawProcessorClient()
{    
    if ( data_ != NULL )
    {
        LOG_D() << "~RawProcessorClient(), fd_:" << fd_ << ", delete data_";
        delete [] data_;
    }
    else
    {
        LOG_D() << "~RawProcessorClient(), fd_:" << fd_;
    }
}

octillion::RawProcessor::RawProcessor()
{
    LOG_D(tag_) << "RawProcessor()";
}

octillion::RawProcessor::~RawProcessor()
{
    LOG_D(tag_) << "~RawProcessor()";    
    clients_.clear();
}

void octillion::RawProcessor::connect( int fd )
{
    // clients_ automatically creates element if key fd does not exist 
    if ( clients_[fd].data_ != NULL )
    {
        LOG_D() << "RawProcessor::connect fd:" << fd << ", delete buffer and reset element in map";
        delete [] clients_[fd].data_;
    }
    else
    {
        LOG_D() << "RawProcessor::connect fd:" << fd << ", reset element map";
    }
    
    clients_[fd].fd_ = fd;
    clients_[fd].headersize_ = 0;
    clients_[fd].keysize_ = 0;
    clients_[fd].datasize_ = 0;
    clients_[fd].dataanchor_ = 0;
    clients_[fd].data_ = NULL;
}

size_t octillion::RawProcessor::readheader( int fd, uint8_t* data, size_t datasize, size_t anchor )
{
    size_t read = 0;
        
    while(( anchor + read ) < datasize )
    {
        if ( clients_[fd].headersize_ == RawProcessorClient::kRawProcessorHeaderSize )
        {                      
            // convert to key_ and keysize_ if header_ contains enough data
            std::memcpy( (void*) &clients_[fd].datasize_, (void*) clients_[fd].header_, 
                RawProcessorClient::kRawProcessorHeaderSize );
                
            clients_[fd].datasize_ = ntohl( clients_[fd].datasize_ );
            
            if ( clients_[fd].datasize_ > kRawProcessorMaxDataChunkSize )
            {
                // something bad happened, client declare chunk size > constraint
                LOG_E() << "fd:" << fd << " has bad datasize " << clients_[fd].datasize_;
                
                clients_[fd].headersize_ = 0;
                CoreServer::get_instance().closesocket( fd );
                
                // no need to handle other data
                return datasize + 1;
            }

            // prepare the data chunk
            LOG_D() << "Create chunk buffer for fd: " << fd << " chunk size: " << clients_[fd].datasize_;
            clients_[fd].data_ = new uint8_t[ clients_[fd].datasize_ ];
            clients_[fd].dataanchor_ = 0;
            break;
        }
        else
        {
            // read next byte into header_
            clients_[fd].header_[ clients_[fd].headersize_++ ] = data[ anchor + read ]; 
            read ++;
        }                  
    }
    
    return read;
}

size_t octillion::RawProcessor::readdata( int fd, uint8_t* data, size_t datasize, size_t anchor )
{
    size_t read = 0;
    size_t remain;
    
    // calculate the remain space for clients_[fd].data_
    remain = clients_[fd].datasize_ - clients_[fd].dataanchor_;
    
    // data_ is full
    if ( remain == 0 )
    {
        return read;
    }
    
    // copy data into clients_[fd].data_
    if ( remain >= datasize - anchor )
    {
        std::memcpy( (void*) ( clients_[fd].data_ + clients_[fd].dataanchor_ ), 
                     (void*) ( data + anchor ), 
                             remain );
                             
        clients_[fd].dataanchor_ += remain;
        return remain;
    }
    else
    {
        std::memcpy( (void*) ( clients_[fd].data_ + clients_[fd].dataanchor_ ), 
                     (void*) ( data + anchor ), 
                            datasize - anchor);
                            
        clients_[fd].dataanchor_ += ( datasize - anchor );
        return datasize - anchor;
    }
}

void octillion::RawProcessor::recv( int fd, uint8_t* data, size_t datasize )
{
    size_t anchor = 0;
    size_t sendsize;
    
    while ( anchor < datasize )
    {
        size_t read;
        
        read = readheader( fd, data, datasize, anchor );
        anchor = anchor + read;
        
        if ( anchor == datasize )
        {
            break;
        }
        
        read = readdata( fd, data, datasize, anchor );
        anchor = anchor + read;
        
        if ( clients_[fd].datasize_ == clients_[fd].dataanchor_ )
        {
            // read whole data, notify upper level and clean up 
            std::string str( (const char*)clients_[fd].data_, clients_[fd].datasize_ ); 
            LOG_D() << "RawProcessor::recv chunk size:" << clients_[fd].datasize_ 
                    << " data:" << str;
            
            clients_[fd].headersize_ = 0;
            clients_[fd].keysize_ = 0;
            clients_[fd].datasize_ = 0;
            clients_[fd].dataanchor_ = 0;
            
            if ( clients_[fd].data_ == NULL )
            {
                LOG_D() << "RawProcessor::recv delete fd:" << fd << " data_ buffer";
                delete [] clients_[fd].data_;
                clients_[fd].data_ = NULL;
            }            
        }
    }    

#if 0    
    decrypt( (uint8_t*)data, datasize, (uint8_t*)testkey, 7 );
    
    std::string str( data, datasize );    
    LOG_D() << "RawProcessor::connect recv fd: " << fd << " size:" << datasize << " data:" << str;
    
    encrypt( (unsigned char*)data, datasize, (unsigned char*)testkey, 7 );
    std::error_code error = CoreServer::get_instance().senddata( fd, data, datasize, sendsize );
       
    
    if ( error != OcError::E_SUCCESS )
    {
        LOG_E() << "Failed to send data, return " << error << "sendsize:" << sendsize;
    }
#endif
}

void octillion::RawProcessor::disconnect( int fd )
{
    LOG_D() << "disconnect fd:" << fd;
    
    // check fd validation, only for safety
    if ( clients_.find( fd ) == clients_.end() )
    {
        LOG_W() << "RawProcessor::disconnect, fd:" << fd << " does not exist";
    }
    else
    {
        LOG_W() << "RawProcessor::disconnect, clean up fd:" << fd << " buffer";
        if ( clients_[fd].data_ == NULL )
        {
            delete [] clients_[fd].data_;
            clients_[fd].data_ = NULL;
        }
        
        clients_.erase( fd );
    }
    
}

void octillion::RawProcessor::encrypt( uint8_t* data, size_t datasize, uint8_t* key, size_t keysize )
{
    for ( int i = 0; i < datasize; i ++ )
    {
        data[i] = data[i] ^ key[i%keysize];
    }
}

void octillion::RawProcessor::decrypt( uint8_t* data, size_t datasize, uint8_t* key, size_t keysize )
{
    for ( int i = 0; i < datasize; i ++ )
    {
        data[i] = data[i] ^ key[i%keysize];
    }    
}
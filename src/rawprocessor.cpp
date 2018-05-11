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
    if ( clients_.find( fd ) != clients_.end() )
    {
        clients_.erase( fd );
    }
    
    clients_[fd].fd_ = fd;
}

std::error_code octillion::RawProcessor::senddata( int fd, uint8_t* data, size_t datasize )
{
    uint32_t header = htonl( datasize );    
    uint8_t key[RawProcessorClient::kRawProcessorMaxKeyPoolSize];
    size_t keysize = (datasize % ( RawProcessorClient::kRawProcessorMaxKeyPoolSize - 1 )) + 1;
    uint8_t* buffer = new uint8_t[datasize];
    std::error_code error;
        
    memcpy( (void*) buffer, (const void*) data, datasize );

    for ( int i = 0; i < keysize; i ++ )
    {
        key[i] = kRawProcessorKeyPool[ (datasize + i) % kRawProcessorKeyPoolSize];
    }
    
    encrypt( buffer, datasize, key, keysize );
       
    error = CoreServer::get_instance().senddata( fd, (const void*)&header, sizeof(uint32_t) );

    if ( error != OcError::E_SUCCESS )
    {
        LOG_E( tag_ ) << "senddata, failed to send data, fd:" << fd << " datasize:" << datasize << " error:" << error;
        // close socket
        CoreServer::get_instance().closesocket( fd );
        
        // notify itself
        disconnect( fd );
    }
    else 
    {
        error = CoreServer::get_instance().senddata( fd, (const void*)buffer, datasize );
        
        if ( error != OcError::E_SUCCESS )
        {
            LOG_E( tag_ ) << "senddata, failed to send data, fd:" << fd << " datasize:" << datasize << " error:" << error;
            
            // close socket
            CoreServer::get_instance().closesocket( fd );
            
            // notify itself
            disconnect( fd );
        }
    }

    delete [] buffer;
}

size_t octillion::RawProcessor::readheader( int fd, uint8_t* data, size_t datasize, size_t anchor )
{
    size_t read = 0;
    
    LOG_D(tag_) << "readheader datasize:" << datasize << 
        " anchor:" << anchor << 
        " clients_[fd].headersize_:" << clients_[fd].headersize_;
    
    // no need to read the header again if header is ready
    if ( clients_[fd].headersize_ == RawProcessorClient::kRawProcessorHeaderSize )
    {
        LOG_D(tag_) << "readheader, header is already there, skip reading";
        return read;        
    }
        
    while(( anchor + read ) < datasize )
    {
        if ( clients_[fd].headersize_ < RawProcessorClient::kRawProcessorHeaderSize )
        {
            // read next byte into header_
            clients_[fd].header_[ clients_[fd].headersize_++ ] = data[ anchor + read ]; 
            read ++;
        }
        
        // if the data in header_ is enough, convert it to headersize_
        if ( clients_[fd].headersize_ == RawProcessorClient::kRawProcessorHeaderSize )
        {                      
            // convert to key_ and keysize_ if header_ contains enough data
            std::memcpy( (void*) &clients_[fd].datasize_, (void*) clients_[fd].header_, 
                RawProcessorClient::kRawProcessorHeaderSize );
                
            clients_[fd].datasize_ = ntohl( clients_[fd].datasize_ );
            
            if ( clients_[fd].datasize_ > kRawProcessorMaxDataChunkSize )
            {
                // something bad happened, client declare chunk size > constraint
                LOG_E(tag_) << "readheader fd:" << fd << " has bad datasize " << clients_[fd].datasize_;
                
                clients_[fd].headersize_ = 0;
                CoreServer::get_instance().closesocket( fd );
                
                // no need to handle other data
                return datasize + 1;
            }
            else if ( clients_[fd].datasize_ == 0 )
            {
                // no need buffer and key since data size is 0
                break;
            }
            
            // generate the encrypt/decrypt key
            clients_[fd].keysize_ = 
                ( clients_[fd].datasize_ % ( RawProcessorClient::kRawProcessorMaxKeyPoolSize - 1 )) + 1;     
            for ( int i = 0; i < clients_[fd].keysize_; i ++ )
            {
                clients_[fd].key_[i] = kRawProcessorKeyPool[ (clients_[fd].datasize_ + i) % kRawProcessorKeyPoolSize];
            }
                
            // prepare the data buffer
            LOG_D(tag_) << "readheader new fd:" << fd << " data_ buffer datasize_:" << clients_[fd].datasize_;
            clients_[fd].data_ = new uint8_t[ clients_[fd].datasize_ ];
            clients_[fd].dataanchor_ = 0;
            break;
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
    
    LOG_D(tag_) << "readdata datasize:" << datasize << 
        " anchor:" << anchor << 
        " clients_[fd].datasize_:" << clients_[fd].datasize_ << 
        " clients_[fd].dataanchor_:" << clients_[fd].dataanchor_ << 
        " remain:" << remain;
    
    // data_ is full
    if ( remain == 0 )
    {
        return read;
    }
    
    // copy data into clients_[fd].data_
    if ( remain <= datasize - anchor )
    {
        std::memcpy( (void*) ( clients_[fd].data_ + clients_[fd].dataanchor_ ), 
                     (void*) ( data + anchor ), 
                             remain );
                                                        
        clients_[fd].dataanchor_ += remain;
        read = remain;
    }
    else
    {
        std::memcpy( (void*) ( clients_[fd].data_ + clients_[fd].dataanchor_ ), 
                     (void*) ( data + anchor ), 
                            datasize - anchor);
                            
        clients_[fd].dataanchor_ += ( datasize - anchor );
        LOG_D(tag_) << "read data ret:" << datasize - anchor << " clients_[fd].dataanchor_:" << clients_[fd].dataanchor_;
        read = datasize - anchor;
    }
    
    if ( clients_[fd].datasize_ == clients_[fd].dataanchor_ )
    {
        // decrypt the data
        decrypt( clients_[fd].data_, clients_[fd].datasize_,
            clients_[fd].key_, clients_[fd].keysize_ );
        
        // read whole data, notify upper level and clean up 
        std::string str( (const char*)clients_[fd].data_, clients_[fd].datasize_ );     
        LOG_D() << "RawProcessor::recv chunk size:" << clients_[fd].datasize_ << " data_:" << str;
            
        // TODO: notify the upper later
          
        // release resource          
        clients_.erase(fd);       
    }
    
    return read;
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
        
        if ( anchor >= datasize )
        {
            break;
        }
        
        read = readdata( fd, data, datasize, anchor );
        anchor = anchor + read;
    }
    
    senddata( fd, (uint8_t*)"ok", (size_t)2 );
}

void octillion::RawProcessor::disconnect( int fd )
{    
    // check fd validation, only for safety
    if ( clients_.find( fd ) == clients_.end() )
    {
        LOG_W() << "RawProcessor::disconnect, fd:" << fd << " does not exist";
    }
    else
    {
        LOG_D() << "RawProcessor::disconnect, fd:" << fd;
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
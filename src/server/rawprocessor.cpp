#include <cstdint>
#include <cstring>
#include <map>
#include <cstdint>
#include <string>

#ifdef WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "server/coreserver.hpp"
#include "server/rawprocessor.hpp"
#include "error/macrolog.hpp"
#include "error/ocerror.hpp"

#include "world/world.hpp"
#include "world/command.hpp"

const std::string octillion::RawProcessor::tag_ = "RawProcessor";

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
        LOG_D(tag_) << "~RawProcessorClient(), fd_:" << fd_ << ", delete data_";        
        delete [] data_;
    }
    else
    {
        LOG_D(tag_) << "~RawProcessorClient(), fd_:" << fd_;
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
        World::get_instance().disconnect( fd );
    }
    
    clients_[fd].fd_ = fd;
    World::get_instance().connect( fd );
}

std::error_code octillion::RawProcessor::senddata( int fd, uint8_t* data, size_t datasize )
{
    uint32_t header = htonl( datasize );
    uint8_t key[RawProcessorClient::kRawProcessorMaxKeyPoolSize];
    size_t keysize = (datasize % ( RawProcessorClient::kRawProcessorMaxKeyPoolSize - 1 )) + 1;
    uint8_t* buffer = new uint8_t[sizeof(uint32_t) + datasize];
    std::error_code error;
    
    memcpy( (void*) buffer, (const void*)&header, sizeof(uint32_t));
    memcpy( (void*) ( buffer + sizeof( uint32_t )), 
            (const void*) data, datasize );

    for ( size_t i = 0; i < keysize; i ++ )
    {
        key[i] = kRawProcessorKeyPool[ (datasize + i) % kRawProcessorKeyPoolSize];
    }
    
    encrypt( (uint8_t*)(buffer + sizeof( uint32_t )), datasize, key, keysize );
    
    LOG_D(tag_) << "senddata, fd:" << fd << " size:" << sizeof(uint32_t) + datasize;   
    error = CoreServer::get_instance().senddata( fd, (const void*)buffer, sizeof( uint32_t ) + datasize );

    delete [] buffer;
    
    if ( error != OcError::E_SUCCESS )
    {
        LOG_E( tag_ ) << "senddata, failed to send data, fd:" << fd << " datasize:" << datasize << " error:" << error;
    }

    return error;
}

std::error_code octillion::RawProcessor::closefd(int fd)
{
    CoreServer::get_instance().requestclosefd(fd);
    return OcError::E_SUCCESS;
}

int octillion::RawProcessor::readheader( int fd, uint8_t* data, size_t datasize, size_t anchor )
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

                // error occurred
                return -1;
            }
            else if ( clients_[fd].datasize_ == 0 )
            {
                // no need buffer and key since data size is 0
                break;
            }
            
            // generate the encrypt/decrypt key
            clients_[fd].keysize_ = 
                ( clients_[fd].datasize_ % ( RawProcessorClient::kRawProcessorMaxKeyPoolSize - 1 )) + 1;     
            for ( size_t i = 0; i < clients_[fd].keysize_; i ++ )
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

int octillion::RawProcessor::readdata( int fd, uint8_t* data, size_t datasize, size_t anchor )
{
    int read = 0;
    int remain;
    
    // calculate the remain space for clients_[fd].data_
    remain = (int)(clients_[fd].datasize_ - clients_[fd].dataanchor_);
    
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
    if ( remain <= (int)(datasize - anchor) )
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
            
        // transfer data to Command and send to the World
        Command* cmd = new Command( fd, clients_[fd].data_, clients_[fd].datasize_ );
        if ( cmd->valid() )
        {
            LOG_D(tag_) << "readdata, add cmd to World";
            World::get_instance().addcmd( fd, cmd );
        }
        else
        {
            LOG_D(tag_) << "readdata, invalid cmd, ignore and close fd";
            delete cmd;
            read = -1; // bad command, return -1 to close the connection
        }
          
        // release resource          
        clients_.erase(fd);
    }
    
    LOG_D( tag_ ) << "RawProcessor::readdata return " << read;
    
    return read;
}

int octillion::RawProcessor::recv( int fd, uint8_t* data, size_t datasize )
{
    size_t anchor = 0;
    
    LOG_D( tag_ ) << "recv enter";
    
    while ( anchor < datasize )
    {
        int read;
        
        read = readheader( fd, data, datasize, anchor );
        
        if ( read < 0 )
        {
            // error, notify server to close fd
            LOG_W(tag_) << "recv, failed to readhead, return 0";
            return 0;
        }
        
        anchor = anchor + read;
        
        if ( anchor >= datasize )
        {
            break;
        }
        
        read = readdata( fd, data, datasize, anchor );
        
        if ( read < 0 )
        {
            // error, notify server to close fd
            LOG_W(tag_) << "recv, failed to readdata, return 0";
            return 0;            
        }
        
        anchor = anchor + read;
    }

    LOG_D(tag_) << "recv, leave successfully, return 1";
    
    return 1;
}

void octillion::RawProcessor::disconnect( int fd )
{    
    World::get_instance().disconnect( fd );
    
    // check fd validation, only for safety
    if ( clients_.find( fd ) == clients_.end() )
    {
        LOG_D(tag_) << "RawProcessor::disconnect, fd:" << fd << " does not has data that need to be read";
    }
    else
    {
        LOG_D(tag_) << "RawProcessor::disconnect, fd:" << fd;
        clients_.erase( fd );        
    }
}

void octillion::RawProcessor::encrypt( uint8_t* data, size_t datasize, uint8_t* key, size_t keysize )
{
    for ( size_t i = 0; i < datasize; i ++ )
    {
        data[i] = data[i] ^ key[i%keysize];
    }
}

void octillion::RawProcessor::decrypt( uint8_t* data, size_t datasize, uint8_t* key, size_t keysize )
{
    for ( size_t i = 0; i < datasize; i ++ )
    {
        data[i] = data[i] ^ key[i%keysize];
    }    
}


#include <string>
#include <cstring>

#include <arpa/inet.h>

#include "error/macrolog.hpp"
#include "world/command.hpp"
#include "database/database.hpp"

octillion::Command::Command( uint32_t pcid, uint8_t* data, size_t datasize )
{
    uint8_t* buf = NULL;
    size_t size;
    
    LOG_D( tag_ ) << "constructor, datasize:" << datasize;
    pcid_ = pcid;
    valid_ = false;
    cmd_ = UNKNOWN;
    
    // minimum command requires uint32_t data at the beginning
    if ( datasize < sizeof( uint32_t) )
    {
        LOG_E( tag_ ) << "constructor, size too small";
        return;
    }
    
    memcpy( (void*)&cmd_, (const void*)data, sizeof( uint32_t ));
    cmd_ = ntohl( cmd_ );
    
    switch( cmd_ )
    {
    case RESERVED_PCID: 
        valid_ = true;
        LOG_D( tag_ ) << "constructor, cmd:RESERVED_PCID";
        return; // no more data
    }
}

size_t octillion::Command::format( uint8_t* buf, size_t buflen, uint32_t cmd, uint32_t argv )
{
    size_t size;
    uint32_t nlheader;
    uint32_t nlcmd;
    uint32_t nlargv;
    
    // return the required buf size if buf address is NULL
    switch( cmd )
    {
    case UNKNOWN:
        size = sizeof(uint32_t); // cmd
        break;
    case RESERVED_PCID:
        size = sizeof(uint32_t) * 2; // cmd + pcid
        break;
    default:
        size = 0;
    }
    
    if ( buf == NULL )
    {
        return size;
    }
    
    nlcmd = htonl( cmd );
    nlargv = htonl( argv );
    
    // copy the data into prepared buffer
    switch( cmd )
    {
    case RESERVED_PCID:
        memcpy( (void*) buf, 
                (const void*) &nlcmd, 
                sizeof( uint32_t ));
        memcpy( (void*)( buf + sizeof( uint32_t )), 
                (const void*) &nlargv, 
                sizeof( uint32_t ));
        return size;
    case UNKNOWN:

        memcpy( (void*) buf, 
                (const void*) &nlcmd, 
                sizeof( uint32_t ));
        return size;    
    default:
        return size;
    }
}

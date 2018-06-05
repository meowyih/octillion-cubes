
#include <string>
#include <cstring>

// ntohl / htonl 
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "error/macrolog.hpp"
#include "world/command.hpp"
#include "database/database.hpp"

octillion::Command::Command( uint32_t pcid, uint8_t* data, size_t datasize )
{
    uint8_t* buf = data;
    size_t minsize = sizeof(uint32_t); // data should at least have 1 cmd
    uint32_t uint32parm;
    
    LOG_D( tag_ ) << "constructor, datasize:" << datasize;
    pcid_ = pcid;
    valid_ = false;
    cmd_ = UNKNOWN;

    switch (cmd_)
    {
    case RESERVED_PCID: // cmd
        minsize = sizeof(uint32_t);
        break;
    case ROLL_CHARACTER: // cmd, class, gender
        minsize = sizeof(uint32_t) + 2 * sizeof(uint32_t);
        break;
    }
    
    // minimum command requires uint32_t data at the beginning
    if ( datasize < minsize)
    {
        LOG_E( tag_ ) << "constructor, size too small";
        return;
    }
    
    memcpy( (void*)&cmd_, (const void*)buf, sizeof( uint32_t ));
    buf = buf + sizeof(uint32_t);
    cmd_ = ntohl( cmd_ );
    
    switch( cmd_ )
    {
    case RESERVED_PCID: 
        return; // no more data
    case ROLL_CHARACTER:

        memcpy((void*)&uint32parm, (const void*)buf, sizeof(uint32_t)); // 1st parm
        uint32parms_.push_back(uint32parm);
        buf = buf + sizeof(uint32_t);

        memcpy((void*)&uint32parm, (const void*)buf, sizeof(uint32_t)); // 2nd parm
        uint32parms_.push_back(uint32parm);
        buf = buf + sizeof(uint32_t);

        return;
    }
}

size_t octillion::Command::format( uint8_t* buf, size_t buflen, uint32_t cmd, const std::vector<uint32_t>& uin32parms)
{
    uint8_t* anchor = buf;
    size_t size;
    uint32_t nlcmd;

    // check if argument is correct
    switch (cmd)
    {
    case UNKNOWN:
        break;
    case RESERVED_PCID:
        if (uin32parms.size() != 1) return 0;
        break;
    case ROLL_CHARACTER:
        if (uin32parms.size() != 4) return 0;
        break;
    default:
        return 0;
    }

    // return the required buf size if buf address is NULL
    // cmd + argv
    size = sizeof(uint32_t) + sizeof(uint32_t) * uin32parms.size();
    
    if ( buf == NULL || buflen == 0 )
    {
        return size;
    }
    
    // copy cmd into prepared buffer
    nlcmd = htonl( cmd );
    memcpy((void*)anchor, (const void*)&nlcmd, sizeof(uint32_t));
    anchor = anchor + sizeof(uint32_t);

    // copy data into prepared buffer
    for (size_t i = 0; i < uin32parms.size(); i++)
    {
        uint32_t nlparm = htonl(uin32parms[i]);
        memcpy((void*)anchor, (const void*)&nlparm, sizeof(uint32_t));
        anchor = anchor + sizeof(uint32_t);
    }
    
    return size;
}

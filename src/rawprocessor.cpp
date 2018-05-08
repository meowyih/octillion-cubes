
#include "rawprocessor.h"
#include "macrolog.h"

RawProcessor::RawProcessor()
{
}

RawProcessor::~RawProcessor()
{
    
}

void RawProcessor::connect( int fd )
{
    LOG_D() << "RawProcessor::connect fd:" << fd;
}

void RawProcessor::recv( int fd, char* data, int datasize)
{
    std::string str( data, datasize );    
    LOG_D() << "RawProcessor::connect recv fd: " << fd << " size:" << datasize << " data:" << str;
}

void RawProcessor::disconnect( int fd )
{
    LOG_D() << "disconnect fd:" << fd;
}

void RawProcessor::encrypt( unsigned char* data, int datasize, unsigned char* key, int keysize )
{
    
}

void RawProcessor::decrypt( unsigned char* data, int datasize, unsigned char* key, int keysize )
{
    
}
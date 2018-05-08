
#include "rawprocessor.h"
#include "macrolog.h"

octillion::RawProcessor::RawProcessor()
{
}

octillion::RawProcessor::~RawProcessor()
{
    
}

void octillion::RawProcessor::connect( int fd )
{
    LOG_D() << "RawProcessor::connect fd:" << fd;
}

void octillion::RawProcessor::recv( int fd, char* data, int datasize)
{
    std::string str( data, datasize );    
    LOG_D() << "RawProcessor::connect recv fd: " << fd << " size:" << datasize << " data:" << str;
}

void octillion::RawProcessor::disconnect( int fd )
{
    LOG_D() << "disconnect fd:" << fd;
}

void octillion::RawProcessor::encrypt( unsigned char* data, int datasize, unsigned char* key, int keysize )
{
    
}

void octillion::RawProcessor::decrypt( unsigned char* data, int datasize, unsigned char* key, int keysize )
{
    
}
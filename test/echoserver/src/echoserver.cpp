#include <system_error>
#include <string>

#include "echoserver.hpp"

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"
#include "server/sslserver.hpp"

int octillion::EchoServer::recv( int fd, uint8_t* data, size_t datasize)
{
    LOG_D(tag_) << "recv " << fd << " " << datasize << " bytes";
    
    std::error_code err = 
        octillion::SslServer::get_instance().senddata( fd, data, datasize, true );
        
    if ( err == OcError::E_SUCCESS )
        return 1;
    
    return 0;
}

void octillion::EchoServer::disconnect( int fd )
{
    LOG_D(tag_) << "disconnect " << fd;
}

void octillion::EchoServer::connect( int fd )
{
    LOG_D(tag_) << "connect " << fd;
}

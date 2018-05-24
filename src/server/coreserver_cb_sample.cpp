#include "error/macrolog.hpp"
#include "error/ocerror.hpp"
#include "server/coreserver.hpp"
#include "server/coreserver_cb_sample.hpp"
#include "world/world.hpp"

octillion::CoreServerCbSample::CoreServerCbSample()
{
    LOG_D(tag_) << "CoreServerCbSample()";
}

octillion::CoreServerCbSample::~CoreServerCbSample()
{
    LOG_D(tag_) << "~CoreServerCbSample()";    
}

void octillion::CoreServerCbSample::connect( int fd )
{
    LOG_D(tag_) << "connect() fd:" << fd;
    World::get_instance().login( fd );
}

int octillion::CoreServerCbSample::recv( int fd, uint8_t* data, size_t datasize)
{
    LOG_D(tag_) << "recv() fd:" << fd << " data[0]:" << (char)data[0] << " datasize:" << datasize;
    // CoreServer::get_instance().senddata( fd, "1234", 4 );
    
    switch( data[0] )
    {
        case 'N': World::get_instance().move( fd, CubePosition::NORTH ); break;
        case 'E': World::get_instance().move( fd, CubePosition::EAST ); break;
        case 'W': World::get_instance().move( fd, CubePosition::WEST ); break;
        case 'S': World::get_instance().move( fd, CubePosition::SOUTH ); break;
        case 'U': World::get_instance().move( fd, CubePosition::UP ); break;
        case 'D': World::get_instance().move( fd, CubePosition::DOWN ); break;
    }
    
    return 1;
}

void octillion::CoreServerCbSample::disconnect( int fd )
{
    LOG_D(tag_) << "disconnect() fd:" << fd;
    World::get_instance().logout( fd );
}
#include <string>
#include <iostream>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/world.hpp"
#include "world/cube.hpp"

#include "server/rawprocessor.hpp"

#include "database/database.hpp"

octillion::World::World()
{
    Cube* cube0 = new Cube(CubePosition(), this);

    CubePosition loc1(1000, 1000, 1000);
    Cube* cube1 = new Cube(loc1, this);

    CubePosition loc2(1001, 1000, 1000);
    Cube* cube2 = new Cube(loc2, this);

    cube1->setexit(*cube2, Cube::exitval());
    cube2->setexit(*cube1, Cube::exitval());
    
    cubes_.insert(std::pair<CubePosition, Cube*>(CubePosition(), cube0));
    cubes_.insert( std::pair<CubePosition, Cube*>(loc1, cube1));
    cubes_.insert( std::pair<CubePosition, Cube*>(loc2, cube2));
#if 0
    for (auto& it : cubes_)
    {
        std::cout << (it.first).x_axis_ << "," << (it.first).y_axis_ << "," << (it.first).z_axis_;
        std::cout << ":" << ((it.second)->location().str());

        uint8_t exit;

        if ((exit = it.second->getexit(CubePosition::NORTH)) > 0)
        {
            std::cout << "exit to NORTH:" << (int)exit << std::endl;
        }

        if ((exit = it.second->getexit(CubePosition::EAST)) > 0)
        {
            std::cout << "exit to EAST:" << (int)exit << std::endl;
        }

        if ((exit = it.second->getexit(CubePosition::WEST)) > 0)
        {
            std::cout << "exit to WEST:" << (int)exit << std::endl;
        }

        if ((exit = it.second->getexit(CubePosition::SOUTH)) > 0)
        {
            std::cout << "exit to SOUTH:" << (int)exit << std::endl;
        }

        if ((exit = it.second->getexit(CubePosition::UP)) > 0)
        {
            std::cout << "exit to UP:" << (int)exit << std::endl;
        }

        if ((exit = it.second->getexit(CubePosition::DOWN)) > 0)
        {
            std::cout << "exit to DOWN:" << (int)exit << std::endl;
        }

        std::cout << std::endl;
    }
#endif    
}

octillion::World::~World()
{
    for (auto& it : cubes_)
    {
        delete it.second;
    }

    for (auto& it : pcs_)
    {
        delete it.second;
    }

    for (auto& it : cmds_)
    {
        delete it.second;
    }
}

std::error_code octillion::World::login(int pcid)
{
    if (pcs_.find(pcid) != pcs_.end())
    {
        return OcError::E_FATAL;
    }

    Player* pc = new Player(pcid);
    pc->move(CubePosition(1000, 1000, 1000));
    pcs_[pcid] = pc;

    return OcError::E_SUCCESS;
}

std::error_code octillion::World::logout(int pcid)
{
    std::map<uint32_t, Player*>::iterator it = pcs_.find(pcid);
    if ( it == pcs_.end())
    {
        return OcError::E_FATAL;
    }
    else
    {
        delete it->second;
        pcs_.erase(pcid);

        return OcError::E_SUCCESS;
    }    
}

//TODO: rewrite this function
std::error_code octillion::World::move(int pcid, const CubePosition & loc)
{
    std::map<uint32_t, Player*>::iterator it = pcs_.find(pcid);
    if ( it == pcs_.end())
    {
        LOG_E(tag_) << "move, cannot find pcid:" << pcid << " in pcs_ map";
        return OcError::E_FATAL;
    }
    else
    {
        it->second->move(loc);
        LOG_I(tag_) << "move, done";
        
        std::string locstr = CubePosition(loc).str();
        // CoreServer::get_instance().senddata( pcid, locstr.c_str(), locstr.length() );
        
        return OcError::E_SUCCESS;        
    }
}

//TODO: rewrite this function
std::error_code octillion::World::move( int pcid, CubePosition::Direction dir )
{
    // get player
    Player* pc;
    std::map<uint32_t, Player*>::iterator pcsit = pcs_.find(pcid);
    if ( pcsit == pcs_.end())
    {
        LOG_E(tag_) << "move, cannot find pcid:" << pcid << " in pcs_ map";
        return OcError::E_FATAL;
    }
    else
    {
        pc = pcsit->second;
    }
    
    // get cube
    Cube* cube;
    std::map<CubePosition, Cube*>::iterator cubesit = cubes_.find(pc->position());
    if ( cubesit == cubes_.end())
    {
        LOG_E(tag_) << "move, cannot find pos:" << pc->position().str() << " in cubes_ map";
        return OcError::E_FATAL;
    }
    else
    {
        cube = cubesit->second;
    }

    CubePosition dest( pc->position(), dir );
    
    if ( cube->getexit( dir ) == 0 )
    {
        // no exit in that direction
        LOG_I(tag_) << "move, no exist";
        return OcError::E_SUCCESS;
    }
    else
    {
        return move( pcid, dest );
    }    
}

void octillion::World::tick()
{
    uint8_t* buf = NULL;
    size_t size;
    uint32_t pcid;
    int fd;
    
    // data that need to send to pc after this tick
    std::map<int, Data> out;

    // handle commands in cmds_
    cmds_lock_.lock();

    for (auto& it : cmds_)
    {
        fd = it.first;
        Command* cmd = it.second;   
        bool success = true; 
        Data data;        
        
        switch( cmd->cmd() )
        {
        case Command::RESERVED_PCID:
            LOG_D( tag_ ) << "handle RESERVED_PCID in tick";
            pcid = Database::get_instance().reservedpcid();
            size = Command::format( NULL, 0, cmd->cmd(), pcid );
            if ( size > 0 )
            {
                buf = new uint8_t[size];
                Command::format( buf, size, cmd->cmd(), pcid );
                data.data = buf;
                data.datasize = size;
                out[fd] = data;
            }
            else
            {
                // error handling
                success = false;
            }
            break;
        default: // undefined commands
            success = false;
            break;
        }
        
        // create an UNKNOWN command to player
        if ( ! success )
        {
            size = Command::format( NULL, 0, Command::UNKNOWN );
            if ( size > 0 )
            {
                buf = new uint8_t[size];
                Command::format( NULL, 0, Command::UNKNOWN );
                data.data = buf;
                data.datasize = size;
                out[fd] = data;
            }
            else
            {
                // fatal error
                LOG_E( tag_ ) << "failed to create error command for fd:" << fd << " cmd:" << cmd->cmd();
            }
        }
    }

    // clear cmds_
    for (auto& it : cmds_)
    {
        delete it.second;
    }

    cmds_.clear();
    cmds_lock_.unlock();

    // send data to each pc via RawProcessor
    for (auto& it : out)
    {
        uint32_t fd = it.first;
        Data data = it.second;
        
        RawProcessor::get_instance().senddata( fd, data.data, data.datasize );

        delete [] data.data;
    }
    
    out.clear();
}

void octillion::World::tickcallback(uint32_t type, uint32_t param1, uint32_t param2)
{
}

void octillion::World::addcmd(int fd, Command * cmd)
{
    cmds_lock_.lock();

    if (cmds_.find(fd) == cmds_.end())
    {
        cmds_[fd] = cmd;
    }
    else
    {
        LOG_W( tag_ ) << "warning: addcmd() found dulicate fd " << fd << " ignore it";
    }

    cmds_lock_.unlock();
}

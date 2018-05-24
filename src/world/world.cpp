
#include <string>
#include <iostream>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/world.hpp"
#include "world/cube.hpp"

#include "server/coreserver.hpp"

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
#if 1
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
        CoreServer::get_instance().senddata( pcid, locstr.c_str(), locstr.length() );
        
        return OcError::E_SUCCESS;        
    }
}

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
    // data that need to send to pc after this tick
    std::map<uint32_t, CubePosition> out;

    // handle commands in cmds_
    cmds_lock_.lock();

    for (auto& it : cmds_)
    {
        Command* cmd = it.second;
        uint32_t pcid = cmd->pcid_;
        CubePosition loc = cmd->loc_;
        std::map<uint32_t, Player*>::iterator pit;

        if ((pit = pcs_.find(pcid)) == pcs_.end())
        {
            break;
        }

        Player* pc = pit->second;
        pc->move(loc);

        // for test, we send back user's location
        out[pcid] = loc;
    }

    // clear cmds_
    for (auto& it : cmds_)
    {
        delete it.second;
    }

    cmds_.clear();

    cmds_lock_.unlock();

    // TODO: send data to each pc via CoreServer
    for (auto& it : out)
    {
        uint32_t id = it.first;
        CubePosition loc = it.second;
        
        std::string locstr = loc.str();
        CoreServer::get_instance().senddata( id, locstr.c_str(), locstr.length() );
    }
}

void octillion::World::tickcallback(uint32_t type, uint32_t param1, uint32_t param2)
{
}

void octillion::World::addcmd(Command * cmd)
{
    cmds_lock_.lock();

    if (cmds_.find(cmd->pcid_) == cmds_.end())
    {
    }
    else
    {
        cmds_[cmd->pcid_] = cmd;
    }

    cmds_lock_.unlock();
}

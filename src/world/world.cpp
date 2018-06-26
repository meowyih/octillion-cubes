#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/world.hpp"
#include "world/cube.hpp"

#include "server/rawprocessor.hpp"
#include "database/database.hpp"

#include "jsonw/jsonw.hpp"

octillion::World::World()
{
    LOG_D(tag_) << "Constructor";

    // init database
    database_.init(std::string("save"));

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
    LOG_D(tag_) << "Destructor";
    for (auto& it : cubes_)
    {
        delete it.second;
    }

    for (auto& it : players_)
    {
        database_.save(it.second);
        delete it.second;
    }

    for (auto& it : cmds_)
    {
        delete it.second;
    }
}

std::error_code octillion::World::connect(int fd)
{
    auto it = players_.find( fd );
    
    if ( it == players_.end())
    {
        LOG_D( tag_ ) << "connect() fd:" << fd;
        players_[fd] = NULL;
        return OcError::E_SUCCESS;
    }
    else
    {
        // something bad happens in TCP layer, same fd connect twice
        LOG_W( tag_ ) << "connect() has detect duplicate fd " << fd;
        return OcError::E_FATAL;
    }
}

std::error_code octillion::World::disconnect(int fd)
{
    auto it = players_.find( fd );
    
    if ( it == players_.end())
    {
        LOG_W( tag_ ) << "disconnect() fd:" << fd << " does not exist in players_";
        return OcError::E_FATAL;
    }
    
    Player* player = it->second;
    
    if ( player == NULL )
    {
        players_.erase( it );
        LOG_I( tag_ ) << "disconnect() fd:" << fd << " has no login record";
        return OcError::E_SUCCESS;
    }
    else 
    {
    
        LOG_I( tag_ ) << "disconnect() fd:" << fd << " save data";
        database_.save( player );
        delete player;
        players_.erase( it );
        
        return OcError::E_SUCCESS;
    }
}

std::error_code octillion::World::move(int pcid, const CubePosition & loc)
{
    /*
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

        RawProcessor::senddata( pcid, (uint8_t*)locstr.c_str(), locstr.length());
     
        return OcError::E_SUCCESS;        
    }
    */
    
    return OcError::E_SUCCESS;        
}

std::error_code octillion::World::move( int pcid, CubePosition::Direction dir )
{
    /*
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
*/
    return OcError::E_SUCCESS;        
}

void octillion::World::tick()
{    
    // data that need to send to pc after this tick
    std::map<int, JsonObjectW*> cmdbacks;
    
    LOG_D(tag_) << "tick, cmds_.size:" << cmds_.size();

    // handle commands in cmds_
    cmds_lock_.lock();

    for (auto& it : cmds_)
    {
        int fd = it.first;
        Command* cmd = it.second;
        std::error_code err = OcError::E_SUCCESS;
        JsonObjectW* cmdback = new JsonObjectW();

        if (cmd == NULL || !cmd->valid())
        {
            cmdback->add(u8"err", Command::E_CMD_BAD_FORMAT);
        }
        else // valid cmd
        {
            switch (cmd->cmd())
            {
            case Command::VALIDATE_USERNAME:
                err = cmdValidateUsername(fd, cmd, cmdback);
                break;
            case Command::CONFIRM_USER:
                err = cmdConfirmUser(fd, cmd, cmdback);
                break;
            default: // undefined commands
                err = cmdUnknown(fd, cmd, cmdback);
                break;
            }
        }

        if (err == OcError::E_SUCCESS)
        {
            // insert data to output data map
            cmdbacks[fd] = cmdback;
        }
        else
        {
            // fatal error, close fd
            RawProcessor::closefd(fd);
            LOG_E(tag_) << "failed to handle incoming command cmd:" << cmd->cmd();
        }
    }

    // all cmd were transfer to cmdbacks, clear cmds_
    for (auto& it : cmds_)
    {
        delete it.second;
    }

    cmds_.clear();
    cmds_lock_.unlock();

    // send data to each pc via RawProcessor to CoreClient
    for (auto& it : cmdbacks)
    {
        uint32_t fd = it.first;
        JsonObjectW* cmdback = it.second;
        JsonObjectW* containerobj = new JsonObjectW();
        containerobj->add(u8"cmd", cmdback);
        JsonTextW* jsontext = new JsonTextW(containerobj);
        std::string utf8 = jsontext->string();

        RawProcessor::senddata(fd, (uint8_t*)utf8.data(), utf8.size() );

        delete jsontext;
    }

    LOG_D(tag_) << "tick, leave";
}

std::error_code octillion::World::cmdUnknown(int fd, Command *cmd, JsonObjectW* jsonobject)
{
    LOG_D(tag_) << "cmdUnknown";
    jsonobject->add(u8"err", Command::E_CMD_UNKNOWN_COMMAND );
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdValidateUsername(int fd, Command *cmd, JsonObjectW* jsonobject)
{
    LOG_D(tag_) << "cmdValidateUsername";
    std::string username, validname;
    std::error_code err;

    auto it = players_.find(fd);

    if (it == players_.end())
    {
        LOG_E(tag_) << "cmdValidateUsername() fd:" << fd << " does not exist in players_";
        return OcError::E_FATAL;
    }

    Player* player = it->second;

    if (player != NULL)
    {
        LOG_E(tag_) << "cmdValidateUsername() fd:" << fd << " has player data.";
        return OcError::E_FATAL;
    }

    username = cmd->strparms_[0];

    err = database_.reserve(fd, username);

    if (err == OcError::E_SUCCESS && username.length() > 6 )
    {
        // valid username
        validname = username;
    }
    else
    {
        // invalid username, provide alternative name
        int appendix = 100;
        std::string altname;
        do
        {
            if (username.length() < 6)
            {
                altname = username + username + username + std::to_string(appendix);
            }
            else
            {
                altname = username + std::to_string(appendix);
            }            
            appendix++;
            err = database_.reserve(fd, altname);

            // don't want to wast time in this
            if (appendix > 999)
            {
                jsonobject->add(u8"cmd", Command::VALIDATE_USERNAME);
                jsonobject->add(u8"err", Command::E_CMD_TOO_COMMON_NAME);
                return OcError::E_SUCCESS;
            }

        } while (err != OcError::E_SUCCESS);

        validname = altname;
    }

    jsonobject->add(u8"cmd", Command::VALIDATE_USERNAME);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
    jsonobject->add(u8"s1", validname);

    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdConfirmUser(int fd, Command *cmd, JsonObjectW* jsonobject)
{
    std::error_code err;
    std::string username = cmd->strparms_[0];
    std::string password = cmd->strparms_[1];
    int gender = cmd->uiparms_[0];
    int cls = cmd->uiparms_[1];

    Player* player = new Player();

    player->username(username);
    player->password( database_.hashpassword(password) );
    player->gender(gender);
    player->cls(cls);

    err = database_.create(fd, player);

    if (err == OcError::E_SUCCESS)
    {
        jsonobject->add(u8"cmd", Command::CONFIRM_USER);
        jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
    }
    else
    {
        LOG_E(tag_) << "cmdConfirmUser, failed to create player, err:" << err;
    }
    
    return err;
}

void octillion::World::tickcallback(uint32_t type, uint32_t param1, uint32_t param2)
{
}

void octillion::World::addcmd(int fd, Command * cmd)
{
    cmds_lock_.lock();

    if (cmds_.find(fd) == cmds_.end())
    {
        // we only keep one cmd for each player in cmd queue during single tick
        cmds_[fd] = cmd;
    }
    else
    {
        LOG_W(tag_) << "duplicate cmd for fd: " << fd;
    }

    cmds_lock_.unlock();
}

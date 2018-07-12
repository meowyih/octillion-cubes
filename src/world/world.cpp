#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/world.hpp"
#include "world/cube.hpp"
#include "world/command.hpp"
#include "world/event.hpp"

#include "server/rawprocessor.hpp"

#include "database/database.hpp"

#include "jsonw/jsonw.hpp"

const std::string octillion::World::tag_ = "World";

octillion::World::World()
{
    LOG_D(tag_) << "World() start";

    // mark vector
    std::map<int, std::map<std::string, Cube*>*> area_marks;

    // init database
    database_.init(std::string("save"));

    // init area
    std::vector<std::string> areafnames = { 
        "data/id_1_control_area.json",
        "data/id_2_control_area_underground.json"
    };

    for (size_t i = 0; i < areafnames.size(); i++)
    {
        // read area data
        std::string fname = areafnames[i];
        std::ifstream fin(fname);

        if (!fin.good())
        {
            LOG_E(tag_) << "Failed to init area file:" << fname << std::endl;
            return;
        }

        JsonW* json = new JsonW(fin);
        if (json->valid() == false)
        {
            LOG_E(tag_) << "Failed to init area file:" << fname << std::endl;
            delete json;
            return;
        }
        else
        {
            // read area cubes
            Area* area = new Area(json);

            if (area->valid())
            {
                areas_.insert(area);
            }
            else
            {
                LOG_E(tag_) << "World() failed to load area file: " << fname << std::endl;
            }
            LOG_I(tag_) << "World() load area:" << area->id() << " contains cubes:" << area->cubes_.size();

            // read area marks
            std::map<std::string, Cube*>* marks = new std::map<std::string, Cube*>();

#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().alloc(__FILE__, __LINE__, marks);
#endif

            Area::getmark(json, *marks, area->cubes_);
            area_marks[area->id()] = marks;

            delete json;
        }

        fin.close();
    }

    // create cubes short cut to areas_
    for (const auto& it : areas_)
    {
        for (const auto& mapit : it->cubes_)
        {
            cubes_[mapit.first] = mapit.second;
        }
    }

    // read global link (link areas)
    std::ifstream fin("data/_global.json");
    if (!fin.good())
    {
        LOG_E(tag_) << "Failed to init area file:" << "data/_global.json" << std::endl;
        return;
    }

    size_t global_link_count = 0;
    JsonW* jglobal = new JsonW(fin);
    JsonW* jglinks = jglobal->get(u8"links");
    for (size_t idx = 0; idx < jglinks->size(); idx++)
    {        
        JsonW* jglink = jglinks->get(idx);
        JsonW* jfrom = jglink->get(u8"from");
        JsonW* jto = jglink->get(u8"to");
        JsonW* jfromcube = jfrom->get("cube");
        JsonW* jtocube = jto->get("cube");

        // get link type, area 'from' id, area 'to' id
        int linktype = (int) jglink->get(u8"type")->integer();
        int area_from = (int)jfrom->get(u8"area")->integer();
        int area_to = (int)jto->get(u8"area")->integer();

        // get marks map for area from and area to
        auto itmark = area_marks.find(area_from);
        if (itmark == area_marks.end())
        {
            LOG_E(tag_) << "no mark for area id " << area_from;
            continue;
        }
        std::map<std::string, Cube*>* marks_from = itmark->second;

        itmark = area_marks.find(area_to);
        if (itmark == area_marks.end())
        {
            LOG_E(tag_) << "no mark for area id " << area_to;
            continue;
        }
        std::map<std::string, Cube*>* marks_to = itmark->second;

        // get Cube* from and Cube* to
        Cube* from;
        Cube* to;

        if (jfromcube->type() == JsonW::STRING)
        {
            auto itcube = marks_from->find(jfromcube->str());
            if (itcube == marks_from->end())
            {
                LOG_E(tag_) << "World(), no mark " << jfromcube->str() << " in area " << area_from;
                continue;
            }
            from = itcube->second;            
        }
        else if (jfromcube->type() == JsonW::ARRAY)
        {
            bool ret;
            CubePosition pos;
            ret = Area::readloc(jfromcube, pos, 0, 0, 0);

            if (ret == false)
            {
                LOG_E(tag_) << "World(), bad from loc: " << jfromcube->text();
                continue;
            }

            auto itcube = cubes_.find(pos);
            if (itcube == cubes_.end())
            {
                LOG_E(tag_) << "World(), no cube for pos " << pos.str();
                continue;
            }
            from = itcube->second;
        }
        else
        {
            LOG_E(tag_) << "World(), bad from loc: " << jfromcube->text();
            continue;
        }

        if (jtocube->type() == JsonW::STRING)
        {
            auto itcube = marks_to->find(jtocube->str());
            if (itcube == marks_to->end())
            {
                LOG_E(tag_) << "World(), no mark " << jtocube->str() << " in area " << area_from;
                continue;
            }
            to = itcube->second;
        }
        else if (jtocube->type() == JsonW::ARRAY)
        {
            bool ret;
            CubePosition pos;
            ret = Area::readloc(jtocube, pos, 0, 0, 0);
            if (ret == false)
            {
                LOG_E(tag_) << "World(), bad to loc: " << jtocube->text();
                continue;
            }

            auto itcube = cubes_.find(pos);
            if (itcube == cubes_.end())
            {
                LOG_E(tag_) << "World(), no cube for pos " << pos.str();
                continue;
            }
            to = itcube->second;
        }
        else
        {
            LOG_E(tag_) << "World(), bad to loc: " << jtocube->text();
            continue;
        }

        // create link
        if (Area::addlink(linktype, from, to) == false)
        {
            LOG_E(tag_) << "World(), failed to add link between " 
                << from->loc().str() << " " << to->loc().str();
        }
        else
        {
            global_link_count++;
        }
    }

    LOG_E(tag_) << "World() add global link " << global_link_count;
    delete jglobal;

    // release marks
    for (auto& itmark : area_marks)
    {
#ifdef MEMORY_DEBUG
        MemleakRecorder::instance().release(itmark.second);
#endif

        delete itmark.second;
    }

    LOG_I(tag_) << "World() creates cube links:" << cubes_.size();
    LOG_D(tag_) << "World() done";
}

octillion::World::~World()
{
    LOG_D(tag_) << "~World() start";

    for (auto& it : players_)
    {
        Player* player = it.second;

        if ( player == NULL )
        {
            continue;
        }

        LOG_D(tag_) << "~World() save and remove player resource, fd:" << it.first << " username:" << player->username() << " pcid:" << player->id();
        
        // neither enter() nor leave() create any resource, 
        // no need to force player to *leave* the world since no event need to 
        // send back to them
        // leave(it.second);

        // save player's information
        database_.save(it.second);

        // delete player from players_
        delete it.second;
    }

    for (auto& it : cmds_)
    {
        LOG_D(tag_) << "~World() delete cmd fd:" << it->fd();
        delete it;
    }

    for (auto it : cube_players_)
    {
        LOG_D(tag_) << "~World() delete std::set in cube_players_";
        if (it.second != NULL)
        {
#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(it.second);
#endif
            delete it.second;
        }
        else
        {
            Cube* cube = it.first;
            CubePosition loc = cube->loc();
            LOG_W(tag_) << "~World(), warning: found null std:set in cube_players_, loc " 
                << loc.x() << "," << loc.y() << "," << loc.z();
        }
    }

    for (auto it : area_players_)
    {
        LOG_D(tag_) << "~World() delete std::set in area_players_";
        if (it.second != NULL)
        {
#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(it.second);
#endif
            delete it.second;
        }
        else
        {
            int area = it.first;
            LOG_W(tag_) << "~World(), warning: found null std:set in area_players_, area id:" << area;
        }
    }

    for (auto it : areas_)
    {
        LOG_D(tag_) << "~World() delete area:" << it->id();
        delete it;
    }

    LOG_D(tag_) << "~World() done";
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

std::error_code octillion::World::disconnect(int fd, std::list<Event*>& events)
{
    auto it = players_.find(fd);
    
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
    }
    else 
    {    
        LOG_I( tag_ ) << "disconnect() fd:" << fd << " save data";

        // force player to leave the world
        leave(player, events);

        // save player information
        database_.save( player );

        // delete player object from players_
        delete player;

        // remove the fd mapping from players_
        players_.erase( it );
    }

    return OcError::E_SUCCESS;
}

std::error_code octillion::World::tick()
{    
    // if need to freeze
    bool freezeworld = false;
    
    // jsons that need send back to player
    std::map<int, JsonW*> jsons;

    // command's response
    std::map<int, JsonW*> cmdbacks;

    // all events generated in this tick
    std::list<Event*> events;
    
    // LOG_D(tag_) << "tick start, cmds_.size:" << cmds_.size();

    // handle commands in cmds_
    cmds_lock_.lock();

    for (auto& it : cmds_)
    {
        int fd = it->fd();
        Command* cmd = it;
        std::error_code err = OcError::E_SUCCESS;
        JsonW* cmdback = new JsonW();
        
        if (cmd == NULL || !cmd->valid())
        {
            cmdback->add(u8"err", Command::E_CMD_BAD_FORMAT);
        }
        else // valid cmd
        {
            switch (cmd->cmd())
            {
            case Command::CONNECT:
                connect(fd);
                delete cmdback;
                // CONNECT is special command that created by rawprocessor, no need to send feedback to user
                continue;
            case Command::DISCONNECT:
                disconnect(fd, events);
                delete cmdback;
                // DISCONNECT is special command that created by rawprocessor, no need to send feedback to user
                continue;
            case Command::VALIDATE_USERNAME:
                err = cmdValidateUsername(fd, cmd, cmdback);
                break;
            case Command::CONFIRM_USER:
                err = cmdConfirmUser(fd, cmd, cmdback, events);
                break;
            case Command::LOGIN:
                err = cmdLogin(fd, cmd, cmdback, events);
                break;
            case Command::LOGOUT:
                err = cmdLogout(fd, cmd, cmdback, events);
                break;
            case Command::MOVE_NORMAL:
                err = cmdMove(fd, cmd, cmdback, events);
                break;
            case Command::FREEZE_WORLD:
                err = cmdFreezeWorld(fd, cmd, cmdback);
                if (err == OcError::E_SUCCESS)
                {
                    freezeworld = true;
                }
                break;
            default: // undefined commands
                err = cmdUnknown(fd, cmd, cmdback);
                break;
            }
        }

        if ( err == OcError::E_SUCCESS )
        {
            // insert data to output data map
            cmdbacks[fd] = cmdback;
        }
        else if ( err == OcError::E_PROTOCOL_FD_LOGOUT )
        {
            // close fd due to logout
            RawProcessor::closefd(fd);
            delete cmdback;
            LOG_I(tag_) << "tick, request disconnect due to logout cmd";
        }
        else
        {
            // fatal error, close fd
            RawProcessor::closefd(fd);
            delete cmdback;
            LOG_E(tag_) << "tick, failed to handle incoming command cmd:" << cmd->cmd();
        }
    }

    // all cmd were transfer to cmdbacks, clear cmds_
    for (auto& it : cmds_)
    {
        delete it;
    }

    cmds_.clear();
    cmds_lock_.unlock();

    // encapsulate cmdbacks (JsonObjectW) into jsons (JsonTextW)
    for ( const auto& itcmdbacks : cmdbacks)
    {
        int fd = itcmdbacks.first;
        JsonW* cmdback = itcmdbacks.second;
        JsonW* containerobj = new JsonW();
        containerobj->add(u8"cmd", cmdback);
        jsons[fd] = containerobj;
    }

    // handle events
    for (auto& event : events)
    {        
        if (event->range_ == Event::RANGE_WORLD) // RANGE_WORLD event
        {
            addjsons(event, &world_players_, jsons);
        }
        else if (event->range_ == Event::RANGE_AREA)
        {
            auto it = area_players_.find(event->areaid_);
            if (it == area_players_.end())
            {
                // no players in this area, ignore it
                continue;
            }
            // add event to all the players in the same cube
            addjsons(event, it->second, jsons);
        }
        else if (event->range_ == Event::RANGE_CUBE) // RANGE_CUBE event
        {
            auto it = cube_players_.find(event->eventcube_);
            if (it == cube_players_.end())
            {
                // no players in this cube, ignore it
                continue;
            }

            // add event to all the players in the same cube
            addjsons(event, it->second, jsons);
        }
        else // TODO: handle other range events
        {
            LOG_E(tag_) << "tick(), expected event range:" << event->range_;
        } // end of if (event->range_ == Event::RANGE_WORLD) 
    }

    // release plyevents
    for (auto& event : events)
    {
        delete event;
    }
    events.clear();
    
    // send data back to players_ and release jsons
    for ( const auto& it : jsons )
    {
        int fd = it.first;
        JsonW* jtext = it.second;
        std::string utf8 = jtext->text();
        RawProcessor::senddata(fd, (uint8_t*)utf8.data(), utf8.size() );

        LOG_D(tag_) << "write fd:" << fd << " json:" << utf8 << std::endl;

        delete jtext;
    }

    // freeze the world and return
    if (freezeworld)
    {
        // cannot stop the server inside the cmds_lock_ mutex
        LOG_I(tag_) << "tick, stopping the coreserver";

        // stop server
        octillion::CoreServer::get_instance().stop();

        return OcError::E_WORLD_FREEZED;
    }

    // LOG_D(tag_) << "tick, done";
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdUnknown(int fd, Command *cmd, JsonW* jback)
{
    LOG_D(tag_) << "cmdUnknown start";
    jback->add(u8"err", cmd->cmd());
    jback->add(u8"err", Command::E_CMD_UNKNOWN_COMMAND );
    LOG_D(tag_) << "cmdUnknown done";
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdValidateUsername(int fd, Command *cmd, JsonW* jsonobject)
{
    std::string username, validname;
    std::error_code err;

    username = cmd->strparms_[0];
    
    LOG_D(tag_) << "cmdValidateUsername start, fd:" << fd << " name:" << username;

    auto it = players_.find(fd);

    if (it == players_.end())
    {
        LOG_E(tag_) << "cmdValidateUsername fd:" << fd << " does not exist in players_";
        return OcError::E_FATAL;
    }

    Player* player = it->second;

    if (player != NULL)
    {
        LOG_E(tag_) << "cmdValidateUsername fd:" << fd << " has player data.";
        return OcError::E_FATAL;
    }

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
            if (appendix > 999)
            {
                jsonobject->add(u8"cmd", Command::VALIDATE_USERNAME);
                jsonobject->add(u8"err", Command::E_CMD_TOO_COMMON_NAME);
                LOG_D(tag_) << "cmdValidateUsername, err: name " << username << " too common";
                return OcError::E_SUCCESS;
            }

        } while (err != OcError::E_SUCCESS);

        validname = altname;
    }

    jsonobject->add(u8"cmd", Command::VALIDATE_USERNAME);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
    jsonobject->add(u8"s1", validname);

    LOG_D(tag_) << "cmdValidateUsername done, valid name:" << validname;
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdConfirmUser(int fd, Command *cmd, JsonW* jsonobject, std::list<Event*>& events)
{
    std::error_code err;
    std::string username = cmd->strparms_[0];
    std::string password = cmd->strparms_[1];
    int gender = cmd->uiparms_[0];
    int cls = cmd->uiparms_[1];
    
    LOG_D(tag_) << "cmdConfirmUser start, fd:" << fd << " user:" << username << " gender:" << gender << " cls:" << cls;
    
    // fd should exists in players_ without loading any data (not yet created)
    auto it = players_.find( fd );
    if ( it == players_.end() )
    {
        LOG_E(tag_) << "cmdConfirmUser, failed to create player since fd " << fd << " does not exist in players_";
        return OcError::E_PROTOCOL_FD_NO_CONNECT;
    }
    else if ( it->second != NULL )
    {
        LOG_E(tag_) << "cmdConfirmUser, failed to create player since fd " << fd << " already login in players_";
        return OcError::E_PROTOCOL_FD_DUPLICATE_CONNECT;
    }

    //  create a default player
    Player* player = new Player();

    player->fd(fd);
    player->username(username);
    player->password( database_.hashpassword(password) );
    player->gender(gender);
    player->cls(cls);

    CubePosition loc;
    err = database_.create(fd, player, loc);

    if (err != OcError::E_SUCCESS)
    {
        delete player;
        LOG_E(tag_) << "cmdConfirmUser, failed to create player:" << username << ", err:" << err;
        return err;
    }

    // set cube
    auto cit = cubes_.find(loc);
    if (cit == cubes_.end())
    {
        LOG_E(tag_) << "cmdConfirmUser, position " << loc.str() << " does not exist";
        delete player;
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }
    player->cube(cit->second);

    // set feedback
    jsonobject->add(u8"cmd", Command::CONFIRM_USER);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
    jsonobject->add(u8"id", (int_fast32_t)player->id());
    
    // player enter the world
    players_[fd] = player;
    enter(player, events);
        
    LOG_D(tag_) << "cmdConfirmUser done, user:" << username;

    return err;
}

std::error_code octillion::World::cmdLogin(int fd, Command* cmd, JsonW* jsonobject, std::list<Event*>& events)
{
    std::error_code err;
    std::string username = cmd->strparms_[0];
    std::string password = cmd->strparms_[1];
    
    LOG_D(tag_) << "cmdLogin start, fd:" << fd << " user:" << username;
    
    // fd should exists in players_
    auto it = players_.find( fd );
    if ( it == players_.end() )
    {
        LOG_E(tag_) << "cmdLogin, failed to login player since fd:" << fd << " does not exist in players_";
        return OcError::E_PROTOCOL_FD_NO_CONNECT;
    }
    else if ( it->second != NULL )
    {
        LOG_E(tag_) << "cmdLogin, login fd:" << fd << " user:" << username << " already exists in players_";
        database_.save( it->second );
        players_.erase( it );
        return OcError::E_PROTOCOL_FD_DUPLICATE_CONNECT;
    }
    
    uint_fast32_t pcid = database_.login( username, password );
    
    if ( pcid == 0 )
    {
        jsonobject->add(u8"cmd", Command::LOGIN);
        jsonobject->add(u8"err", Command::E_CMD_WRONG_USERNAME_PASSWORD);
        LOG_D(tag_) << "cmdLogin, err: bad username/password" << username << " " << password;
        return OcError::E_SUCCESS;
    }
    
    Player* player = new Player();
    CubePosition loc;
    err = database_.load( pcid, player, loc );
    if (err != OcError::E_SUCCESS)
    {
        LOG_E(tag_) << "cmdLogin, database_.load pcid:" << pcid << " return err:" << err;
        delete player;
        return err;
    }

    auto cit = cubes_.find(loc);
    if (cit == cubes_.end())
    {
        LOG_E(tag_) << "cmdLogin, position " << loc.str() << " does not exist";
        delete player;
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }
    player->cube(cit->second);
    
    // mapping player with fd
    player->fd(fd);
    players_[fd] = player;

    // player enter the world
    enter(player, events);

    jsonobject->add(u8"cmd", Command::LOGIN);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
    jsonobject->add(u8"id", (int_fast32_t)player->id());
    
    LOG_I(tag_) << "cmdLogin done, fd:" << fd << " pcid:" << pcid;
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdLogout(int fd, Command* cmd, JsonW* jsonobject, std::list<Event*>& events)
{
    LOG_D(tag_) << "cmdLogout start, fd:" << fd;
    
    // fd should exists in players_
    auto it = players_.find( fd );
    if ( it == players_.end() )
    {
        LOG_E(tag_) << "cmdLogout, failed to logout player since fd:" << fd << " does not exist in players_";
        return OcError::E_PROTOCOL_FD_NO_CONNECT;
    }

    // save and release player
    if (it->second != NULL)
    {
        // save player information
        database_.save(it->second);

        // remove player from the world
        leave(it->second, events);

        // delete the player object from playes_, but the fd entry is still alive until
        // physically disconnect
        delete it->second;
        it->second = NULL;
    }

    jsonobject->add(u8"cmd", Command::LOGOUT);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);

    LOG_D(tag_) << "cmdLogout done, fd:" << fd;
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdMove(int fd, Command* cmd, JsonW* jsonobject, std::list<Event*>& events)
{
    LOG_D(tag_) << "cmdMove start, fd:" << fd;

    // fd should exists in players_
    auto pit = players_.find(fd);
    if (pit == players_.end())
    {
        LOG_E(tag_) << "cmdMove, failed since fd:" << fd << " does not exist in players_";
        return OcError::E_PROTOCOL_FD_NO_CONNECT;
    }

    // check player's location and the target location
    Player* player = pit->second;
    CubePosition targetloc(player->cube()->loc(), cmd->uiparms_[0]);

    // check if cube exist
    auto cit = cubes_.find(targetloc);
    if (cit == cubes_.end())
    {
        LOG_E(tag_) << "cmdMove, player moves to invalid position " << targetloc.str();
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }

    // TODO: check player's status to see if moveable or not

    // create event
    Event* event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_ARRIVE;
    event->player(*player);
    event->eventcube_ = cit->second;
    event->subcube_ = player->cube();
    event->direction_ = Cube::opposite_dir(cmd->uiparms_[0]);
    events.push_back(event);

    event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_LEAVE;
    event->player(*player);
    event->eventcube_ = player->cube();
    event->subcube_ = cit->second;
    event->direction_ = cmd->uiparms_[0];
    events.push_back(event);

    // move player
    move(player, cit->second);

    // generate cmdback
    jsonobject->add(u8"cmd", Command::MOVE_NORMAL);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);

    LOG_D(tag_) << "cmdMove done, fd:" << fd << " pos:" << player->cube()->loc().str();
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdFreezeWorld(int fd, Command* cmd, JsonW* jsonobject)
{
    LOG_D(tag_) << "cmdFreezeWorld start, fd:" << fd;

    // TODO: check if allow to freeze
    jsonobject->add(u8"cmd", Command::FREEZE_WORLD);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);

    LOG_D(tag_) << "cmdFreezeWorld done, fd:" << fd;
    return OcError::E_SUCCESS;
}

void octillion::World::addcmd(Command * cmd)
{
    cmds_lock_.lock();
    cmds_.push_back(cmd);
    cmds_lock_.unlock();
}

std::error_code octillion::World::enter(Player* player, std::list<Event*>& events)
{
    LOG_D(tag_) << "enter start";

    // put player in the world_players_
    world_players_.insert(player);

    // put the player in the area_players_
    int areaid = player->cube()->area();
    auto it_area_playerset = area_players_.find(areaid);
    if (it_area_playerset == area_players_.end())
    {
        std::set<Player*>* playerset = new std::set<Player*>();

#ifdef MEMORY_DEBUG
        MemleakRecorder::instance().alloc(__FILE__, __LINE__, playerset);
#endif

        playerset->insert(player);
        area_players_[areaid] = playerset;
    }
    else
    {
        std::set<Player*>* playerset = it_area_playerset->second;
        playerset->insert(player);
    }

    // put player in the cube_players_    
    auto it_cube_playerset = cube_players_.find(player->cube());
    if (it_cube_playerset == cube_players_.end())
    {
        std::set<Player*>* playerset = new std::set<Player*>();

#ifdef MEMORY_DEBUG
        MemleakRecorder::instance().alloc(__FILE__, __LINE__, playerset);
#endif

        playerset->insert(player);
        cube_players_[player->cube()] = playerset;
    }
    else
    {
        std::set<Player*>* playerset = it_cube_playerset->second;
        playerset->insert(player);
    }

    // create event
    Event* event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_LOGIN;
    event->player(*player);
    event->eventcube_ = player->cube();
    events.push_back(event);

    LOG_D(tag_) << "enter done";

    return OcError::E_SUCCESS;
}
std::error_code octillion::World::leave(Player* player, std::list<Event*>& events)
{
    LOG_D(tag_) << "leave start";

    // delete player from cube_players_, 
    // which keep track of all the players in one specific cube
    CubePosition loc = player->cube()->loc();
    auto itcube = cubes_.find(loc);

    if (itcube == cubes_.end())
    {
        // TODO: put player in any default cube
        LOG_E(tag_) << "Player " << player->id() << " try to leave from non-exist cube x:" << loc.x() << " y:" << loc.y() << " z:" << loc.z();
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }

    // remove from world_players_
    if (world_players_.erase(player) == 1)
    {
        LOG_D(tag_) << "remove player:" << player->id() << " from world_players_";
    }
    else
    {
        LOG_E(tag_) << "world_players_ does not contain player id:" << player->id();
    }

    // remove from area_players_
    Cube* cube = itcube->second;
    uint_fast32_t areaid = cube->area();
    auto it_area_playerset = area_players_.find(areaid);
    if (it_area_playerset == area_players_.end())
    {
        LOG_E(tag_) << "player does not exist in area_players_";
    }
    else
    {
        std::set<Player*>* playerset = it_area_playerset->second;
        playerset->erase(player);

        LOG_D(tag_) << "remove player " << player->id() << " from area_players_";
        if (playerset->size() == 0)
        {

#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(it_area_playerset->second);
#endif
            delete it_area_playerset->second;
            area_players_.erase(it_area_playerset);

            LOG_D(tag_) << "area:" << areaid << " has no players, remove playerset from area_players_";
        }
    }

    // remove from cube_players_
    auto it_cube_playerset = cube_players_.find(cube);
    if (it_cube_playerset == cube_players_.end())
    {
        // fatal error, player does not exist in cube_players_
        LOG_E(tag_) << "player does not exist in cube_players_";
    }
    else
    {


        // remove player from cube_players_
        std::set<Player*>* playerset = it_cube_playerset->second;
        playerset->erase(player);
        LOG_D(tag_) << "remove player " << player->id() << " from cube_players_";

        if (playerset->size() == 0)
        {

#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(playerset);
#endif
            delete playerset;
            cube_players_.erase(it_cube_playerset);

            LOG_D(tag_) << "cube " << loc.x() << "," << loc.y() << "," << loc.z()
                << " has no players, remove playerset from cube_players_ map";
        }
    }

    // create event
    Event* event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_LOGOUT;
    event->player(*player);
    event->eventcube_ = cube;
    events.push_back(event);

    LOG_D(tag_) << "leave done";

    return OcError::E_SUCCESS;
}

void octillion::World::addjsons(Event* event, std::set<Player*>* players, std::map<int, JsonW*>& jsons)
{
    for (auto& player : *players)
    {
        JsonW* jarray; // events array
        int fd = player->fd();

        // get "events" array in jsons for the player
        // case 1: fd does not exist in jsons
        // case 2: fd exist in jsons, but there is no "events" name-pair in main object
        // case 3: fd exist and there is "events" name-pair in main object
        auto fdjson = jsons.find(fd);
        if (fdjson == jsons.end())
        {
            // case 1: fd does not exist in jsons
            JsonW* jobject = new JsonW();
            jarray = new JsonW();
            jobject->add("events", jarray);
            jsons[fd] = jobject;
        }
        else
        {
            JsonW* jobject = fdjson->second;
            jarray = jobject->get("events");

            if (jarray == NULL)
            {
                jarray = new JsonW();
                jobject->add("events", jarray);
            }
        } // end of get jarray

        jarray->add(event->json());
    } // for (auto& player : *players)
} // void octillion::World::addjsons

// help function, move player location and change cube_players_, and area_players_
// function WILL NOT check the newloc's existence
void octillion::World::move(Player* player, Cube* cube_to)
{
    Cube* cube_from = player->cube();

    int area_from = cube_from->area();
    int area_to = cube_to->area();

    // check area 
    if (area_from != area_to)
    {
        // remove player from area_players
        auto itarea = area_players_.find(area_from);
        if (itarea == area_players_.end())
        {
            LOG_E(tag_) << "move() err, cannot area_from id:" << area_from << " in area_players_";
            return;
        }
        std::set<Player*>* player_set = itarea->second;
        size_t size = player_set->erase(player);

        if (size != 1)
        {
            LOG_E(tag_) << "move() err, erase player " << player->id() 
                << " from area_from id:" << area_from 
                << " in area_players_ return " << size;
            return;
        }

        if (player_set->size() == 0)
        {
#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(player_set);
#endif
            delete player_set;
            area_players_.erase(itarea);
        }

        // add player into area_players
        itarea = area_players_.find(area_to);
        if (itarea == area_players_.end())
        {
            player_set = new std::set<Player*>();

#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().alloc(__FILE__, __LINE__, player_set);
#endif

            area_players_[area_to] = player_set;
        }
        else
        {
            player_set = itarea->second;
        }

        player_set->insert(player);
    }

    if (cube_from != cube_to)
    {
        std::set<Player*>* player_set;
        size_t size;

        // remove player from core_players_
        auto itcube = cube_players_.find(cube_from);
        if (itcube == cube_players_.end())
        {
            LOG_E(tag_) << "move() err, cannot find cube:" << cube_from->loc().str() << " in cube_players_";
            return;
        }

        player_set = itcube->second;
        size = player_set->erase(player);
        if (size != 1)
        {
            LOG_E(tag_) << "move() err, erase player " << player->id()
                << " from cube_players_ loc:" << cube_from->loc().str()
                << " in cube_players_ return " << size;
            return;
        }

        if (player_set->size() == 0)
        {
            delete player_set;
#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(player_set);
#endif
            cube_players_.erase(itcube);
        }

        // add player to core_players_
        itcube = cube_players_.find(cube_to);
        if (itcube == cube_players_.end())
        {
            player_set = new std::set<Player*>();

#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().alloc(__FILE__, __LINE__, player_set);
#endif

            cube_players_[cube_to] = player_set;
        }
        else
        {
            player_set = itcube->second;
        }

        player_set->insert(player);
    }

    player->cube(cube_to);
}
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

    // init database
    database_.init(std::string("save"));

    // init area
    std::vector<std::string> areafnames = { 
        "data/id_1_control_area.json" 
    };

    for (size_t i = 0; i < areafnames.size(); i++)
    {
        std::string fname = areafnames[i];
        std::ifstream fin(fname);

        if (!fin.good())
        {
            LOG_E(tag_) << "Failed to init area file:" << fname << std::endl;
            return;
        }

        octillion::JsonTextW* json = new octillion::JsonTextW(fin);
        if (json->valid() == false)
        {
            LOG_E(tag_) << "Failed to init area file:" << fname << std::endl;
            delete json;
            return;
        }
        else
        {
            octillion::Area* area = new Area(json);
            areas_.insert(area);
            LOG_I(tag_) << "World() load area:" << area->id() << " contains cubes:" << area->cubes_.size();
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
        // no nned to force player to *leave* the world since no event need to 
        // send back to enduser
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
            LOG_W(tag_) << "~World(), warning: found non-empty std:set in cube_players_, loc " 
                << loc.x() << "," << loc.y() << "," << loc.z();
        }
    }

    for (auto it : areas_)
    {
        LOG_D(tag_) << "~World() delete area:" << it->id();
        delete it;
    }

    LOG_D(tag_) << "~World() done";
}

std::error_code octillion::World::connect(int fd, std::map<Cube*, std::list<Event*>*>& events)
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

std::error_code octillion::World::disconnect(int fd, std::map<Cube*, std::list<Event*>*>& events)
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
    std::map<int, JsonTextW*> jsons;

    // command's response
    std::map<int, JsonObjectW*> cmdbacks;

    // events created by players, map key is where the event created
    std::map<Cube*, std::list<Event*>*> plyevents;
    
    // LOG_D(tag_) << "tick start, cmds_.size:" << cmds_.size();

    // handle commands in cmds_
    cmds_lock_.lock();

    for (auto& it : cmds_)
    {
        int fd = it->fd();
        Command* cmd = it;
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
            case Command::CONNECT:
                connect(fd, plyevents);
                delete cmdback;
                // CONNECT is special command that created by rawprocessor, no need to send feedback to user
                continue;
            case Command::DISCONNECT:
                disconnect(fd, plyevents);
                delete cmdback;
                // DISCONNECT is special command that created by rawprocessor, no need to send feedback to user
                continue;
            case Command::VALIDATE_USERNAME:
                err = cmdValidateUsername(fd, cmd, cmdback);
                break;
            case Command::CONFIRM_USER:
                err = cmdConfirmUser(fd, cmd, cmdback, plyevents);
                break;
            case Command::LOGIN:
                err = cmdLogin(fd, cmd, cmdback, plyevents);
                break;
            case Command::LOGOUT:
                err = cmdLogout(fd, cmd, cmdback, plyevents);
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
        JsonObjectW* cmdback = itcmdbacks.second;
        JsonObjectW* containerobj = new JsonObjectW();
        containerobj->add(u8"cmd", cmdback);
        JsonTextW* jsontext = new JsonTextW(containerobj);
        jsons[fd] = jsontext;
    }

    // handle player events
    for (auto& plyevent : plyevents)
    {
        Cube* eventcube = plyevent.first;
        auto itplayerset = cube_players_.find(eventcube);

        if (itplayerset == cube_players_.end())
        {
            // no players in this cube, ignore it
            continue;
        }

        std::list<Event*>* eventlist = plyevent.second;
        for (auto& event : *eventlist)
        {
            // add event to all the players in the save cube
            addjsons(event, itplayerset->second, jsons);
        }
    }

    // release plyevents
    for (auto& plyevent : plyevents)
    {
        std::list<Event*>* elist = plyevent.second;

        for (auto& event : *elist)
        {
            delete event;
        }

#ifdef MEMORY_DEBUG
        MemleakRecorder::instance().release(elist);
#endif

        delete elist;
    }
    plyevents.clear();
    
    // send data back to players_ and release jsons
    for ( const auto& it : jsons )
    {
        int fd = it.first;
        JsonTextW* jsontext = it.second;
        std::string utf8 = jsontext->string();
        RawProcessor::senddata(fd, (uint8_t*)utf8.data(), utf8.size() );

        LOG_D(tag_) << "write fd:" << fd << " json:" << utf8 << std::endl;

        delete jsontext;
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

std::error_code octillion::World::cmdUnknown(int fd, Command *cmd, JsonObjectW* jsonobject)
{
    LOG_D(tag_) << "cmdUnknown start";
    jsonobject->add(u8"err", Command::E_CMD_UNKNOWN_COMMAND );
    LOG_D(tag_) << "cmdUnknown done";
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdValidateUsername(int fd, Command *cmd, JsonObjectW* jsonobject)
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

std::error_code octillion::World::cmdConfirmUser(int fd, Command *cmd, JsonObjectW* jsonobject, std::map<Cube*, std::list<Event*>*>& events)
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

    err = database_.create(fd, player);

    if (err == OcError::E_SUCCESS)
    {        
        jsonobject->add(u8"cmd", Command::CONFIRM_USER);
        jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
        players_[fd] = player;

        // player enter the world
        enter(player, events);
        
        LOG_D(tag_) << "cmdConfirmUser done, user:" << username;
    }
    else
    {
        delete player;
        LOG_E(tag_) << "cmdConfirmUser, failed to create player:" << username << ", err:" << err;
    }

    return err;
}

std::error_code octillion::World::cmdLogin(int fd, Command* cmd, JsonObjectW* jsonobject, std::map<Cube*, std::list<Event*>*>& events)
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
    
    uint32_t pcid = database_.login( username, password );
    
    if ( pcid == 0 )
    {
        jsonobject->add(u8"cmd", Command::LOGIN);
        jsonobject->add(u8"err", Command::E_CMD_WRONG_USERNAME_PASSWORD);
        LOG_D(tag_) << "cmdLogin, err: bad username/password" << username << " " << password;
        return OcError::E_SUCCESS;
    }
    
    Player* player = new Player();
    err = database_.load( pcid, player );
    player->fd(fd);
    
    if ( err != OcError::E_SUCCESS)
    {
        LOG_E(tag_) << "cmdLogin, database_.load pcid:" << pcid << " return err:" << err;
        delete player;
        return err;
    }
    
    // mapping player with fd
    players_[fd] = player;

    // player enter the world
    enter(player, events);

    jsonobject->add(u8"cmd", Command::LOGIN);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);
    
    LOG_I(tag_) << "cmdLogin done, fd:" << fd << " pcid:" << pcid;
    
    return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdLogout(int fd, Command* cmd, JsonObjectW* jsonobject, std::map<Cube*, std::list<Event*>*>& events)
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

std::error_code octillion::World::cmdFreezeWorld(int fd, Command* cmd, JsonObjectW* jsonobject)
{
    LOG_D(tag_) << "cmdFreezeWorld start, fd:" << fd;

    // TODO: check if allow to freeze
    jsonobject->add(u8"cmd", Command::FREEZE_WORLD);
    jsonobject->add(u8"err", Command::E_CMD_SUCCESS);

    LOG_D(tag_) << "cmdFreezeWorld done, fd:" << fd;
    return OcError::E_SUCCESS;
}

void octillion::World::tickcallback(uint32_t type, uint32_t param1, uint32_t param2)
{
}

void octillion::World::addcmd(Command * cmd)
{
    cmds_lock_.lock();
    cmds_.push_back(cmd);
    cmds_lock_.unlock();
}

std::error_code octillion::World::enter(Player* player, std::map<Cube*, std::list<Event*>*>& events)
{
    LOG_D(tag_) << "enter start";

    // insert player into cube_players_, 
    // which keep track of all the players in one specific cube
    CubePosition loc = player->position();
    auto itcube = cubes_.find(loc);

    if (itcube == cubes_.end())
    {
        // TODO: put player in any default cube
        LOG_E(tag_) << "Player " << player->id() << " try to enter to non-exist cube x:" << loc.x() << " y:" << loc.y() << " z:" << loc.z();
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }

    Cube* cube = itcube->second;

    auto itplayer = cube_players_.find(cube);

    if (itplayer == cube_players_.end())
    {
        std::set<Player*>* playerset = new std::set<Player*>();

#ifdef MEMORY_DEBUG
        MemleakRecorder::instance().alloc(__FILE__, __LINE__, playerset);
#endif

        playerset->insert(player);
        cube_players_[cube] = playerset;
    }
    else
    {
        std::set<Player*>* playerset = itplayer->second;
        playerset->insert(player);
    }

    // create event
    Event* event = new Event();
    event->type_ = Event::TYPE_PLAYER_LOGIN;
    event->player_ = player;
    event->tocube_ = cube;
    addevent(cube, event, events);

    LOG_D(tag_) << "enter done";

    return OcError::E_SUCCESS;
}
std::error_code octillion::World::leave(Player* player, std::map<Cube*, std::list<Event*>*>& events)
{
    LOG_D(tag_) << "leave start";

    // delete player from cube_players_, 
    // which keep track of all the players in one specific cube
    CubePosition loc = player->position();
    auto itcube = cubes_.find(loc);

    if (itcube == cubes_.end())
    {
        // TODO: put player in any default cube
        LOG_E(tag_) << "Player " << player->id() << " try to leave from non-exist cube x:" << loc.x() << " y:" << loc.y() << " z:" << loc.z();
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }

    Cube* cube = itcube->second;

    auto itplayer = cube_players_.find(cube);

    if (itplayer == cube_players_.end())
    {
        // fatal error, player does not exist in cube_players_
        LOG_E(tag_) << "Player does not exist in cube_players_";
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }
    else
    {
        std::set<Player*>* playerset = itplayer->second;
        playerset->erase(player);
        LOG_I(tag_) << "Remove player " << player->id() << " from cube_players_";

        if (playerset->size() == 0)
        {

#ifdef MEMORY_DEBUG
            MemleakRecorder::instance().release(itplayer->second);
#endif

            delete itplayer->second;
            cube_players_.erase(itplayer);

            LOG_I(tag_) << "Cube " << loc.x() << "," << loc.y() << "," << loc.z()
                << " has no players, remove cube from cube_players_ map";
        }
    }

    // create event
    Event* event = new Event();
    event->type_ = Event::TYPE_PLAYER_LOGOUT;
    event->player_ = player;
    event->tocube_ = cube;
    addevent(cube, event, events);

    LOG_D(tag_) << "leave done";

    return OcError::E_SUCCESS;
}

void octillion::World::addevent(Cube* cube, Event* event, std::map<Cube*, std::list<Event*>*>& events)
{
    auto itevents = events.find(cube);
    if (itevents == events.end())
    {
        std::list<Event*>* list = new std::list<Event*>();

#ifdef MEMORY_DEBUG
        MemleakRecorder::instance().alloc(__FILE__, __LINE__, list);
#endif

        list->push_back(event);
        events[cube] = list;
    }
    else
    {
        std::list<Event*>* list = itevents->second;
        list->push_back(event);
    }
}

void octillion::World::addjsons(Event* event, std::set<Player*>* players, std::map<int, JsonTextW*>& jsons)
{
    for (auto& player : *players)
    {
        JsonArrayW* jarray; // events array
        int fd = player->fd();

        // get "events" array in jsons for the player
        // case 1: fd does not exist in jsons
        // case 2: fd exist in jsons, but there is no "events" name-pair in main object
        // case 3: fd exist and there is "events" name-pair in main object
        auto fdjson = jsons.find(fd);
        if (fdjson == jsons.end())
        {
            // case 1: fd does not exist in jsons
            JsonObjectW* jobject = new JsonObjectW();
            jarray = new JsonArrayW();
            jobject->add("events", jarray);
            JsonTextW* jtext = new JsonTextW(jobject);
            jsons[fd] = jtext;
        }
        else
        {
            JsonTextW* jtext = fdjson->second;
            JsonObjectW* jobject = jtext->value()->object();
            JsonValueW* jvalue = jobject->find("events");

            if (jvalue == NULL)
            {
                jarray = new JsonArrayW();
                jobject->add("events", jarray);
            }
            else
            {
                jarray = jvalue->array();
            }
        } // end of get jarray

        jarray->add(event->json());
    } // for (auto& player : *players)
} // void octillion::World::addjsons
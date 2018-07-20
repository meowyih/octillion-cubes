#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "world/world.hpp"
#include "world/creature.hpp"
#include "world/cube.hpp"
#include "world/command.hpp"
#include "world/event.hpp"
#include "world/mob.hpp"

#include "server/rawprocessor.hpp"

#include "database/database.hpp"

#include "jsonw/jsonw.hpp"

const std::string octillion::World::tag_ = "World";

octillion::World::World()
{
	bool init_succeed = true;

	// init random seed for World() rand() usage
	srand((unsigned int) time(NULL));

    LOG_D(tag_) << "World() start";

    // mark vector
    std::map<int, std::map<std::string, Cube*>*> area_marks;

	// default monster configuration
	std::map<uint_fast32_t, Mob*> mob_wiki;

    // init database
    database_.init(std::string("save"));

	// read global data
	std::ifstream fin(global_config_file_);
	if (!fin.good())
	{
		LOG_E(tag_) << "Failed to open " << global_config_file_;
		init_succeed = false;
		return;
	}
	
	JsonW* jglobal = new JsonW(fin);
	if ( jglobal->valid() == false )
	{
		LOG_E(tag_) << "Failed to read " << global_config_file_;
		init_succeed = false;
		return;
	}

	// read stamp
	global_config_stamp_ = jglobal->get(u8"stamp")->str();
	if (global_config_stamp_.length() == 0)
	{
		LOG_E(tag_) << "Failed to get stamp from " << global_config_file_;
		init_succeed = false;
		delete jglobal;
		return;
	}

	JsonW* jarea_files = jglobal->get(u8"area");
	if (jarea_files->valid() == false || jarea_files->type() != JsonW::ARRAY || jarea_files->size() == 0)
	{
		LOG_E(tag_) << "Failed to get area from " << global_config_file_;
		init_succeed = false;
		delete jglobal;
		return;
	}

	for (size_t i = 0; i < jarea_files->size(); i++)
	{
		JsonW* jarea_file = jarea_files->get(i);
		JsonW* jid = jarea_file->get(u8"id");
		JsonW* jstamp = jarea_file->get(u8"stamp");
		JsonW* jfile = jarea_file->get(u8"file");

		if (jid == NULL || jstamp == NULL || jfile == NULL || 
			jid->integer() == 0 || jstamp->str().length() == 0 || jfile->str().length() == 0)
		{
			LOG_E(tag_) << "Failed to get area file data from " << global_config_file_;
			init_succeed = false;
			delete jglobal;
			return;
		}

		auto it = area_files_.find((int)jid->integer());
		if (it != area_files_.end())
		{
			LOG_E(tag_) << "Failed to duplicate area id " << jid->integer();
			init_succeed = false;
			delete jglobal;
			return;
		}

		area_files_[(int)jid->integer()] = jfile->str();
	}

    // init area
    for (size_t i = 0; i < area_files_.size(); i++)
    {
        // read area data
        std::string fname = jarea_files->get(i)->get(u8"file")->str();
        std::ifstream fin(fname);

        if (!fin.good())
        {
			LOG_E(tag_) << "Failed to init area file:" << fname;
			init_succeed = false;
			delete jglobal;
            return;
        }

        JsonW* json = new JsonW(fin);
        if (json->valid() == false)
        {
			LOG_E(tag_) << "Failed to init area file:" << fname;
			init_succeed = false;
			delete jglobal;
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
				LOG_E(tag_) << "World() failed to load area file: " << fname;
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

	// read global links
	size_t global_link_count = 0;
    JsonW* jglinks = jglobal->get(u8"links");

    for (size_t idx = 0; idx < jglinks->size(); idx++)
    {        
        JsonW* jglink = jglinks->get(idx);
        JsonW* jfrom = jglink->get(u8"from");
        JsonW* jto = jglink->get(u8"to");
        JsonW* jfromcube = jfrom->get("cube");
        JsonW* jtocube = jto->get("cube");

        Cube *from, *to;

        // get link type, area 'from' id, area 'to' id
        std::string linktype = jglink->get(u8"type")->str();
		bool is_twoway = true;
        int area_from = (int)jfrom->get(u8"area")->integer();
        int area_to = (int)jto->get(u8"area")->integer();
		if (linktype == "1way")
		{
			is_twoway = false;
		}

        // read 'from'
        // sample: "from": { "area": 1, "cube":"central" },
        // sample: "from": { "cube":[10001,10001,10001] },
        // sample: "from": { "cube":[1,1,1], "offset": [100000, 100000, 100000],: },
        from = readloc( jfrom, area_marks, cubes_);
        to = readloc( jto, area_marks, cubes_);

        if (from == NULL || to == NULL)
        {
            LOG_E(tag_) << "World() bad json " << jglink->text();
			init_succeed = false;
            continue;
        }
		
		JsonW* jattrs = jglink->get("attr");
		uint_fast32_t attr;
		bool ret;
		if (Cube::json2attr(jattrs, attr) != OcError::E_SUCCESS)
		{
			ret = Area::addlink(is_twoway, from, to);
		}
		else
		{
			ret = Area::addlink(is_twoway, from, to, attr);
		}

        // create link
        if (ret == false)
        {
            LOG_E(tag_) << "World(), failed to add link between " 
                << from->loc().str() << " " << to->loc().str();
			init_succeed = false;
        }
        else
        {
            global_link_count++;
        }
    }

	// read global reborn cube
	JsonW* jgreborn = jglobal->get(u8"reborn");
	CubePosition reborn_loc;
	if (jgreborn == NULL || jgreborn->type() != JsonW::ARRAY)
	{
		LOG_E(tag_) << "World(), failed to read global reborn location.";
		init_succeed = false;
	}
	else
	{
		if (Area::readloc(jgreborn, reborn_loc, 0, 0, 0) == false)
		{
			LOG_E(tag_) << "World(), failed to read global reborn location. JsonW: " << jgreborn->text();
			init_succeed = false;
		}
		else
		{
			auto itcube = cubes_.find(reborn_loc);
			if (itcube == cubes_.end())
			{
				LOG_E(tag_) << "World(), global reborn location does not exist. JsonW: " << jgreborn->text();
				init_succeed = false;
			}
			else
			{
				global_reborn_cube_ = itcube->second;
			}
		}
	}

	// read global mob's wiki
	JsonW* jgmobs = jglobal->get(u8"mobs");
	for (size_t idx = 0; idx < jgmobs->size(); idx++)
	{
		JsonW* jmob = jgmobs->get(idx);
		Mob* mob = new Mob(jmob);

		if (mob == NULL || mob->valid() == false)
		{			
			LOG_E(tag_) << "World() failed to read mob wiki " << jmob->text();
			init_succeed = false;
			delete mob;
			continue;
		}
		else
		{
			auto itmobwiki = mob_wiki.find(mob->type());
			if (itmobwiki != mob_wiki.end())
			{
				LOG_E(tag_) << "World() duplicate mobwiki " << jmob->text();
				init_succeed = false;
				delete mob;
				continue;
			}
			mob_wiki[ mob->type() ] = mob;
		}
	}

	// read area mob
	for (size_t i = 0; i < jarea_files->size(); i++)
	{
		// read area data
		std::string fname = jarea_files->get(i)->get(u8"file")->str();
		std::ifstream fin(fname);

		if (!fin.good())
		{
			LOG_E(tag_) << "Failed to init area file:" << fname;
			delete jglobal;
			init_succeed = false;
			return;
		}

		JsonW* json = new JsonW(fin);
		fin.close();
		if (json->valid() == false)
		{
			// bad file content, go next
			LOG_E(tag_) << "Failed to init area file:" << fname;
			delete json;
			json = NULL;
			init_succeed = false;
			break;
		}
		else
		{

			// read all mobs
			JsonW* jmobs = json->get(u8"mobs");
			if (jmobs == NULL || jmobs->type() != JsonW::ARRAY )
			{
				// no mobs in this area
				delete json;
				json = NULL;
				continue;
			}

			CubePosition goffset;
			JsonW* joffset = json->get(u8"offset");
			if (joffset != NULL && joffset->type() == JsonW::ARRAY &&
				joffset->size() == 3)
			{
				goffset.set(
					(uint_fast32_t) joffset->get(0)->integer(),
					(uint_fast32_t) joffset->get(1)->integer(),
					(uint_fast32_t) joffset->get(2)->integer()
				);
			}

			for (size_t idx = 0; idx < jmobs->size(); idx++)
			{
				// read each mob
				JsonW* jmob = jmobs->get(idx);
				Mob* mob = new Mob( 
					(int) json->get(u8"id")->integer(),
					(int) idx + 1,
					jmob,
					mob_wiki,
					area_marks,
					cubes_,
					goffset);

				if (mob->valid() == false)
				{
					LOG_E(tag_) << "bad mob in area:" << json->get(u8"id")->integer() << " " << jmob->text();
					delete mob;
					delete json;
					continue;
				}

				auto itmob = world_mobs_.find(mob->id());
				if (itmob != world_mobs_.end())
				{
					LOG_E(tag_) << "duplicate mob in area:" << json->get(u8"id")->integer() << " " << jmob->text();
					delete mob;
					delete json;
					continue;
				}

				// assign to world_mobs_, cube_mobs_, and area_mobs_
				std::set<Creature*>* creature_set;

				world_mobs_[mob->id()] = mob;
				creature_set = add(mob->cube(), cube_mobs_, mob);
				mob->cube()->mobs_ = creature_set;
				add(mob->cube()->area(), area_mobs_, mob);
			}
		}

		delete json;
	}

	LOG_D(tag_) << "total mob: " << world_mobs_.size();

	// remove mob wiki
	for (auto itmob : mob_wiki)
	{
		delete itmob.second;
	}

    LOG_D(tag_) << "World() add global link " << global_link_count;
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

	if (init_succeed)
	{
		initialized_ = true;
	}
}

octillion::World::~World()
{
    LOG_D(tag_) << "~World() start";

    for (auto& it : players_)
    {
        Player* player = static_cast<Player*>(it.second);

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
        database_.save(player);

        // delete player from players_
		delete player;
    }

	for (auto& it : cmds_in_)
	{
		LOG_D(tag_) << "~World() delete cmds_in_ fd:" << it->fd();
		delete it;
	}

	for (auto& it : cmds_out_)
	{
		Command* cmd = it.second;
		LOG_D(tag_) << "~World() delete cmds_out_ fd:" << cmd->fd();
		delete cmd;
	}

    for (auto& it : cmds_)
    {
		Command* cmd = it.second;
        LOG_D(tag_) << "~World() delete cmd fd:" << cmd->fd();
        delete cmd;
    }

    for (auto&it : cube_players_)
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

	for (auto it : cube_mobs_)
	{
#ifdef MEMORY_DEBUG
		MemleakRecorder::instance().release(it.second);
#endif
		delete it.second;
	}

	for (auto it : area_mobs_)
	{
#ifdef MEMORY_DEBUG
		MemleakRecorder::instance().release(it.second);
#endif
		delete it.second;
	}

	for (auto it : world_mobs_)
	{
		delete static_cast<Mob*>(it.second);
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

std::error_code octillion::World::disconnect(int fd)
{
	LOG_D(tag_) << "disconnect starts";

    auto it = players_.find(fd);
    
    if ( it == players_.end())
    {
        LOG_W( tag_ ) << "disconnect() fd:" << fd << " does not exist in players_";
        return OcError::E_FATAL;
    }
    
    Player* player = static_cast<Player*>(it->second);
    
    if ( player == NULL )
    {
		// no player data in players_, possible reason:
		// 1. not login yet, 
		// 2. receive both LOGOUT and DISCONNECT command
        players_.erase( it );
        LOG_I( tag_ ) << "disconnect() fd:" << fd << " has no login record";
		return OcError::E_SUCCESS;
    }

    LOG_I( tag_ ) << "disconnect() fd:" << fd << " save data";

    // save player information
    database_.save( player );    

	// delete player from cube_players_, 
	// which keep track of all the players in one specific cube
	CubePosition loc = player->cube()->loc();
	auto itcube = cubes_.find(loc);

	if (itcube == cubes_.end())
	{
		// TODO: put player in any default cube
		LOG_E(tag_) << "disconnect(), Player " << player->id() << " try to leave from non-exist cube x:" << loc.x() << " y:" << loc.y() << " z:" << loc.z();
		return OcError::E_WORLD_BAD_CUBE_POSITION;
	}

	// remove from world_players_
	if (world_players_.erase(player) == 1)
	{
		LOG_D(tag_) << "disconnect(), remove player:" << player->id() << " from world_players_";
	}
	else
	{
		LOG_E(tag_) << "disconnect(), world_players_ does not contain player id:" << player->id();
	}

	Cube* cube = itcube->second;
	uint_fast32_t areaid = cube->area();

	// remove from area_players_
	erase(areaid, area_players_, player);

	// remove from cube_players_ and update cube player list
	cube->players_ = erase(cube, cube_players_, player);

	// remove the fd mapping from players_
	players_.erase(it);

	// delete player object from players_
	delete player;

	LOG_D(tag_) << "disconnect done";

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

    if (!initialized_)
    {
        LOG_E(tag_) << "cannot run tick() since World init failed";
        return OcError::E_FATAL;
    }
	
	// handle login commands in cmds_in_
	cmds_lock_.lock();
	for (auto& it : cmds_in_ )
	{
		int fd = it->fd();
		Command* cmd = it;
		connect(fd);
		delete cmd;
	}
	cmds_in_.clear();

	// we only generate event here, the actually 
	// logout is at the end of tick()
	for (auto& it : cmds_out_)
	{
		int fd = it.first;
		Command* cmd = it.second;

		auto itplayer = players_.find(fd);
		if (itplayer == players_.end())
		{
			continue;
		}

		Player* player = static_cast<Player*>(itplayer->second);

		if (player != NULL)
		{
			// create event if player was login before
			Event* event = new Event();
			event->range_ = Event::RANGE_CUBE;
			event->type_ = Event::TYPE_PLAYER_LOGOUT;
			event->player_ = player;
			event->eventcube_ = player->cube();
			events.push_back(event);
		}
	}

	cmds_lock_.unlock();

    // handle normal commands in cmds_
    cmds_lock_.lock();
    for (auto& it : cmds_)
    {
		Command* cmd = it.second;
        int fd = cmd->fd();
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
			case Command::GET_SERVER_VERSION:
				err = cmdGetServerVersion(fd, cmd, cmdback);
				break;
			case Command::GET_GLOBAL_DATA_STAMP:
				err = cmdGetGlobalDataStamp(fd, cmd, cmdback);
				break;
			case Command::GET_GLOBAL_DATA:
				err = cmdGetGlobalData(fd, cmd, cmdback);
				break;
			case Command::GET_AREA_DATA:
				err = cmdGetAreaData(fd, cmd, cmdback);
				break;
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
                // not possible
				err = OcError::E_FATAL;
				LOG_E(tag_) << "tick(), fatal, found LOGOUT in cmds_";
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
        delete it.second;
    }
    cmds_.clear();
    cmds_lock_.unlock();

	// handle player's tick
	for (auto itplayer : world_players_)
	{
		Player* player = static_cast<Player*>(itplayer);
		tick(player, events);
	}

	// handle monster moving, attacking, dead and reborn
	for (auto itmob : world_mobs_)
	{
		Mob* mob = static_cast<Mob*>(itmob.second);
		tick(mob, events);
	}

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
		else if (event->range_ == Event::RANGE_PRIVATE)
		{
			int fd = event->player_->fd();
			auto itplayer = players_.find(fd);
			if (itplayer != players_.end())
			{
				addjsons(event, static_cast<Player*>(itplayer->second), jsons);
			}
		}
        else // TODO: handle other range events
        {
            LOG_E(tag_) << "tick(), unexpected event range:" << event->range_;
        } // end of if (event->range_ == Event::RANGE_WORLD) 
    }

    // release plyevents
	// std::cerr << "events size " << events.size() << std::endl;
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

        LOG_I(tag_) << "write fd:" << fd << " json:" << utf8 << std::endl;

        delete jtext;
    }

	// actual delete without generate event
	cmds_lock_.lock();
	for (auto& it : cmds_out_)
	{
		int fd = it.first;
		Command* cmd = it.second;
		disconnect(fd);
		delete cmd;
	}
	cmds_out_.clear();
	cmds_lock_.unlock();

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

std::error_code octillion::World::cmdGetServerVersion(int fd, Command *cmd, JsonW* jback)
{
	jback->add(u8"cmd", cmd->cmd());
	jback->add(u8"err", Command::E_CMD_SUCCESS);
	jback->add(u8"version", global_version_);
	return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdGetGlobalDataStamp(int fd, Command *cmd, JsonW* jback)
{
	jback->add(u8"cmd", cmd->cmd());
	jback->add(u8"err", Command::E_CMD_SUCCESS);
	jback->add(u8"stamp", global_config_stamp_);
	return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdGetAreaData(int fd, Command *cmd, JsonW* jback)
{
	int areaid = (int)cmd->uiparms_[0];
	auto it = area_files_.find(areaid);
	if (it == area_files_.end())
	{
		jback->add(u8"cmd", cmd->cmd());
		jback->add(u8"err", Command::E_CMD_BAD_FORMAT);
		return OcError::E_SUCCESS;
	}
	
	std::string fname = it->second;
	std::ifstream fin(fname);
	if (!fin.good())
	{
		LOG_E(tag_) << "cmdGetAreaData(), failed to read area file:" << fname;
		jback->add(u8"cmd", cmd->cmd());
		jback->add(u8"err", Command::E_CMD_FILE_IO_ERROR);
		return OcError::E_SUCCESS;
	}

	JsonW* jarea = new JsonW(fin);

	if (jarea->valid() == false)
	{
		LOG_E(tag_) << "cmdGetAreaData(), invalid jaon area file:" << fname;
		jback->add(u8"cmd", cmd->cmd());
		jback->add(u8"err", Command::E_CMD_FILE_IO_ERROR);
		return OcError::E_SUCCESS;
	}

	jback->add(u8"cmd", cmd->cmd());
	jback->add(u8"err", Command::E_CMD_SUCCESS);
	jback->add(u8"data", jarea);

	return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdGetGlobalData(int fd, Command *cmd, JsonW* jback)
{
	// read global data
	std::ifstream fin(global_config_file_);
	if (!fin.good())
	{
		LOG_E(tag_) << "cmdGetGlobalData(), Failed to open " << global_config_file_;
		return OcError::E_FATAL;
	}

	JsonW* jglobal = new JsonW(fin);
	if (jglobal->valid() == false)
	{
		LOG_E(tag_) << "cmdGetGlobalData(), Failed to read " << global_config_file_;
		return OcError::E_FATAL;
	}

	jback->add(u8"cmd", cmd->cmd());
	jback->add(u8"err", Command::E_CMD_SUCCESS);
	jback->add(u8"data", jglobal);
	return OcError::E_SUCCESS;
}

std::error_code octillion::World::cmdUnknown(int fd, Command *cmd, JsonW* jback)
{
    LOG_D(tag_) << "cmdUnknown start";
    jback->add(u8"cmd", cmd->cmd());
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

    Player* player = static_cast<Player*>(it->second);

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

    CubePosition loc, loc_reborn;
    err = database_.create(fd, player, loc, loc_reborn);

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

	auto citreborn = cubes_.find(loc_reborn);
	if (citreborn == cubes_.end())
	{
		LOG_E(tag_) << "cmdConfirmUser, reborn pos " << loc_reborn.str() << " does not exist";
		delete player;
		return OcError::E_WORLD_BAD_CUBE_POSITION;
	}
	player->cube_reborn(citreborn->second);

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

	// check if username is still in the world
	for (auto itplayer : world_players_)
	{
		Player* player = static_cast<Player*>(itplayer);
		if (player->username() == username)
		{
			LOG_W(tag_) << "cmdLogin, failed, " << username << " has not logout yet";
			return OcError::E_PROTOCOL_FD_DUPLICATE_LOGIN;
		}
	}
  
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
        database_.save(static_cast<Player*>(it->second));
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
    CubePosition loc, loc_reborn;
    err = database_.load( pcid, player, loc, loc_reborn);
    if (err != OcError::E_SUCCESS)
    {
        LOG_E(tag_) << "cmdLogin, database_.load pcid:" << pcid << " return err:" << err;
        delete player;
        return err;
    }


	// validate reborn cube, if bad, set to global reborn cube
	auto citreborn = cubes_.find(loc_reborn);
	if (citreborn == cubes_.end())
	{
		LOG_W(tag_) << "cmdLogin, user:" << player->username() << " " << player->id()
			<< " reborn position " << loc.str() << " does not exist";
		player->cube_reborn(global_reborn_cube_);
	}
	else
	{
		player->cube_reborn(citreborn->second);
	}
	
	// validate player locates cube, if bad, set to reborn cube
    auto cit = cubes_.find(loc);
    if (cit == cubes_.end())
    {
        LOG_W(tag_) << "cmdLogin, user:" << player->username() << " " << player->id() 
			<< " login position " << loc.str() << " does not exist";
		player->cube(player->cube_reborn());
    }
	else
	{
		player->cube(cit->second);
	}

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
    else if (pit->second == NULL)
    {
        LOG_E(tag_) << "cmdMove, failed since fd:" << fd << " not login yet";
        return OcError::E_PROTOCOL_FD_LOGOUT;
    }

    // check player's location and the target location
    Player* player = static_cast<Player*>(pit->second);
    CubePosition targetloc(player->cube()->loc(), cmd->uiparms_[0]);

    // check if cube exist
    auto cit = cubes_.find(targetloc);
    if (cit == cubes_.end())
    {
        LOG_E(tag_) << "cmdMove, player moves to invalid position " << targetloc.str();
        return OcError::E_WORLD_BAD_CUBE_POSITION;
    }

    // TODO: check player's status to see if moveable or not

    // create cube event
    Event* event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_ARRIVE;
    event->player_ = player;
    event->eventcube_ = cit->second;
    event->subcube_ = player->cube();
    event->direction_ = Cube::opposite_dir(cmd->uiparms_[0]);
    events.push_back(event);
	
	event = new Event();
	event->range_ = Event::RANGE_CUBE;
	event->type_ = Event::TYPE_PLAYER_ARRIVE_PRIVATE;
	event->player_ = player;
	event->eventcube_ = cit->second;
	event->subcube_ = player->cube();
	event->direction_ = Cube::opposite_dir(cmd->uiparms_[0]);
	events.push_back(event);

    event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_LEAVE;
	event->player_ = player;
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
	if (cmd == NULL)
	{
		LOG_W(tag_) << "addcmd() cmd is NULL";
		return;
	}

	if (cmd->cmd() == Command::CONNECT)
	{
		cmds_lock_.lock();
		cmds_in_.insert(cmd);
		cmds_lock_.unlock();
	}
	else if (cmd->cmd() == Command::DISCONNECT || cmd->cmd() == Command::LOGOUT)
	{
		cmds_lock_.lock();

		int fd = cmd->fd();
		auto it = cmds_out_.find(fd);
		if (it != cmds_out_.end())
		{
			LOG_D(tag_) << "addcmd() detect multiple disconnect and logout cmd";
		}
		else
		{
			cmds_out_[fd] = cmd;
		}
		
		cmds_lock_.unlock();
	}
	else
	{
		cmds_lock_.lock();
		auto it = cmds_.find(cmd->fd());
		if (it == cmds_.end())
		{
			cmds_[cmd->fd()] = cmd;
		}
		else
		{
			LOG_W(tag_) << "addcmd() multiple cmds for same fd: " << cmd->fd() << " drop it";
			delete cmd;
		}
		cmds_lock_.unlock();
	}
}

std::error_code octillion::World::enter(Player* player, std::list<Event*>& events)
{
    LOG_D(tag_) << "enter start";

    // put player in the world_players_
    world_players_.insert(player);

	// put player in the area_players_
	add(player->cube()->area(), area_players_, player);

	// put player in the cube_players_, and assign the shortcut to cube
	player->cube()->players_ = add(player->cube(), cube_players_, player);

    // create event
    Event* event = new Event();
    event->range_ = Event::RANGE_CUBE;
    event->type_ = Event::TYPE_PLAYER_LOGIN;
	event->player_ = player;
    event->eventcube_ = player->cube();
    events.push_back(event);

	event = new Event();
	event->range_ = Event::RANGE_CUBE;
	event->type_ = Event::TYPE_PLAYER_LOGIN_PRIVATE;
	event->player_ = player;
	event->eventcube_ = player->cube();
	events.push_back(event);

    LOG_D(tag_) << "enter done";

    return OcError::E_SUCCESS;
}

void octillion::World::addjsons(Event* event, std::set<Creature*>* players, std::map<int, JsonW*>& jsons)
{
    for (auto& player : *players)
    {
		addjsons(event, static_cast<Player*>(player), jsons);
    } // for (auto& player : *players)
} // void octillion::World::addjsons

void octillion::World::addjsons(Event* event, Player* player, std::map<int, JsonW*>& jsons)
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
} // void octillion::World::addjsons

// help function, move player location and change cube_players_, and area_players_
// function WILL NOT check the newloc's existence
void octillion::World::move(Player* player, Cube* cube_to)
{
    Cube* cube_from = player->cube();

    int area_from = cube_from->area();
    int area_to = cube_to->area();

	// move player in area_players_
	if (area_from != area_to)
	{
		erase(area_from, area_players_, player);
		add(area_to, area_players_, player);
	}

	// move player in cube_players_
	erase(cube_from, cube_players_, player);
	cube_to->players_ = add(cube_to, cube_players_, player);

	// move player
    player->cube(cube_to);
}

octillion::Cube* octillion::World::readloc(
    JsonW* json,
    const std::map<int, std::map<std::string, Cube*>*>& area_marks,
    const std::map<CubePosition, Cube*>& cubes
)
{
    int areaid;
    JsonW* jarea = json->get(u8"area");
    JsonW* jloc = json->get(u8"cube");    
    if (jarea == NULL || jarea->valid() == false || jarea->type() != JsonW::INTEGER )
    {
        areaid = 0;
    }
    else
    {
        areaid = (int)jarea->integer();
    }

    if (jloc->type() == JsonW::STRING && areaid > 0 )
    {
        std::map<std::string, Cube*>* markmap;
        auto itmark = area_marks.find(areaid);
        if (itmark == area_marks.end())
        {
            LOG_E(tag_) << "readloc() areaid:" << areaid << " is not in area_marks";
            return NULL;
        }
        markmap = itmark->second;

        auto it = markmap->find(jloc->str());
        if (it == markmap->end())
        {
            LOG_E(tag_) << "readloc() mark:" << jloc->str() << " is not in markmap";
            return NULL;
        }

        return it->second;
    }
    else if (jloc->type() == JsonW::ARRAY)
    {
        JsonW* joffset = jloc->get(u8"offset");
        bool ret;
        CubePosition offset;
        CubePosition cubepos;

        if (joffset != NULL && joffset->type() == JsonW::ARRAY &&
            joffset->size() == 3)
        {
            offset.set(
                (uint_fast32_t) joffset->get(0)->integer(),
                (uint_fast32_t) joffset->get(1)->integer(),
                (uint_fast32_t) joffset->get(2)->integer());
        }

        ret = Area::readloc(jloc, cubepos, offset.x(), offset.y(), offset.z());
        if (ret == false)
        {
            LOG_E(tag_) << "readloc(), Area::readloc() failed";
            return NULL;
        }

        auto itcube = cubes.find(cubepos);
        if (itcube == cubes.end())
        {
            LOG_E(tag_) << "cubepos " << cubepos.str() << " not in cubes";
            return NULL;
        }
        return itcube->second;
    }
    else
    {
        LOG_E(tag_) << "readloc() bad json type";
        return NULL;
    }
}

void octillion::World::reborn(Player* player, std::list<Event*>& events)
{
	Event* event = new Event();
	event->range_ = Event::RANGE_CUBE;
	event->type_ = Event::TYPE_PLAYER_DEAD;
	event->player_ = player;
	event->eventcube_ = player->cube();
	events.push_back(event);

	// move player to reborn cube
	move(player, player->cube_reborn());

	// reborn player
	player->status(Player::STATUS_IDLE);
	player->hp(100);

	// cube event
	event = new Event();
	event->range_ = Event::RANGE_CUBE;
	event->type_ = Event::TYPE_PLAYER_REBORN;
	event->player_ = player;
	event->eventcube_ = player->cube();
	events.push_back(event);

	// private event
	event = new Event();
	event->range_ = Event::RANGE_PRIVATE;
	event->type_ = Event::TYPE_PLAYER_REBORN_PRIVATE;
	event->player_ = player;
	event->eventcube_ = player->cube();
	events.push_back(event);
}

std::error_code octillion::World::tick(Player* player, std::list<Event*>& events)
{
	// handle combat
	if (player->status() == Player::STATUS_COMBAT)
	{
		Mob* mob = static_cast<Mob*>(player->target());
		if (mob == NULL)
		{
			LOG_W(tag_) << "player " << player->id() << " combat target is null";
			player->status(Player::STATUS_IDLE);
		}
		else
		{
			// decrease mob's hp
			int dmg = 1;
			mob->hp(mob->hp() - dmg);

			// player attack event
			if (mob->hp() > 0)
			{
				Event* event = new Event();
				event->type_ = Event::TYPE_PLAYER_ATTACK;
				event->range_ = Event::RANGE_CUBE;
				event->eventcube_ = player->cube();
				event->mob_ = mob;
				event->i32parm_ = dmg;
				events.push_back(event);
			}
			else
			{
				mob->status(Mob::STATUS_GHOST);
				player->status(Player::STATUS_IDLE);

				Event* event = new Event();
				event->type_ = Event::TYPE_MOB_DEAD;
				event->range_ = Event::RANGE_CUBE;
				event->eventcube_ = player->cube();
				event->mob_ = mob;
				event->player_ = player;
				event->i32parm_ = dmg;
				events.push_back(event);
			}
		}
	}

	return OcError::E_SUCCESS;
}

std::error_code octillion::World::tick(Mob* mob, std::list<Event*>& events)
{
	// get the player list in the same cube
	std::set<Creature*>* players = NULL;
	auto itplayers = cube_players_.find(mob->cube());
	if (itplayers != cube_players_.end())
	{
		players = itplayers->second;
	}

	// check reborn count
	if (mob->status() == Mob::STATUS_GHOST)
	{
		int next_reborn_count = mob->next_reborn_count() - 1;
		if (next_reborn_count <= 0)
		{
			mob->next_reborn_count(
				mob->max_reborn_tick()
			);
			mob->status(Mob::STATUS_IDLE);

			// mob reborn event
			Event* event = new Event();
			event->type_ = Event::TYPE_MOB_REBORN;
			event->range_ = Event::RANGE_CUBE;
			event->mob_ = mob;
			events.push_back(event);
		}
		else
		{
			mob->next_reborn_count(
				next_reborn_count
			);
		}
	}

	// get the highest lv and lowerest lv in the same cube
	if (mob->status() == Mob::STATUS_IDLE && players != NULL && players->size() > 0)
	{
		Player *plstrong = NULL, *plweak = NULL;
		for (auto it : *players)
		{
			if (plstrong == NULL)
			{
				plstrong = static_cast<Player*>(it);
				plweak = static_cast<Player*>(it);
			}
			else
			{
				Player* pl = static_cast<Player*>(it);
				if (plstrong->lvl() < pl->lvl())
				{
					plstrong = pl;
				}
				if (plweak->lvl() > pl->lvl())
				{
					plweak = pl;
				}
			}
		}

		switch (mob->combat())
		{
		case Mob::COMBAT_DUMMY: // never attack player
		case Mob::COMBAT_PEACE: // never automatically attack player 
			break;
		case Mob::COMBAT_COWARD: // run away if player is stronger than mob
			if (mob->lvl() <= plstrong->lvl())
			{
				// run aways!
				mob->next_move_count(0);
			}
			break;
		case Mob::COMBAT_AGGRESSIVE: // fight if player is weaker or equally level
			if (mob->lvl() >= plweak->lvl())
			{
				// attack the plweak!
				mob->target(static_cast<Creature*>(plweak));
				mob->status( Mob::STATUS_COMBAT );
				plweak->target(static_cast<Creature*>(mob));
				plweak->status(Player::STATUS_COMBAT);
				combat_mobs_.insert(mob);
				combat_players_.insert(plweak);
			}
			break;
		case Mob::COMBAT_CRAZY: // fight when player appears
						   // attack the plweak!
			mob->target(static_cast<Creature*>(plweak));
			mob->status(Mob::STATUS_COMBAT);
			plweak->target(static_cast<Creature*>(mob));
			plweak->status(Player::STATUS_COMBAT);
			combat_mobs_.insert(mob);
			combat_players_.insert(plweak);

			break;
		}
	}

	// combat handler
	if (mob->status() == Mob::STATUS_COMBAT)
	{
		Player* target = static_cast<Player*>(mob->target());

		if (target == NULL)
		{
			LOG_E(tag_) << "mobs target is NULL";
			mob->status(Mob::STATUS_IDLE);
		}
		else if (target->cube() != mob->cube())
		{
			LOG_E(tag_) << "mob " << mob->id() << " target is in other cube";
			mob->status(Mob::STATUS_IDLE);
		}
		else
		{
			int_fast32_t plhp = target->hp();
			int_fast32_t mobattack = 1;

			if (target->status() != Player::STATUS_COMBAT)
			{
				target->status(Player::STATUS_COMBAT);
				target->target(mob);
			}

			if (plhp < mobattack)
			{
				plhp = 0;

				target->hp(plhp);

				Event* event = new Event();
				event->type_ = Event::TYPE_MOB_ATTACK;
				event->range_ = Event::RANGE_PRIVATE;
				event->player_ = target;
				event->mob_ = mob;
				event->i32parm_ = mobattack;
				events.push_back(event);

				// erase the player from combat_players
				combat_players_.erase(target);

				// erase the mob from combat_players
				combat_mobs_.erase(mob);
				mob->status(Mob::STATUS_IDLE);

				// send player back to reset point
				reborn( target, events);
			}
			else
			{
				plhp = plhp - mobattack;

				target->hp(plhp);

				Event* event = new Event();
				event->type_ = Event::TYPE_MOB_ATTACK;
				event->range_ = Event::RANGE_PRIVATE;
				event->player_ = target;
				event->mob_ = mob;
				event->i32parm_ = mobattack;
				events.push_back(event);
			}
		}
	}

	// move handler, mob can only move when in status idle or ghost
	if (mob->status() == Mob::STATUS_IDLE || mob->status() == Mob::STATUS_GHOST )
	{
		Cube* dest = NULL;
		
		unsigned int next_move_count = mob->next_move_count();
		unsigned int max_move_tick = mob->max_move_tick();
		unsigned int min_move_tick = mob->min_move_tick();
		
		if (mob->random_path() == true)
		{			
			if (next_move_count == 0)
			{
				// move randomly, move to the cube that allows mob
				// TODO: how about other type?
				int dir = mob->cube()->random_exit(Cube::MOB_CUBE);
				dest = mob->cube()->find(cubes_, dir);
				mob->next_move_count(
					(rand() % (max_move_tick - min_move_tick + 1))
					+ min_move_tick);			
			}
			else
			{
				mob->next_move_count(next_move_count - 1);
			}
		}
		else if (mob->path_size() > 0)
		{
			if (next_move_count == 0)
			{
				dest = mob->next_path();
				mob->next_move_count(
					(rand() % (max_move_tick - min_move_tick + 1))
					+ min_move_tick);
			}
			else
			{
				mob->next_move_count(next_move_count - 1);
			}
		}
		else
		{
			// not allow to move
		}

		if (dest != NULL && mob->status() != Mob::STATUS_GHOST )
		{
			// LOG_I(tag_) << "m" << mob->id() << " " << mob->loc()->loc().str() << "->" << dest->loc().str();
			// generate event
			if (mob->status() != Mob::STATUS_GHOST)
			{
				Event* event = new Event();
				event->type_ = Event::TYPE_MOB_LEAVE;
				event->range_ = Event::RANGE_CUBE;
				event->eventcube_ = mob->cube();
				event->subcube_ = dest;
				event->mob_ = mob;
				events.push_back(event);

				event = new Event();
				event->type_ = Event::TYPE_MOB_ARRIVE;
				event->range_ = Event::RANGE_CUBE;
				event->eventcube_ = dest;
				event->subcube_ = mob->cube();
				event->mob_ = mob;
				events.push_back(event);
			}

			// change cube's mob list
			mob->cube()->mobs_ = erase(mob->cube(), cube_mobs_, mob);
			dest->mobs_ = add(dest, cube_mobs_, mob);

			// change area's mob list if needed
			if (mob->cube()->area() != dest->area())
			{
				erase(mob->cube()->area(), area_mobs_, mob);
				add(dest->area(), area_mobs_, mob);
			}

			// move mob
			mob->cube(dest);
		}
	}

	return OcError::E_SUCCESS;
}
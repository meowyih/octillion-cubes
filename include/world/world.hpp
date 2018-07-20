#ifndef OCTILLION_WORLD_HEADER
#define OCTILLION_WORLD_HEADER

#include <map>
#include <set>
#include <list>
#include <mutex>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"

#include "world/cube.hpp"
#include "world/creature.hpp"
#include "world/player.hpp"
#include "world/mob.hpp"
#include "world/command.hpp"
#include "world/event.hpp"

#include "database/database.hpp"
#include "database/filedatabase.hpp"

#include "jsonw/jsonw.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class World;
}

class octillion::World
{
private:
	const std::string global_version_ = "0.01";
	const std::string global_config_file_ = "data/_global.json";

private:
    const static std::string tag_;

public:
    //Singleton
    static World& get_instance()
    {
        static World instance;
        return instance;
    }

    // avoid accidentally copy
    World(World const&) = delete;
    void operator = (World const&) = delete;

private:
    World();
    ~World();

public:    
    void addcmd(Command* cmd);

public:
    std::error_code tick();
	std::error_code tick(Mob* mob, std::list<Event*>& events);
	std::error_code tick(Player* player, std::list<Event*>& events);

private:
    std::error_code cmdUnknown(int fd, Command *cmd, JsonW* jsonobject);

	// server version query and data update
	std::error_code cmdGetServerVersion(int fd, Command *cmd, JsonW* jsonobject);
	std::error_code cmdGetGlobalDataStamp(int fd, Command *cmd, JsonW* jsonobject);
	std::error_code cmdGetGlobalData(int fd, Command *cmd, JsonW* jsonobject);
	std::error_code cmdGetAreaData(int fd, Command *cmd, JsonW* jsonobject);

    // create and login
	// there is no logout function, it is stored cmds_out_ and handled by disconnect()
    std::error_code cmdValidateUsername(int fd, Command *cmd, JsonW* jsonobject);
    std::error_code cmdConfirmUser(int fd, Command* cmd, JsonW* jsonobject, std::list<Event*>& events);
    std::error_code cmdLogin(int fd, Command* cmd, JsonW* jsonobject, std::list<Event*>& events );

    // move
    std::error_code cmdMove(int fd, Command* cmd, JsonW* jsonobject, std::list<Event*>& events);

    // god's command
    std::error_code cmdFreezeWorld(int fd, Command* cmd, JsonW* jsonobject);

private:
    std::error_code connect(int fd);
    std::error_code disconnect(int fd);

    // enter and leave is to registry and unregistry one player to the world
    // this two function should be called after player exists in players_ map
    // players enters the world after login
    std::error_code enter(Player* player, std::list<Event*>& events);

    // help function, create/add json object based on event
    inline void addjsons(Event* event, std::set<Creature*>* players, std::map<int, JsonW*>& jsons);
	inline void addjsons(Event* event, Player* players, std::map<int, JsonW*>& jsons);

    // help function, move player location and change cube_players_, and area_players_
    // function WILL NOT check the newloc's existence
    inline void move(Player* player, Cube* cube_to);

	// help function, reborn the player
	inline void reborn(Player* player, std::list<Event*>& events);

	// help function, add creature into a cube/area creature map
	inline static std::set<Creature*>* add(Cube* cube, std::map<Cube*, std::set<Creature*>*>& map, Creature* creature)
	{
		auto it = map.find(cube);
		if (it == map.end())
		{
			std::set<Creature*>* cset = new std::set<Creature*>();
#ifdef MEMORY_DEBUG
			MemleakRecorder::instance().alloc(__FILE__, __LINE__, cset);
#endif
			cset->insert(creature);
			map[cube] = cset;
			return cset;
		}
		else
		{
			std::set<Creature*>* cset = it->second;
			cset->insert(creature);
			return cset;
		}
	}

	// help function, add creature into a cube/area creature map
	inline static std::set<Creature*>* erase(Cube* cube, std::map<Cube*, std::set<Creature*>*>& map, Creature* creature)
	{
		auto it = map.find(cube);
		if (it == map.end())
		{
			return NULL;
		}
		else
		{
			std::set<Creature*>* cset = it->second;
			cset->erase(creature);

			if (cset->size() == 0)
			{				
#ifdef MEMORY_DEBUG
				MemleakRecorder::instance().release(cset);
#endif
				delete cset;
				map.erase(it);
				return NULL;
			}

			return cset;
		}
	}

	inline static std::set<Creature*>* add(int areaid, std::map<int, std::set<Creature*>*>& map, Creature* creature)
	{
		auto it = map.find(areaid);
		if (it == map.end())
		{
			std::set<Creature*>* cset = new std::set<Creature*>();
#ifdef MEMORY_DEBUG
			MemleakRecorder::instance().alloc(__FILE__, __LINE__, cset);
#endif
			cset->insert(creature);
			map[areaid] = cset;
			return cset;
		}
		else
		{
			std::set<Creature*>* cset = it->second;
			cset->insert(creature);
			return cset;
		}
	}

	inline static std::set<Creature*>* erase(int areaid, std::map<int, std::set<Creature*>*>& map, Creature* creature)
	{
		auto it = map.find(areaid);
		if (it == map.end())
		{
			return NULL;
		}
		else
		{
			std::set<Creature*>* cset = it->second;
			cset->erase(creature);

			if (cset->size() == 0)
			{
#ifdef MEMORY_DEBUG
				MemleakRecorder::instance().release(cset);
#endif
				delete cset;
				map.erase(it);
				return NULL;
			}

			return cset;
		}
	}

    // read a Cube pointer from global json
    static Cube* readloc(
        JsonW* json,
        const std::map<int, std::map<std::string, Cube*>*>& area_marks,
        const std::map<CubePosition, Cube*>& cubes
    );

private:
    bool initialized_ = false;
    std::mutex cmds_lock_;
    std::map<int, Command*> cmds_; // fd and standard cmd
	std::set<Command*> cmds_in_; // connect cmd
	std::map<int, Command*> cmds_out_; // fd and logout / disconnect cmd
	std::string global_config_stamp_;

private:
	std::map<int, std::string> area_files_;
    std::set<Area*> areas_;
    std::map<CubePosition, Cube*> cubes_; // shortcut to all cubes, DO NOT release the cube.
	Cube* global_reborn_cube_ = NULL;
    
    std::map<int, Creature*> players_;
    std::set<Creature*> world_players_; // shortcut to the players in the world, DO NOT release the player.
    std::map<Cube*, std::set<Creature*>*> cube_players_; // shortcut to the players in specific cube, DO NOT release the player.
    std::map<int, std::set<Creature*>*> area_players_; // shortcut to the players in specific area, DO NOT release the player.

	std::map<uint_fast32_t, Creature*> world_mobs_;
	std::map<Cube*, std::set<Creature*>*> cube_mobs_; // shortcut to the mobs in specific cube, DO NOT release the mobs.
	std::map<int, std::set<Creature*>*> area_mobs_; // shortcut to the mobs in specific area, DO NOT release the mobs.

	std::set<Creature*> combat_players_;
	std::set<Creature*> combat_mobs_;

private:
    FileDatabase database_;

#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__,__LINE__, memory);

        return memory;
    }

    static void* operator new[](size_t size)
    {
        void* memory = MALLOC(size);

        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);

        return memory;
    }

    static void operator delete(void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }

    static void operator delete[](void* p)
    {
        MemleakRecorder::instance().release(p);
        FREE(p);
    }
#endif
};

#endif

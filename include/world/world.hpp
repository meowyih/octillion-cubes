#ifndef OCTILLION_WORLD_HEADER
#define OCTILLION_WORLD_HEADER

#include <map>
#include <set>
#include <list>
#include <mutex>

#include "error/macrolog.hpp"
#include "error/ocerror.hpp"

#include "world/tick.hpp"
#include "world/cube.hpp"
#include "world/player.hpp"
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

class octillion::World : octillion::Tick, octillion::TickCallback
{
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
    virtual std::error_code tick() override;
    virtual void tickcallback(
        uint32_t type,
        uint32_t param1,
        uint32_t param2) override;

private:
    std::error_code cmdUnknown(int fd, Command *cmd, JsonObjectW* jsonobject);
    std::error_code cmdValidateUsername(int fd, Command *cmd, JsonObjectW* jsonobject);
    std::error_code cmdConfirmUser(int fd, Command* cmd, JsonObjectW* jsonobject, std::list<Event*>& events);
    std::error_code cmdLogin(int fd, Command* cmd, JsonObjectW* jsonobject, std::list<Event*>& events );
    std::error_code cmdLogout(int fd, Command* cmd, JsonObjectW* jsonobject, std::list<Event*>& events);
    std::error_code cmdFreezeWorld(int fd, Command* cmd, JsonObjectW* jsonobject);

private:
    std::error_code connect(int fd);
    std::error_code disconnect(int fd, std::list<Event*>& events);

    // enter and leave is to registry and unregistry one player to the world
    // this two function should be called after player exists in players_ map
    // players enters the world after login
    std::error_code enter(Player* player, std::list<Event*>& events);

    // player leaves the world when logout, disconnect or the world has been destroyed
    std::error_code leave(Player* player, std::list<Event*>& events);

    // help function, create/add json object based on event
    inline void addjsons(Event* event, std::set<Player*>* players, std::map<int, JsonTextW*>& jsons);

private:
    std::mutex cmds_lock_;
    std::list<Command*> cmds_; // fd and Command*

private:
    std::set<Area*> areas_;
    std::map<CubePosition, Cube*> cubes_; // shortcut to all cubes, DO NOT release the cube.
    
    std::map<int, Player*> players_;
    std::set<Player*> world_players_; // shortcut to the players in the world, DO NOT release the player.
    std::map<Cube*, std::set<Player*>*> cube_players_; // shortcut to the players in specific cube, DO NOT release the player.
    std::map<int, std::set<Player*>*> area_players_; // shortcut to the players in specific area, DO NOT release the player.

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

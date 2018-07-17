#ifndef OCTILLION_EVENT_HEADER
#define OCTILLION_EVENT_HEADER

#include "jsonw/jsonw.hpp"
#include "world/player.hpp"
#include "world/cube.hpp"
#include "world/mob.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class Event;
}

class octillion::Event
{
public:
	const static int RANGE_NONE = 0;
	const static int RANGE_PRIVATE = 1;
	const static int RANGE_CUBE = 2;
	const static int RANGE_AREA = 3;
	const static int RANGE_WORLD = 4;

	const static int TYPE_UNKNOWN = 0;

	const static int TYPE_PLAYER_LOGIN = 1;
	const static int TYPE_PLAYER_LOGOUT = 2;
	const static int TYPE_PLAYER_ARRIVE = 10;
	const static int TYPE_PLAYER_LEAVE = 11;
	const static int TYPE_PLAYER_DEAD = 12;
	const static int TYPE_PLAYER_REBORN = 13;

	const static int TYPE_MOB_REBORN = 20;
	const static int TYPE_MOB_DEAD = 21;
	const static int TYPE_MOB_ARRIVE = 22;
	const static int TYPE_MOB_LEAVE = 23;

	const static int TYPE_MOB_ATTACK = 24;

	const static int TYPE_JSON_SIMPLE = 1;
	const static int TYPE_JSON_SIMPLE_WITH_LOC = 2;
	const static int TYPE_JSON_DETAIL = 3;
	const static int TYPE_JSON_DETAIL_WITH_LOC = 4;

public:
    JsonW* json();

    // player_ is an object that might be deleted after logout, so
    // we need to make a copy in event
    void player(const Player& player)
    {
        player_ = player;
    }

public:
    int range_ = RANGE_NONE;
    int type_ = TYPE_UNKNOWN;
    Player player_;
    int areaid_;
    Cube* eventcube_ = NULL;
    Cube* subcube_ = NULL;
    int direction_;
	Mob* mob_;
	int_fast32_t i32parm;


#ifdef MEMORY_DEBUG
public:
    static void* operator new(size_t size)
    {
        void* memory = MALLOC(size);
        MemleakRecorder::instance().alloc(__FILE__, __LINE__, memory);
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
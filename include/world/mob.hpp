#ifndef OCTILLION_MOBS_HEADER
#define OCTILLION_MOBS_HEADER

#include <string>
#include <vector>
#include <list>
#include <system_error>

#include "error/macrolog.hpp"

#include "jsonw/jsonw.hpp"

#include "world/cube.hpp"
#include "world/creature.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

namespace octillion
{
    class Mob;
}

class octillion::Mob : public octillion::Creature
{
private:
    const std::string tag_ = "Mob";

public:
    // construct a general mob only based on race, subrace and level
    // default path is random, combat_type is peace, 
    // short and long mob's description is race name
    // Mob(int race, int subrace, int level);

	// read default mob information stores in _global.json
	Mob(const JsonW* jmob);

    // read a mob information from json
    Mob( int areaid, int order,
		const JsonW* jmob,
		const std::map<uint_fast32_t, Mob*>& default_config,
		const std::map<int, std::map<std::string, Cube*>*>& area_marks,
		const std::map<CubePosition, Cube*>& cubes, 
		CubePosition goffset );

public:
	const static int COMBAT_DUMMY = 0;
	const static int COMBAT_COWARD = 1;
	const static int COMBAT_PEACE = 2;
	const static int COMBAT_AGGRESSIVE = 3;
	const static int COMBAT_CRAZY = 4;

	const static int STATUS_IDLE = 0;
	const static int STATUS_COMBAT = 1;
	const static int STATUS_GHOST = 2;

	// json mask
	const static uint_fast32_t J_HP = 0x1;
	const static uint_fast32_t J_CUBE = 0x2;
	const static uint_fast32_t J_ATTR = 0x4;
	const static uint_fast32_t J_DISPLAY = 0x8;

public:
	uint_fast32_t id(); // global unique id
	uint_fast32_t type(); // global type id

	Cube* cube() { return cube_; }
	int status() { return status_; }
	int combat() { return combat_; }
	int lvl() { return lvl_; }
	int next_move_count() { return next_move_count_; }
	int min_move_tick() {
		return min_move_tick_;
	}
	int max_move_tick() {
		return max_move_tick_;
	}
	int max_reborn_tick() { return max_reborn_tick_; }
	int next_reborn_count() { return next_reborn_count_; }
	bool valid() { return valid_; }
	void* target() { return target_; }
	bool random_path() { return random_path_; }
	size_t path_size() { return path_.size(); }
	Cube* next_path();

	void status(int status);
	void next_move_count(unsigned int next_move_count) {
		next_move_count_ = next_move_count;
	}
	void next_reborn_count(unsigned int next_reborn_count) {
		next_reborn_count_ = next_reborn_count;
	}

	void cube(Cube* cube) { cube_ = cube; }
	void target(Creature* target) { target_ = target; }

	// parameter type
	// 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
	// 2 - Event::TYPE_JSON_DETAIL, for detail event usage
	JsonW* json(uint_fast32_t type) override;

private:
	static Cube* readloc(
		int areaid,
		const JsonW* jloc,
		const std::map<int, std::map<std::string, Cube*>*>& area_marks, 
		const std::map<CubePosition, Cube*>& cubes,
		CubePosition goffset );
private:
    // mob's information
	int status_ = STATUS_IDLE;
	int area_ = 0;
	int id_ = 0;
    std::string short_;
    std::string long_;
    int race_ = 0;
    int subrace_ = 0;
    int lvl_ = 1;
    int combat_ = 0;

	bool valid_ = false;

	Cube* cube_;

	// attacking player
	Creature* target_ = NULL; 

    // path related
    bool random_path_ = false;
    int min_move_tick_ = 2; // default value 30 sec per move
	int max_move_tick_ = 2;
	int max_reborn_tick_ = 10;
	int next_reborn_count_ = 1000;
	int next_move_count_ = 2;
	int path_idx_ = 0;
    std::vector<Cube*> path_;

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
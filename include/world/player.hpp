#ifndef OCTILLION_PLAYER_HEADER
#define OCTILLION_PLAYER_HEADER

#include <cstdint>
#include <ostream>
#include <map>
#include <string>

#include "world/cube.hpp"
#include "world/creature.hpp"
#include "jsonw/jsonw.hpp"

#ifdef MEMORY_DEBUG
#include "memory/memleak.hpp"
#endif

// classes - skiller, believer
// gender - male, female, neutral
// attribute - Constitution, Mental, Luck, Charming

namespace octillion
{
    class Player;
}

class octillion::Player : public octillion::Creature
{
private:
	const std::string tag_ = "Player";

public:
	// Gender
	const static int GENDER_NEUTRAL = 0;
	const static int GENDER_MALE = 1;
	const static int GENDER_FEMALE = 2;

	// Class
	const static int CLS_SKILLER = 1;
	const static int CLS_BELIEVER = 2;

	// status
	const static int STATUS_IDLE = 0;
	const static int STATUS_COMBAT = 1;

	// json mask
	const static uint_fast32_t J_HP = 0x1;
	const static uint_fast32_t J_CUBE = 0x2;
	const static uint_fast32_t J_ATTR = 0x4;
	const static uint_fast32_t J_DISPLAY = 0x8;

public:

	Player() { hp_ = 100; maxhp_ = hp_; }

    Player& operator = (const Player& rhs)
    {
        status_ = rhs.status_;
        id_ = rhs.id_;

        gender_ = rhs.gender_;
        cls_ = rhs.cls_;

        con_ = rhs.con_;
        men_ = rhs.men_;
        luc_ = rhs.luc_;
        cha_ = rhs.cha_;
		hp_ = rhs.hp_;
		maxhp_ = rhs.maxhp_;
        cube_ = rhs.cube_;
		cube_reborn_ = rhs.cube_reborn_;

        username_ = rhs.username_;
        password_ = rhs.password_;

		fd_ = rhs.fd_;
		target_ = rhs.target_;

        return *this;
    }

public:
    std::string username() { return username_; }
    std::string password() { return password_; }
    int status() { return status_; }
    int_fast32_t id() { return id_; }
    int cls() { return cls_; }
    int gender() { return gender_; }
    int con() { return con_; }
    int men() { return men_ ; }
    int luc() { return luc_; }
    int cha() { return cha_; }
	int lvl() { return lvl_; }
	
	void* target() { return target_; }
    int fd() { return fd_; }
    Cube* cube() { return cube_; }
	Cube* cube_reborn() { return cube_reborn_; }
    
    void username(std::string username) { username_ = username; }
    void password(std::string password) { password_ = password; }
	void status(int status);
    void id(uint_fast32_t id) { id_ = id; }
    void cls(int cls) { cls_ = cls; }
    void gender(int gender) { gender_ = gender; }
    void con(int con) { con_ = con; }
    void men(int men) { men_ = men; }
    void luc(int luc) { luc_ = luc; }
    void cha(int cha) { cha_ = cha;  }
	void lvl(int lvl) { lvl_ = lvl; }

	void target( Creature* mob ) { target_ = mob; }
    void fd(int fd) { fd_ = fd; }
    void cube(Cube* cube) { cube_ = cube; }
	void cube_reborn(Cube* cube) { cube_reborn_ = cube; }
    
    // put implementation in header is to avoid the complexity of
    // namespace plus friend, it could be done in cpp, but I don't see any
    // advantage
    friend std::ostream& operator<< (std::ostream& os, const Player& player)
    {
        os << "username(" << player.username_ << ")";
        os << "password(" << player.password_ << ")";
        os << "id(" << player.id_ << ")";
        os << "cls(" << player.cls_ << ")";
        os << "gender(" << player.gender_ << ")";
        os << "con(" << player.con_ << ")";
        os << "men(" << player.men_ << ")";
        os << "luc(" << player.luc_ << ")";
        os << "cha(" << player.cha_ << ")";
        return os;
    }

    // parameter type
    // 1 - Event::TYPE_JSON_SIMPLE, for login/logout/arrive/leave event usage
    // 2 - Event::TYPE_JSON_DETAIL, for detail event usage
    JsonW* json( uint_fast32_t type ) override;
    
private:
    int fd_ = 0;
    std::string username_;
    std::string password_;
    int status_ = 0;
    int_fast32_t id_ = 0;
    int cls_ = 0;
    int gender_ = 0;
    int con_ = 0, men_ = 0, luc_ = 0, cha_ = 0;
	int lvl_ = 0;

    Creature* target_ = NULL; // a Mob
    
	Cube* cube_ = NULL; // shortcut to the cube_
	Cube* cube_reborn_ = NULL;

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
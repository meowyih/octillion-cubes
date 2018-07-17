#ifndef OCTILLION_PLAYER_HEADER
#define OCTILLION_PLAYER_HEADER

#include <cstdint>
#include <ostream>
#include <map>
#include <string>

#include "world/cube.hpp"
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

class octillion::Player
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
public:
    Player() :
        status_(0), id_(0), 
        gender_(0), cls_(0), 
        con_(0), men_(0), luc_(0), cha_(0) {}

    Player(uint_fast32_t id) :
        status_(0), id_(id), 
        gender_(0), cls_(0), 
        con_(0), men_(0), luc_(0), cha_(0) {}

    Player(uint_fast32_t id, 
        int gender, int cls, 
        int con, int men, int luc, int cha,
        std::string username, std::string password) :
        status_(0), id_(id), 
        gender_(gender), cls_(cls), 
        con_(con), men_(men), luc_(luc), cha_(cha), 
        username_(username), password_(password) {}

    explicit Player(const Player& rhs) :
        status_(rhs.status_), id_(rhs.id_), 
        gender_(rhs.gender_), cls_(rhs.cls_), 
        con_(rhs.con_), men_(rhs.men_), luc_(rhs.luc_), cha_(rhs.cha_), 
        cube_(rhs.cube_),
        username_(rhs.username_), password_(rhs.password_) {}

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
        cube_ = rhs.cube_;

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
    uint_fast32_t id() { return id_; }
    int cls() { return cls_; }
    int gender() { return gender_; }
    int con() { return con_; }
    int men() { return men_ ; }
    int luc() { return luc_; }
    int cha() { return cha_; }
	int lvl() { return lvl_; }
	uint_fast32_t hp() { return hp_; }
	void* target() { return target_; }
    int fd() { return fd_; }
    Cube* cube() { return cube_; }
	Cube* cube_reborn() { return cube_reborn_; }
    
    void username(std::string username) { username_ = username; }
    void password(std::string password) { password_ = password; }
    void status(int status) { status_ = status; }
    void id(uint_fast32_t id) { id_ = id; }
    void cls(int cls) { cls_ = cls; }
    void gender(int gender) { gender_ = gender; }
    void con(int con) { con_ = con; }
    void men(int men) { men_ = men; }
    void luc(int luc) { luc_ = luc; }
    void cha(int cha) { cha_ = cha;  }
	void lvl(int lvl) { lvl_ = lvl; }
	void hp(uint_fast32_t hp) { hp_ = hp; }
	void target( void* mob ) { target_ = mob; }
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
    JsonW* json( int type );
    
private:
    int fd_ = 0;
    std::string username_;
    std::string password_;
    int status_;
    uint_fast32_t id_;
    int cls_;
    int gender_;
    int con_, men_, luc_, cha_;
	int lvl_;

	uint_fast32_t hp_ = 10;
    void* target_; // a Mob
    
	Cube* cube_; // shortcut to the cube_
	Cube* cube_reborn_;

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
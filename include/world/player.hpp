#ifndef OCTILLION_PLAYER_HEADER
#define OCTILLION_PLAYER_HEADER

#include <cstdint>
#include <ostream>
#include <map>
#include <string>

#include "world/cube.hpp"

// classes - skiller, believer
// gender - male, female, neutral
// attribute - Constitution, Mental, Luck, Charming

namespace octillion
{
    class Player;
}

class octillion::Player
{
public:
    const static uint32_t STATUS_ABANDON = 0;
    const static uint32_t STATUS_CREATING = 1;
    const static uint32_t STATUS_LOGIN_CONNECT = 2;
    const static uint32_t STATUS_LOGIN_DISCONNECT = 3;
    const static uint32_t STATUS_LOGOUT = 4;
  
    const static uint32_t GENDER_NEUTRAL = 0;
    const static uint32_t GENDER_MALE = 1;
    const static uint32_t GENDER_FEMALE = 2;

    const static uint32_t CLS_SKILLER = 1;
    const static uint32_t CLS_BELIEVER = 2;
    
public:
    Player() :
        status_(0), id_(0), gender_(0), cls_(0), con_(0), men_(0), luc_(0), cha_(0) {}
    Player(uint32_t id) :
        status_(0), id_(id), gender_(0), cls_(0), con_(0), men_(0), luc_(0), cha_(0) {}
    Player(uint32_t id, uint32_t gender, uint32_t cls, uint32_t con, uint32_t men, uint32_t luc, uint32_t cha) :
        status_(0), id_(id), gender_(gender), cls_(cls), con_(con), men_(men), luc_(luc), cha_(cha) {}

public:
    // help static function to roll a player attribute based on gender and cls
    static void roll(uint32_t gender, uint32_t cls, std::map<std::string, uint32_t>& attribute);

public:
    uint32_t status() { return status_; }
    uint32_t id() { return id_; }
    uint32_t cls() { return cls_; }
    uint32_t gender() { return gender_; }
    uint32_t con() { return con_; }
    uint32_t men() { return men_ ; }
    uint32_t luc() { return luc_; }
    uint32_t cha() { return cha_; }

    void status(uint32_t status) { status_ = status; }
    void id(uint32_t id) { id_ = id; }
    void cls(uint32_t cls) { cls_ = cls; }
    void gender(uint32_t gender) { gender_ = gender; }
    void con(uint32_t con) { con_ = con; }
    void men(uint32_t men) { men_ = men; }
    void luc(uint32_t luc) { luc_ = luc; }
    void cha(uint32_t cha) { cha_ = cha;  }

    // put implementation in header is to avoid the complexity of
    // namespace plus friend, it could be done in cpp, but I don't see any
    // advantage
    friend std::ostream& operator<< (std::ostream& os, const Player& player)
    {
        os << "id(" << player.id_ << ")";
        os << "cls(" << player.cls_ << ")";
        os << "gender(" << player.gender_ << ")";
        os << "con(" << player.con_ << ")";
        os << "men(" << player.men_ << ")";
        os << "luc(" << player.luc_ << ")";
        os << "cha(" << player.cha_ << ")";
        return os;
    }

    void move(CubePosition loc);
    CubePosition position() { return loc_; }

private:
    uint32_t status_;
    uint32_t id_;
    uint32_t cls_;
    uint32_t gender_;
    uint32_t con_, men_, luc_, cha_;

    CubePosition loc_;
};

#endif
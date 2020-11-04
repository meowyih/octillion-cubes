#ifndef OCTILLION_PLAYER_HEADER
#define OCTILLION_PLAYER_HEADER

#include <string>
#include <memory>

#include "world/cube.hpp"
#include "world/creature.hpp"


namespace octillion
{
    class Player;
}

class octillion::Player : public octillion::Creature
{
private:
	const std::string tag_ = "Player";
    
public:
    int_fast32_t id_;
    int          fd_;
};

#endif
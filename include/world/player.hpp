#ifndef OCTILLION_PLAYER_HEADER
#define OCTILLION_PLAYER_HEADER

#include <cstdint>

#include "world/cube.hpp"

namespace octillion
{
    class Player;
}

class octillion::Player
{
public:
    Player(uint32_t id) { id_ = id; }

public:
    uint32_t id() { return id_; }
    void move(CubePosition loc);
    CubePosition position() { return loc_; }

private:
    uint32_t id_;
    CubePosition loc_;
};

#endif
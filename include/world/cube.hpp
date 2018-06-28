#ifndef OCTILLION_CUBE_HEADER
#define OCTILLION_CUBE_HEADER

#include <cstdint>
#include <string>
#include <system_error>

#include "world/tick.hpp"

namespace octillion
{
    class CubePosition;
    class Cube;
}

class octillion::CubePosition
{
public:
    enum Direction { NORTH , EAST, WEST, SOUTH, UP, DOWN };

public:
    CubePosition();
    CubePosition(const CubePosition& rhs);
    CubePosition(const CubePosition& rhs, Direction dir);
    CubePosition(uint32_t x, uint32_t y, uint32_t z);

    std::string str();

public:
    uint32_t x_axis_, y_axis_, z_axis_;

    bool operator < (const CubePosition& rhs) const
    {
        if (x_axis_ < rhs.x_axis_)
        {
            return true;
        }
        else if (x_axis_ == rhs.x_axis_ && y_axis_ < rhs.y_axis_)
        {
            return true;
        }
        else if (x_axis_ == rhs.x_axis_ && y_axis_ == rhs.y_axis_ && z_axis_ < rhs.z_axis_)
        {
            return true;
        }

        return false;
    }
};

class octillion::Cube : octillion::Tick
{
public:
    Cube(CubePosition loc, TickCallback* cb );
    ~Cube();

public:
    CubePosition location() { return loc_; }

public:
    
    // enum ExitType { NORMAL, HIDDEN, FLY, SWIM };

    uint8_t getexit(CubePosition::Direction direction);
    bool setexit(const Cube& cube, uint8_t exit);
    void setexit(CubePosition::Direction direction, uint8_t exit);

    static uint8_t exitval(
        bool normal = true,
        bool hidden = false,
        bool fly = false,
        bool swim = false,
        bool rev1 = false,
        bool rev2 = false,
        bool rev3 = false,
        bool rev4 = false);

public:
    virtual std::error_code tick() override;

private:
    CubePosition loc_;
    TickCallback* cb_;

private:
    uint8_t exits_[6];
};

#endif

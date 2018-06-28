
#include <sstream>
#include <cstring> // memset
#include <system_error>

#include "world/cube.hpp"
#include "error/ocerror.hpp"

octillion::CubePosition::CubePosition()
{
    x_axis_ = 0;
    y_axis_ = 0;
    z_axis_ = 0;
}

octillion::CubePosition::CubePosition(const CubePosition& rhs)
{
    x_axis_ = rhs.x_axis_;
    y_axis_ = rhs.y_axis_;
    z_axis_ = rhs.z_axis_;
}

octillion::CubePosition::CubePosition(uint32_t x, uint32_t y, uint32_t z)
{
    x_axis_ = x;
    y_axis_ = y;
    z_axis_ = z;
}

octillion::CubePosition::CubePosition(const CubePosition& rhs, Direction dir)
{
    x_axis_ = rhs.x_axis_;
    y_axis_ = rhs.y_axis_;
    z_axis_ = rhs.z_axis_;
    
    switch ( dir )
    {
    case CubePosition::NORTH: y_axis_++; break;
    case CubePosition::EAST:  x_axis_++; break;
    case CubePosition::SOUTH: y_axis_--; break;
    case CubePosition::WEST:  x_axis_--; break;
    case CubePosition::UP:    z_axis_++; break;
    case CubePosition::DOWN:  z_axis_--; break;
    }
}

std::string octillion::CubePosition::str()
{
    std::ostringstream oss;
    oss << "(" << x_axis_ << "," << y_axis_ << "," << z_axis_ << ")";
    return oss.str();
}

octillion::Cube::Cube(CubePosition loc, TickCallback* cb)
{
    loc_ = loc;
    cb_ = cb;
    
    std::memset( exits_, 0, sizeof exits_);
}

octillion::Cube::~Cube()
{
}

std::error_code octillion::Cube::tick()
{
    return OcError::E_SUCCESS;
}

bool octillion::Cube::setexit(const Cube& cube, uint8_t type)
{
    if (cube.loc_.y_axis_ == loc_.y_axis_ && cube.loc_.z_axis_ == loc_.z_axis_)
    {
        if (cube.loc_.x_axis_ == loc_.x_axis_ + 1)
        {
            setexit(CubePosition::EAST, type);
        }
        else if (cube.loc_.x_axis_ == loc_.x_axis_ - 1)
        {
            setexit(CubePosition::WEST, type);
        }

        return true;
    }

    if (cube.loc_.x_axis_ == loc_.x_axis_ && cube.loc_.z_axis_ == loc_.z_axis_)
    {
        if (cube.loc_.y_axis_ == loc_.y_axis_ + 1)
        {
            setexit(CubePosition::NORTH, type);
        }
        else if (cube.loc_.y_axis_ == loc_.y_axis_ - 1)
        {
            setexit(CubePosition::SOUTH, type);
        }

        return true;
    }

    if (cube.loc_.x_axis_ == loc_.x_axis_ && cube.loc_.y_axis_ == loc_.y_axis_)
    {
        if (cube.loc_.z_axis_ == loc_.z_axis_ + 1)
        {
            setexit(CubePosition::UP, type);
        }
        else if (cube.loc_.z_axis_ == loc_.z_axis_ - 1)
        {
            setexit(CubePosition::DOWN, type);
        }

        return true;
    }

    return false;
}

uint8_t octillion::Cube::getexit(CubePosition::Direction direction)
{
    switch (direction)
    {
    case CubePosition::NORTH: return exits_[0];
    case CubePosition::EAST: return exits_[1];
    case CubePosition::SOUTH: return exits_[2];
    case CubePosition::WEST: return exits_[3];
    case CubePosition::UP: return exits_[4];
    case CubePosition::DOWN: return exits_[5];
    }

    return 0;
}

void octillion::Cube::setexit(CubePosition::Direction direction, uint8_t exit)
{
    switch (direction)
    {
    case CubePosition::NORTH: exits_[0] = exit; return;
    case CubePosition::EAST: exits_[1] = exit; return;
    case CubePosition::SOUTH: exits_[2] = exit; return;
    case CubePosition::WEST: exits_[3] = exit; return;
    case CubePosition::UP: exits_[4] = exit; return;
    case CubePosition:: DOWN: exits_[5] = exit; return;
    }
    
    return;
}

uint8_t octillion::Cube::exitval(bool normal, bool hidden, bool fly, bool swim, bool rev1, bool rev2, bool rev3, bool rev4)
{
    uint8_t exit = 0;
    if (normal)
    {
        exit = exit | 1;
    }

    if (hidden)
    {
        exit = exit | 2;
    }

    if (fly)
    {
        exit = exit | 4;
    }

    if (swim)
    {
        exit = exit | 8;
    }

    if (rev1)
    {
        exit = exit | 16;
    }

    if (rev2)
    {
        exit = exit | 32;
    }

    if (rev3)
    {
        exit = exit | 64;
    }

    if (rev4)
    {
        exit = exit | 128;
    }

    return exit;
}



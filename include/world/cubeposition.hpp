#ifndef OCTILLION_CUBEPOSITION_HEADER
#define OCTILLION_CUBEPOSITION_HEADER

#include <string>
#include <memory>

#include "jsonw/jsonw.hpp"

namespace octillion
{
    class CubePosition;
}

class octillion::CubePosition
{
private:
    const std::string tag_ = "CubePosition";

public:
    const static int X_INC = 1;
    const static int Y_INC = 2;
    const static int Z_INC = 4;
    const static int X_DEC = 3;
    const static int Y_DEC = 0;
    const static int Z_DEC = 5;

    const static int X_INC_Y_INC = 6;
    const static int X_INC_Y_DEC = 7;
    const static int X_DEC_Y_INC = 8;
    const static int X_DEC_Y_DEC = 9;
    const static int TOTAL_EXIT = 10;

public:
    CubePosition();
    CubePosition(const CubePosition& rhs);
    CubePosition(const CubePosition& rhs, int direction);
    CubePosition(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);

    void set(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);

    std::string str();
public:
    uint_fast32_t x() const { return x_axis_; }
    uint_fast32_t y() const { return y_axis_; }
    uint_fast32_t z() const { return z_axis_; }

    // convert cube position into json 
    std::shared_ptr<JsonW> json();

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

    bool operator == (const CubePosition& rhs) const
    {
        return x_axis_ == rhs.x_axis_ && y_axis_ == rhs.y_axis_ && z_axis_ == rhs.z_axis_;
    }

    CubePosition& operator = (const CubePosition& rhs)
    {
        x_axis_ = rhs.x_axis_;
        y_axis_ = rhs.y_axis_;
        z_axis_ = rhs.z_axis_;

        return *this;
    }

private:
    uint_fast32_t x_axis_;
    uint_fast32_t y_axis_;
    uint_fast32_t z_axis_;
};

// add a hash function for CubePosition for unordered_map usage
namespace std {

    template <>
    struct hash<octillion::CubePosition>
    {
        std::size_t operator()(const octillion::CubePosition& key) const
        {
            // we don't care about the overflow
            std::size_t a = (std::size_t)key.x();
            std::size_t b = (std::size_t)key.y();
            std::size_t c = (std::size_t)key.z();

            // cantor pairing function x 2
            std::size_t d = 5 * (a + b) * (a + b + 1) + b;
            return 5 * (c + d) * (c + d + 1) + d;
        }
    };
}

#endif
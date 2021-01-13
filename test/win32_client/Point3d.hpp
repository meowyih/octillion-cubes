#ifndef POINT_3D_HEADER
#define POINT_3D_HEADER

#include <cstdint>

class Point3d
{
public:
    Point3d() : x_(0), y_(0), z_(0) {}
    Point3d(int_fast32_t x, int_fast32_t y, int_fast32_t z) : x_(x), y_(y), z_(z) {}
    void set(int_fast32_t x, int_fast32_t y, int_fast32_t z)
    {
        x_ = x;
        y_ = y;
        z_ = z;
    }
    int_fast32_t x_, y_, z_;

    bool equal(const Point3d& rhs)
    {
        return (x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_);
    }
};
#endif
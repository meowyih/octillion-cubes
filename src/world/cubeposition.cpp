#include "world/cubeposition.hpp"

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

octillion::CubePosition::CubePosition(const CubePosition& rhs, int direction)
{
    x_axis_ = rhs.x_axis_;
    y_axis_ = rhs.y_axis_;
    z_axis_ = rhs.z_axis_;

    switch (direction)
    {
    case X_INC:
        x_axis_++;
        break;
    case Y_INC:
        y_axis_++;
        break;
    case Z_INC:
        z_axis_++;
        break;
    case X_DEC:
        x_axis_--;
        break;
    case Y_DEC:
        y_axis_--;
        break;
    case Z_DEC:
        z_axis_--;
        break;
    }
}

octillion::CubePosition::CubePosition(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
{
    x_axis_ = x;
    y_axis_ = y;
    z_axis_ = z;
}

void octillion::CubePosition::set(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
{
    x_axis_ = x;
    y_axis_ = y;
    z_axis_ = z;
}

std::string octillion::CubePosition::str()
{
    std::ostringstream oss;
    oss << "(" << x_axis_ << "," << y_axis_ << "," << z_axis_ << ")";
    return oss.str();
}

std::shared_ptr<JsonW> octillion::CubePosition::json()
{
    std::shared_ptr<JsonW> jobject = std::make_shared<JsonW>();
    jobject->add("x", (int)x_axis_);
    jobject->add("y", (int)y_axis_);
    jobject->add("z", (int)z_axis_);
    return jobject;
}

#include <chrono>

#include "D2BrushCreator.hpp"
#include "MapFrameAnimation.hpp"

MapFrameAnimation::MapFrameAnimation()
{
    animation_length_ms_ = 0;
    shift1_ = D2D1::Point2F(0.f, 0.f);
    side1_ = 0.f;
    shift2_ = D2D1::Point2F(0.f, 0.f);
    side2_ = 0.f;
}

void MapFrameAnimation::start(
    long length_ms,
    std::shared_ptr<octillion::Cube> from, D2D_POINT_2F shift1, FLOAT side1, 
    std::shared_ptr<octillion::Cube> to, D2D_POINT_2F shift2, FLOAT side2)
{
    animation_length_ms_ = length_ms;
    shift_.x = (from->loc().x() - to->loc().x()) * side2 + shift2.x - shift1.x;
    shift_.y = (from->loc().y() - to->loc().y()) * side2 + shift2.y - shift1.y;
    start_time_ = std::chrono::system_clock::now().time_since_epoch();

    cube_start_ = from;
    side1_ = side1;
    shift1_ = shift1;
    cube_end_ = to;
    shift2_ = shift2;
    side2_ = side2;
}

bool MapFrameAnimation::draw(ID2D1HwndRenderTarget* target, D2BrushCreator& brush, RenderMapFrame& frame)
{
    std::chrono::system_clock::duration duration =
        std::chrono::system_clock::now().time_since_epoch() - start_time_;

    long duration_ms = (long)(std::chrono::duration_cast<milliseconds>(duration).count());

    FLOAT ratio;
    
    if (duration_ms >= animation_length_ms_)
    {
        frame.draw(target, brush, cube_end_, shift2_, side2_);
        return false;
    }

    ratio = duration_ms * 1.f / animation_length_ms_ * 1.f;

    frame.draw(target, brush, cube_start_, 
        D2D1::Point2F( shift_.x * ratio, shift_.y * ratio), 
        side1_ + (side2_ - side1_) * ratio);

    return true;
}

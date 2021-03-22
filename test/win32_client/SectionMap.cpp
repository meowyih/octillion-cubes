#include <unordered_set>
#include <list>

#include "error/macrolog.hpp"
#include "SectionMap.hpp"

#include "D2BrushCreator.hpp"

SectionMap::SectionMap()
{
}

SectionMap::~SectionMap()
{
    fini();
}

bool SectionMap::init(ID2D1Factory* factory, D2D1_RECT_F clip, std::shared_ptr<octillion::Cube> cube)
{
    fini();
    return base_frame_.init(factory, clip, cube);
}

void SectionMap::fini()
{
    base_frame_.fini();
}

void SectionMap::start_animation(
    long duration_ms, 
    std::shared_ptr<octillion::Cube> cube_from, 
    D2D_POINT_2F shift1, FLOAT side_length1, 
    std::shared_ptr<octillion::Cube> cube_to, 
    D2D_POINT_2F shift2, FLOAT side_length2)
{
    if (is_animation_running())
    {
        stop_animation();
    }

    base_frame_animation_.start(duration_ms, cube_from, shift1, side_length1,
        cube_to, shift2, side_length2);

    is_in_animation_ = true;
}

void SectionMap::stop_animation()
{
    is_in_animation_ = false;
}

bool SectionMap::is_animation_running()
{
    return is_in_animation_;
}

void SectionMap::update(
    ID2D1HwndRenderTarget* target, 
    D2BrushCreator& brush, 
    std::shared_ptr<octillion::Cube> cube,
    D2D_POINT_2F shift,
    FLOAT side_length)
{
    // base_frame_.draw(target, brush, cube_zero, D2D1::Point2F(0.f, 0.f), side_length_);
 
    // test animation
    if (is_in_animation_)
    {

        is_in_animation_ = base_frame_animation_.draw(target, brush, base_frame_);
    }
    else
    {
        base_frame_.draw(target, brush, cube, shift, side_length_);
    }
}

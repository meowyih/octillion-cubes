#pragma once

#include <chrono>

#include "D2BrushCreator.hpp"
#include "RenderMapFrame.hpp"

using std::chrono::milliseconds;

class MapFrameAnimation
{
public:
    MapFrameAnimation();

    void start( long length_ms,
        std::shared_ptr<octillion::Cube> from,
        D2D_POINT_2F shift1,
        FLOAT side1,
        std::shared_ptr<octillion::Cube> to,
        D2D_POINT_2F shift2,
        FLOAT side2
        );

    bool draw(
        ID2D1HwndRenderTarget* target,
        D2BrushCreator& brush,
        RenderMapFrame& frame);

private:
    std::chrono::system_clock::duration start_time_;
    long animation_length_ms_;
    D2D_POINT_2F shift_;

    std::shared_ptr<octillion::Cube> cube_start_;
    D2D_POINT_2F shift1_;
    FLOAT side1_;

    std::shared_ptr<octillion::Cube> cube_end_;
    D2D_POINT_2F shift2_;
    FLOAT side2_;
};


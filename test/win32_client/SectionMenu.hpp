#pragma once

#include <d2d1.h>

#include "D2BrushCreator.hpp"

class SectionMenu
{
public:
    SectionMenu();
    ~SectionMenu();

    void init(FLOAT w, FLOAT h, FLOAT offset_x, FLOAT offset_y );
    void update(ID2D1HwndRenderTarget* target, D2BrushCreator& brush );

private:
    FLOAT w_;
    FLOAT h_;
    
    FLOAT line_x_;
    FLOAT line_y_;
    FLOAT line_length_;
    FLOAT line_stroke_width_;

    FLOAT circle_center_x_;
    FLOAT circle_center_y_;
    FLOAT circle_radius_;
    FLOAT circle_stroke_width_;
};


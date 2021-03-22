#include "SectionMenu.hpp"

SectionMenu::SectionMenu()
{
}

SectionMenu::~SectionMenu()
{
}

void SectionMenu::init(FLOAT w, FLOAT h, FLOAT offset_x, FLOAT offset_y)
{
    w_ = w;
    h_ = h;
    line_x_ = offset_x + w_ * 1.618f / 2.618f;
    line_y_ = offset_y;
    line_length_ = h_ - offset_y;
    line_stroke_width_ = 1.6f;
    circle_center_x_ = offset_x + w_ / 2.0f;
    circle_center_y_ = offset_y + h_ / 2.0f;
    circle_radius_ = w_ / 2;
    circle_stroke_width_ = 1.0f;
}

void SectionMenu::update(ID2D1HwndRenderTarget* target, D2BrushCreator& brush)
{
    target->DrawLine(
        D2D1::Point2F( line_x_, line_y_),
        D2D1::Point2F( line_x_, line_y_ + line_length_),
        brush.get(target, D2BrushCreator::SOLID_MIDDLE_SEPERATOR),
        line_stroke_width_ );

    target->FillEllipse(D2D1::Ellipse(
        D2D1::Point2F( circle_center_x_, circle_center_y_ ),
        circle_radius_,
        circle_radius_), brush.get(target, D2BrushCreator::SOLID_BACKGROUND));

    target->DrawEllipse(D2D1::Ellipse(
        D2D1::Point2F( circle_center_x_, circle_center_y_ ),
        circle_radius_,
        circle_radius_), brush.get(target, D2BrushCreator::SOLID_MIDDLE_SEPERATOR),
        circle_stroke_width_ );
}

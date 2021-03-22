#include "error/macrolog.hpp"

#include "SectionText.hpp"

SectionText::SectionText()
{
    init(D2D1::Rect(0.f, 0.f, 0.f, 0.f));
}

SectionText::~SectionText()
{
}

void SectionText::init(D2D1_RECT_F canvas)
{
    canvas_ = canvas;
    pos_backward_ = 0.f;
    line_gap_ = 10.f;
}

void SectionText::add(std::wstring str, IDWriteFactory* factory, D2TextFormatCreator& format)
{
    FLOAT canvas_width = canvas_.right - canvas_.left;
    FLOAT canvas_height = canvas_.bottom - canvas_.top;

    // create section sub text
    std::shared_ptr<SectionSubText> subtext = std::make_shared<SectionSubText>();
    if (subtext->init(
        factory, 
        format.get(factory, D2TextFormatCreator::DEFAULT), 
        canvas_width, canvas_height, str) == false)
    {
        LOG_E(tag_) << "add failed, cannot init subtext";
        subtext->fini();
        return;
    }

    items_.push_front(subtext);

    return;
}

void SectionText::add(std::wstring lead, std::wstring str, IDWriteFactory* factory, D2TextFormatCreator& format)
{
    FLOAT canvas_width = canvas_.right - canvas_.left;
    FLOAT canvas_height = canvas_.bottom - canvas_.top;

    // create section sub text
    std::shared_ptr<SectionSubText> subtext = std::make_shared<SectionSubText>();

    if (subtext->init(
        factory,
        format.get(factory, D2TextFormatCreator::SECTION_TEXT_LEADING),
        format.get(factory, D2TextFormatCreator::DEFAULT),
        canvas_width, canvas_height, lead, str) == false)
    {
        LOG_E(tag_) << "add failed, cannot init subtext";
        subtext->fini();
        return;
    }

    items_.push_front(subtext);

    return;
}

void SectionText::update(ID2D1Factory* factory, ID2D1HwndRenderTarget* target, D2BrushCreator& brush)
{
    FLOAT low, high;
    FLOAT canvas_width = canvas_.right - canvas_.left;
    FLOAT canvas_height = canvas_.bottom - canvas_.top;

    high = canvas_height + pos_backward_;

    // draw background
    target->FillRectangle(canvas_, brush.get(target, D2BrushCreator::SOLID_BACKGROUND));

    // draw each text section
    for (auto it = items_.begin(); it != items_.end(); it++)
    {
        D2D_RECT_F rect;
        if ((*it)->get_rect(rect) == false)
        {
            LOG_E(tag_) << "update(), failed to get subtext's rect";
            continue;
        }

        low = high - (rect.bottom - rect.top);

        if (high < 0)
        {
            break; // all the remaining text are outside the drawing area
        }
        else if ( low > canvas_height)
        {
            // not in the drawable area (below the bottom line), ignore it
        }
        else if (high <= canvas_height && low >= 0)
        {
            // whole area is inside the drawable area
            (*it)->draw(factory, target, 
                brush,
                canvas_.left, canvas_.top + low, canvas_);
        }
        else
        {
            (*it)->draw(factory, target, 
                brush,
                canvas_.left, canvas_.top + low,
                canvas_,
                SectionSubText::DRAW_VERTICAL_PARTIAL                
            );
        }
        
        high -= (rect.bottom - rect.top);
        high -= line_gap_; // add line gap
    }
}

void SectionText::pos_inc(FLOAT size)
{
    pos_backward_ += size;
}

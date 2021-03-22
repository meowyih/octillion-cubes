#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <list>

#include "D2TextFormatCreator.hpp"
#include "D2BrushCreator.hpp"

#include "SectionSubText.hpp"

//             w
//   |(x,y) +------+                (0,0) ----
//   |      |item 1|                      line gap
//   |      +------+                 low  ----
//   h      |item 2|                      item N
//   |      +------+                 high ----
//   +  --  |item 3| (x+w, y+h)           line gap
// pos_back +------+
//   |      |item 4|
//   +  --  +------+ 
class SectionText
{
private:
    const std::string tag_ = "SectionText";
public:
    SectionText();
    ~SectionText();

    void init(D2D1_RECT_F canvas);
    void add(std::wstring str, IDWriteFactory* factory, D2TextFormatCreator& format);
    void add(std::wstring lead, std::wstring str, IDWriteFactory* factory, D2TextFormatCreator& format);
    void update(ID2D1Factory* factory, ID2D1HwndRenderTarget* target, D2BrushCreator& brush);
    void pos_inc(FLOAT size);

private:
    D2D1_RECT_F canvas_;
    FLOAT line_gap_;

private:
    std::list<std::shared_ptr<SectionSubText>> items_;
    FLOAT pos_backward_; // anchor's distance from bottom
};


#pragma once

#include <string>
#include <map>
#include <memory>

#include <d2d1.h>

class D2BrushCreator
{
public:
    const static int SOLID_DEFAULT = 1;
    const static int SOLID_BACKGROUND = 2;
    const static int SOLID_MIDDLE_SEPERATOR = 3;
    const static int SOLID_BLACK = 4;
    const static int SOLID_GENERAL_TEXT = 5;
    const static int SOLID_WHITE = 6;
    const static int SOLID_RED = 7;

private:
    const std::string tag_ = "D2BrushCreator";

public:
    D2BrushCreator();
    ~D2BrushCreator();

public:
    // avoid accidentally copy
    D2BrushCreator(D2BrushCreator const&) = delete;
    void operator = (D2BrushCreator const&) = delete;

public:
    ID2D1Brush* get(ID2D1HwndRenderTarget* target, int id); 
    void fini();

private:
    std::map<int,ID2D1Brush*> brushes_;
};

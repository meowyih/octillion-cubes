#pragma once

#include <d2d1.h>
#include <unordered_set>
#include <vector>
#include <list>
#include <string>

#include "world/cubeposition.hpp"
#include "world/worldmap.hpp"

#include "D2BrushCreator.hpp"
#include "RenderMapFrame.hpp"
#include "MapFrameAnimation.hpp"

class SectionMap
{
private:
    const std::string tag_ = "SectionMap";

public:
    SectionMap();
    ~SectionMap();

    bool init(ID2D1Factory* factory, D2D1_RECT_F clip, std::shared_ptr<octillion::Cube> cube);
    void fini();

    void start_animation(
        long duration_ms,
        std::shared_ptr<octillion::Cube> cube_from,
        D2D_POINT_2F shift1,
        FLOAT side_length1,
        std::shared_ptr<octillion::Cube> cube_to,
        D2D_POINT_2F shift2,
        FLOAT side_length2
    );

    void stop_animation();

    bool is_animation_running();

    // 'cube' and 'side_length' won't work if during animation
    void update(
        ID2D1HwndRenderTarget* target,
        D2BrushCreator& brush,
        std::shared_ptr<octillion::Cube> cube,
        D2D_POINT_2F shift,
        FLOAT side_length);

    FLOAT side_length_ = 30.f;

private:
    RenderMapFrame base_frame_;
    MapFrameAnimation base_frame_animation_;

    bool is_in_animation_ = false;
};


#pragma once

#include <memory>
#include <string>
#include <list>

#include <d2d1.h>

#include "world/cube.hpp"

#include "D2BrushCreator.hpp"

class RenderMapFrame
{
private:
    const std::string tag_ = "RenderMapFrame";

private:
    const FLOAT side_length_ = 30.f;
    const FLOAT wall_length_ = 25.f;

public:
    class CubeProperty
    {
    public:
        bool n_wall = false;
        bool e_wall = false;
        bool s_wall = false;
        bool w_wall = false;
        bool nw_pillar = true;
        bool sw_pillar = true;
        bool ne_pillar = true;
        bool se_pillar = true;
    };

    class Line
    {
    public:
        D2D1_POINT_2F start;
        D2D1_POINT_2F end;
    public:
        Line();
        Line(D2D1_POINT_2F from, D2D1_POINT_2F to);
    };

public:
    RenderMapFrame();
    ~RenderMapFrame();

    bool init(ID2D1Factory* factory, D2D1_RECT_F clip, std::shared_ptr<octillion::Cube> cube);
    void fini();
    void draw(
        ID2D1HwndRenderTarget* target,
        D2BrushCreator& brush,
        std::shared_ptr<octillion::Cube> center_cube,
        D2D_POINT_2F shift,
        FLOAT side_length);

    // get origin point information
    std::shared_ptr<octillion::Cube> origin();

private:
    // call need to create geometry
    bool enclosed_lines_to_geometry(ID2D1Factory* factory, std::list<RenderMapFrame::Line>& lines, ID2D1PathGeometry* geometry);

private:
    static void render_cube_frame(
        bool render_in_canvas,
        D2D_RECT_F canvas,
        std::shared_ptr<octillion::Cube> cube,
        D2D1_POINT_2F cube_pos, // cube position on canvas coordination
        FLOAT side_length,
        FLOAT wall_length,
        std::unordered_set<octillion::CubePosition>& record,
        std::list<RenderMapFrame::Line>& cache);

    static inline void render_cube_frame_west(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        std::list<RenderMapFrame::Line>& cache
    );

    static inline void render_cube_frame_north(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        std::list<RenderMapFrame::Line>& cache
    );

    static inline void render_cube_frame_east(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        std::list<RenderMapFrame::Line>& cache
    );

    static inline void render_cube_frame_south(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        std::list<RenderMapFrame::Line>& cache
    );

    static inline bool cut_line(
        const D2D_RECT_F& canvas,
        const RenderMapFrame::Line& line,
        RenderMapFrame::Line& cut
    );

    static inline bool is_in_rect(
        const D2D_RECT_F& canvas,
        const D2D1_POINT_2F& pt
    );

    static inline void lines_to_enclosed_lines(std::list<RenderMapFrame::Line>& lines, std::list<RenderMapFrame::Line>& out);
    static inline void simplify_lines(std::list<RenderMapFrame::Line>& lines);

private:
    std::shared_ptr<octillion::Cube> cached_cube_;
    ID2D1GeometryGroup* geometry_group_;
    D2D1_RECT_F rect_mask_;
    ID2D1RectangleGeometry* geomatry_rect_mask_;
};

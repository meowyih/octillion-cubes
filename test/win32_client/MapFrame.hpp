#pragma once

#include <memory>
#include <string>
#include <list>
#include <unordered_set>

#include <d2d1.h>

#include "world/cube.hpp"

using std::shared_ptr;
using std::list;
using std::unordered_set;

using octillion::Cube;
using octillion::CubePosition;

class MapFrame
{
public:
    MapFrame();
    ~MapFrame();

public:
    // render_map() create group geometry 'geogroup' without release it, 
    // caller should do that by themselves.
    static bool render_map(
        ID2D1Factory* factory, 
        ID2D1GeometryGroup** geogroup, 
        shared_ptr<Cube> cube);

private:
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

private:
    // call need to create geometry
    static bool enclosed_lines_to_geometry(ID2D1Factory* factory, list<MapFrame::Line>& lines, ID2D1PathGeometry* geometry);

    static void render_cube_frame(
        bool render_in_canvas,
        D2D_RECT_F canvas,
        shared_ptr<Cube> cube,
        D2D1_POINT_2F cube_pos, // cube position on canvas coordination
        FLOAT side_length,
        FLOAT wall_length,
        unordered_set<CubePosition>& record,
        list<MapFrame::Line>& cache);

    static inline void render_cube_frame_west(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        list<MapFrame::Line>& cache
    );

    static inline void render_cube_frame_north(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        list<MapFrame::Line>& cache
    );

    static inline void render_cube_frame_east(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        list<MapFrame::Line>& cache
    );

    static inline void render_cube_frame_south(
        const CubeProperty& property,
        D2D1_POINT_2F cube_pos,
        FLOAT side_length,
        FLOAT wall_length,
        FLOAT pillar_length,
        list<MapFrame::Line>& cache
    );

    static inline bool cut_line(
        const D2D_RECT_F& canvas,
        const MapFrame::Line& line,
        MapFrame::Line& cut
    );

    static inline bool is_in_rect(
        const D2D_RECT_F& canvas,
        const D2D1_POINT_2F& pt
    );

    static inline void lines_to_enclosed_lines(list<MapFrame::Line>& lines, list<MapFrame::Line>& out);
    static inline void simplify_lines(list<MapFrame::Line>& lines);
};


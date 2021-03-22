#include "MapFrame.hpp"

using std::vector;

MapFrame::MapFrame()
{
}

MapFrame::~MapFrame()
{
}

bool MapFrame::render_map(ID2D1Factory* factory, ID2D1GeometryGroup** geogroup, shared_ptr<Cube> cube)
{
    HRESULT hr = E_FAIL;
    unordered_set<CubePosition> record;
    list<MapFrame::Line> raw_lines;
    vector<ID2D1Geometry*> geometries;

    FLOAT side_length = 30.f;
    FLOAT wall_length = 25.f;

    // convert map into lines
    render_cube_frame(false, D2D1::RectF(0.f, 0.f), cube, D2D1::Point2F(0.f, 0.f), side_length, wall_length, record, raw_lines);
    record.clear(); // release unused memory

    // convert lines to several geometries
    while (raw_lines.size() > 0)
    {
        ID2D1PathGeometry* geometry = NULL;

        hr = factory->CreatePathGeometry(&geometry);

        if (!SUCCEEDED(hr))
        {
            break;
        }

        if (enclosed_lines_to_geometry(factory, raw_lines, geometry))
        {
            geometries.push_back(geometry);
        }
        else
        {
            hr = E_FAIL;
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = factory->CreateGeometryGroup(
            D2D1_FILL_MODE_ALTERNATE,
            geometries.data(),
            geometries.size(),
            geogroup
        );
    }

    for (auto it = geometries.begin(); it != geometries.end(); it++)
    {
        (*it)->Release();
    }

    if (SUCCEEDED(hr))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool MapFrame::enclosed_lines_to_geometry(ID2D1Factory* factory, list<MapFrame::Line>& lines, ID2D1PathGeometry* geometry)
{
    ID2D1GeometrySink* sink = NULL;
    HRESULT hr;
    list<MapFrame::Line> enclosed_lines;
    vector<D2D1_POINT_2F> path;

    if (lines.size() <= 1)
    {
        return false;
    }

    lines_to_enclosed_lines(lines, enclosed_lines);

    if (enclosed_lines.size() <= 1)
    {
        return false; // we need at least two lines to form a geometry
    }

    // convert lines to direct2d path geometry
    path.reserve(enclosed_lines.size() * 2);

    for (auto it = enclosed_lines.begin(); it != enclosed_lines.end(); it++)
    {
        path.push_back(it->start);
        path.push_back(it->end);
    }

    hr = geometry->Open(&sink);

    if (SUCCEEDED(hr))
    {
        sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        sink->BeginFigure(
            path.at(0),
            D2D1_FIGURE_BEGIN_FILLED
        );
        sink->AddLines(path.data() + 1, path.size() - 1);
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        hr = sink->Close();
    }

    if (sink != NULL)
        sink->Release();

    if (SUCCEEDED(hr))
    {
        return true;
    }

    return false;
}


void MapFrame::render_cube_frame(
    bool render_in_canvas,
    D2D_RECT_F canvas,
    shared_ptr<Cube> cube,
    D2D1_POINT_2F cube_pos,
    FLOAT side_length,
    FLOAT wall_length,
    unordered_set<CubePosition>& record,
    list<MapFrame::Line>& cache)
{
    bool partial = false;
    CubeProperty property;
    FLOAT pillar_length = (side_length - wall_length) / 2.f;
    list<MapFrame::Line> lines;

    // when cube does not exist, stop recurrsively loop
    if (cube == nullptr)
        return;

    // when cube already visited, stop recurrsively loop
    if (record.find(cube->loc()) != record.end())
        return;
    else
        record.insert(cube->loc());

    if (render_in_canvas)
    {
        // when cube is outside the canvas
        if (cube_pos.x > canvas.right || cube_pos.x + side_length < canvas.left ||
            cube_pos.y > canvas.bottom || cube_pos.y + side_length < canvas.top)
        {
            return;
        }

        // check if cube is partially outside the canvas
        if (cube_pos.x < canvas.left || cube_pos.x + side_length > canvas.right ||
            cube_pos.y < canvas.top || cube_pos.y + side_length > canvas.bottom)
        {
            partial = true;
        }
    }

    // check if wall exist
    if (cube->exits_[Cube::X_INC] == 0)
        property.e_wall = true;

    if (cube->exits_[Cube::X_DEC] == 0)
        property.w_wall = true;

    if (cube->exits_[Cube::Y_DEC] == 0)
        property.n_wall = true;

    if (cube->exits_[Cube::Y_INC] == 0)
        property.s_wall = true;

    // check if pillar exist
    if (property.n_wall == false && property.e_wall == false)
    {
        shared_ptr<Cube> ne_cube = cube->adjacent_cubes_[Cube::X_INC_Y_DEC];

        if (ne_cube != nullptr &&
            ne_cube->exits_[Cube::X_DEC] != 0 &&
            ne_cube->exits_[Cube::Y_INC] != 0)
        {
            property.ne_pillar = false;
        }
    }

    if (property.n_wall == false && property.w_wall == false)
    {
        shared_ptr<Cube> nw_cube = cube->adjacent_cubes_[Cube::X_DEC_Y_DEC];

        if (nw_cube != nullptr &&
            nw_cube->exits_[Cube::X_INC] != 0 &&
            nw_cube->exits_[Cube::Y_INC] != 0)
        {
            property.nw_pillar = false;
        }
    }

    if (property.s_wall == false && property.e_wall == false)
    {
        shared_ptr<Cube> se_cube = cube->adjacent_cubes_[Cube::X_INC_Y_INC];

        if (se_cube != nullptr &&
            se_cube->exits_[Cube::X_DEC] != 0 &&
            se_cube->exits_[Cube::Y_DEC] != 0)
        {
            property.se_pillar = false;
        }
    }

    if (property.s_wall == false && property.w_wall == false)
    {
        shared_ptr<Cube> sw_cube = cube->adjacent_cubes_[Cube::X_DEC_Y_INC];

        if (sw_cube != nullptr &&
            sw_cube->exits_[Cube::X_INC] != 0 &&
            sw_cube->exits_[Cube::Y_DEC] != 0)
        {
            property.sw_pillar = false;
        }
    }

    // get lines in each direction
    lines.clear();
    render_cube_frame_west(property,
        cube_pos, side_length, wall_length, pillar_length, lines);

    render_cube_frame_north(property,
        cube_pos, side_length, wall_length, pillar_length, lines);

    render_cube_frame_east(property,
        cube_pos, side_length, wall_length, pillar_length, lines);

    render_cube_frame_south(property,
        cube_pos, side_length, wall_length, pillar_length, lines);

    if (partial)
    {
        for (auto it = lines.begin(); it != lines.end(); it++)
        {
            MapFrame::Line cut;

            if (cut_line(canvas, *it, cut))
            {
                cache.push_back(cut);
            }
        }
    }
    else
    {
        cache.insert(cache.end(), lines.begin(), lines.end());
    }

    // recursively travel east, west, north and south cube
    render_cube_frame(render_in_canvas, canvas,
        cube->adjacent_cubes_[Cube::Y_DEC],
        D2D1::Point2F(cube_pos.x, cube_pos.y - side_length), side_length, wall_length, record, cache);

    render_cube_frame(render_in_canvas, canvas,
        cube->adjacent_cubes_[Cube::X_INC],
        D2D1::Point2F(cube_pos.x + side_length, cube_pos.y), side_length, wall_length, record, cache);

    render_cube_frame(render_in_canvas, canvas,
        cube->adjacent_cubes_[Cube::Y_INC],
        D2D1::Point2F(cube_pos.x, cube_pos.y + side_length), side_length, wall_length, record, cache);

    render_cube_frame(render_in_canvas, canvas,
        cube->adjacent_cubes_[Cube::X_DEC],
        D2D1::Point2F(cube_pos.x - side_length, cube_pos.y), side_length, wall_length, record, cache);

    return;
}

inline void MapFrame::render_cube_frame_west(
    const CubeProperty& property,
    D2D1_POINT_2F cube_pos,
    FLOAT side_length, FLOAT wall_length, FLOAT pillar_length, list<MapFrame::Line>& cache)
{
    D2D1_POINT_2F start, end;

    // when there is no west wall, check if need to draw pillar
    if (!property.w_wall)
    {
        if (property.nw_pillar && !property.n_wall)
        {
            start = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y);
            end = D2D1::Point2F(start.x, start.y + pillar_length);
            cache.push_back(MapFrame::Line(start, end));
        }

        if (property.sw_pillar && !property.s_wall)
        {
            start = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y + pillar_length + wall_length);
            end = D2D1::Point2F(start.x, start.y + pillar_length);
            cache.push_back(MapFrame::Line(start, end));
        }

        return;
    }

    // west wall exist, check the start and end point for left wall
    if (property.n_wall)
    {
        start = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y + pillar_length);
    }
    else
    {
        start = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y);
    }

    if (property.s_wall)
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y + pillar_length + wall_length);
    }
    else
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y + pillar_length + wall_length + pillar_length);
    }

    cache.push_back(MapFrame::Line(start, end));

    return;
}

inline void MapFrame::render_cube_frame_north(
    const CubeProperty& property, D2D1_POINT_2F cube_pos, FLOAT side_length, FLOAT wall_length, FLOAT pillar_length,
    list<MapFrame::Line>& cache)
{
    D2D1_POINT_2F start, end;

    // when there is no north wall, check if need to draw pillar
    if (!property.n_wall)
    {
        if (property.nw_pillar && !property.w_wall)
        {
            start = D2D1::Point2F(cube_pos.x, cube_pos.y + pillar_length);
            end = D2D1::Point2F(start.x + pillar_length, start.y);
            cache.push_back(MapFrame::Line(start, end));
        }

        if (property.ne_pillar && !property.e_wall)
        {
            start = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length);
            end = D2D1::Point2F(start.x + pillar_length, start.y);
            cache.push_back(MapFrame::Line(start, end));
        }

        return;
    }

    // north wall exist, check the start and end point for left wall
    if (property.w_wall)
    {
        start = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y + pillar_length);
    }
    else
    {
        start = D2D1::Point2F(cube_pos.x, cube_pos.y + pillar_length);
    }

    if (property.e_wall)
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length);
    }
    else
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length + wall_length + pillar_length, cube_pos.y + pillar_length);
    }

    cache.push_back(MapFrame::Line(start, end));

    return;
}

inline void MapFrame::render_cube_frame_east(
    const CubeProperty& property, D2D1_POINT_2F cube_pos, FLOAT side_length, FLOAT wall_length, FLOAT pillar_length,
    list<MapFrame::Line>& cache)
{
    D2D1_POINT_2F start, end;

    // when there is no east wall, check if need to draw pillar
    if (!property.e_wall)
    {
        if (property.ne_pillar && !property.n_wall)
        {
            start = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y);
            end = D2D1::Point2F(start.x, start.y + pillar_length);
            cache.push_back(MapFrame::Line(start, end));
        }

        if (property.se_pillar && !property.s_wall)
        {
            start = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length + wall_length);
            end = D2D1::Point2F(start.x, start.y + pillar_length);
            cache.push_back(MapFrame::Line(start, end));
        }

        return;
    }

    // east wall exist, check the start and end point for left wall
    if (property.n_wall)
    {
        start = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length);
    }
    else
    {
        start = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y);
    }

    if (property.s_wall)
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length + wall_length);
    }
    else
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length + wall_length + pillar_length);
    }

    cache.push_back(MapFrame::Line(start, end));

    return;
}

inline void MapFrame::render_cube_frame_south(
    const CubeProperty& property, D2D1_POINT_2F cube_pos, FLOAT side_length, FLOAT wall_length, FLOAT pillar_length,
    list<MapFrame::Line>& cache)
{
    D2D1_POINT_2F start, end;

    // when there is no south wall, check if need to draw pillar
    if (!property.s_wall)
    {
        if (property.sw_pillar && !property.w_wall)
        {
            start = D2D1::Point2F(cube_pos.x, cube_pos.y + pillar_length + wall_length);
            end = D2D1::Point2F(start.x + pillar_length, start.y);
            cache.push_back(MapFrame::Line(start, end));
        }

        if (property.se_pillar && !property.e_wall)
        {
            start = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length + wall_length);
            end = D2D1::Point2F(start.x + pillar_length, start.y);
            cache.push_back(MapFrame::Line(start, end));
        }

        return;
    }

    // south wall exist, check the start and end point for left wall
    if (property.w_wall)
    {
        start = D2D1::Point2F(cube_pos.x + pillar_length, cube_pos.y + pillar_length + wall_length);
    }
    else
    {
        start = D2D1::Point2F(cube_pos.x, cube_pos.y + pillar_length + wall_length);
    }

    if (property.e_wall)
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length + wall_length, cube_pos.y + pillar_length + wall_length);
    }
    else
    {
        end = D2D1::Point2F(cube_pos.x + pillar_length + wall_length + pillar_length, cube_pos.y + pillar_length + wall_length);
    }

    cache.push_back(MapFrame::Line(start, end));

    return;
}

inline bool MapFrame::cut_line(const D2D_RECT_F& canvas, const MapFrame::Line& line, MapFrame::Line& cut)
{
    // case 1: start is not in the canvas
    if (is_in_rect(canvas, line.start))
    {
        // both points are in the canvas
        if (is_in_rect(canvas, line.end))
        {
            cut.start = line.start;
            cut.end = line.end;
            return true;
        }

        // pt start is in the canvas and pt end is not
        cut.start = line.start;
        if (line.start.x == line.end.x)
        {
            // vertical line
            cut.end.x = line.end.x;
            cut.end.y = (line.end.y < canvas.top) ? canvas.top : canvas.bottom;
        }
        else
        {
            // horizontal line
            cut.end.x = (line.end.x < canvas.left) ? canvas.left : canvas.right;
            cut.end.y = line.end.y;
        }
        return true;
    }
    else if (is_in_rect(canvas, line.end))
    {
        // pt start is not in the canvas and pt end is
        cut.end = line.end;
        if (line.start.x == line.end.x)
        {
            // vertical line
            cut.start.x = line.start.x;
            cut.start.y = (line.start.y < canvas.top) ? canvas.top : canvas.bottom;
        }
        else
        {
            // horizontal line
            cut.start.x = (line.start.x < canvas.left) ? canvas.left : canvas.right;
            cut.start.y = line.start.y;
        }
        return true;
    }
    else if (line.start.x == line.end.x && line.start.x >= canvas.left && line.start.x <= canvas.right &&
        ((line.start.y <= canvas.top && line.end.y >= canvas.bottom) ||
            (line.start.y >= canvas.bottom && line.end.y <= canvas.top))
        )
    {
        // vertical line that cross over the canvas
        cut.start.x = line.start.x;
        cut.start.y = canvas.top;
        cut.end.x = line.end.x;
        cut.end.y = canvas.bottom;
        return true;
    }
    else if (line.start.y == line.end.y && line.start.y >= canvas.top && line.start.y <= canvas.bottom &&
        ((line.start.x <= canvas.left && line.end.x >= canvas.right) ||
            (line.start.x >= canvas.right && line.end.x <= canvas.left))
        )
    {
        // horizontal line that cross over the canvas
        cut.start.x = canvas.left;
        cut.start.y = line.start.y;
        cut.end.x = canvas.right;
        cut.end.y = line.end.y;
        return true;
    }
    else
    {
        // both points are not in the canvas
        return false;
    }
}

inline bool MapFrame::is_in_rect(const D2D_RECT_F& canvas, const D2D1_POINT_2F& pt)
{
    return (pt.x >= canvas.left && pt.x <= canvas.right && pt.y >= canvas.top && pt.y <= canvas.bottom);
}

inline void MapFrame::lines_to_enclosed_lines(list<MapFrame::Line>& lines, list<MapFrame::Line>& out)
{
    D2D1_POINT_2F start, end;
    auto it = lines.begin();

    if (it == lines.end())
    {
        // lines is empty
        return;
    }

    // put first line into geometry and mark the geometry start and end
    start = (*it).start;
    end = (*it).end;
    it = lines.erase(it);

    out.push_back(MapFrame::Line(start, end));

    // check if any line in lines that can add into geometry
    while (true)
    {
        bool stop_search = true;

        if (lines.size() == 0)
        {
            break;
        }

        for (it = lines.begin(); it != lines.end(); it++)
        {
            D2D1_POINT_2F start_segment, end_segment;
            start_segment = (*it).start;
            end_segment = (*it).end;

            if (start.x == start_segment.x && start.y == start_segment.y && end.x == end_segment.x && end.y == end_segment.y)
            {
                // enclose one geometry
                out.push_back(MapFrame::Line(start, end));
                stop_search = true;
                break;
            }
            else if (start.x == start_segment.x && start.y == start_segment.y)
            {
                out.push_front(MapFrame::Line(end_segment, start_segment));
                lines.erase(it);
                start = end_segment;
                stop_search = false;
                break;
            }
            else if (start.x == end_segment.x && start.y == end_segment.y)
            {
                out.push_front(MapFrame::Line(start_segment, end_segment));
                lines.erase(it);
                start = start_segment;
                stop_search = false;
                break;
            }
            else if (end.x == start_segment.x && end.y == start_segment.y)
            {
                out.push_back(MapFrame::Line(start_segment, end_segment));
                lines.erase(it);
                end = end_segment;
                stop_search = false;
                break;
            }
            else if (end.x == end_segment.x && end.y == end_segment.y)
            {
                out.push_back(MapFrame::Line(end_segment, start_segment));
                lines.erase(it);
                end = start_segment;
                stop_search = false;
                break;
            }
        }

        if (stop_search)
        {
            break;
        }
    }

    // enclose the geometry if needed 
    if (out.size() <= 1)
    {
        return;
    }

    it = out.begin();
    start = it->start;
    it = out.end();
    it--;
    end = it->end;

    if (start.x == end.x && start.y == end.y)
    {
        // no need to enclose
    }
    else if (start.x == end.x || start.y == end.y)
    {
        // enclose vertically / horizontally
        out.push_back(MapFrame::Line(end, start));
    }
    else
    {
        // TODO: need two/three lines to enclose, require more data to determine
        // which side to enclose, line end and start does not work in certain situation
        out.push_back(MapFrame::Line(end, start));
    }

    simplify_lines(out);
}

inline void MapFrame::simplify_lines(list<MapFrame::Line>& lines)
{
    auto it = lines.begin();

    if (it == lines.end())
    {
        return;
    }

    while (true)
    {
        MapFrame::Line first, second;

        first = *it;
        it++;
        if (it == lines.end())
        {
            break;
        }

        second = *it;
        if (first.start.x == second.end.x) // continual vertical line
        {
            it = lines.erase(it);
            it--;
            it->end = second.end;
        }
        else if (first.start.y == second.end.y) // continual horizontal line
        {
            it = lines.erase(it);
            it--;
            it->end = second.end;
        }
    }
}

MapFrame::Line::Line()
{
    start = D2D1::Point2F(0.f, 0.f);
    end = D2D1::Point2F(0.f, 0.f);
}

MapFrame::Line::Line(D2D1_POINT_2F from, D2D1_POINT_2F to) : start(from), end(to)
{
}

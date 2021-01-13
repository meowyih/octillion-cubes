#include <cstdlib>

#include "error/macrolog.hpp"
#include "world/worldmap.hpp"

#include "MazeBitmapCreator.hpp"
#include "Config.hpp"

MazeBitmapCreator::MazeBitmapCreator()
{
    // last_pos_, a position that does not exist, same as last_depth_
    last_pos_.set(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
    last_depth_ = -1;
    w_ = -1;
    h_ = -1;
    map_ = nullptr;
}

void MazeBitmapCreator::set(int w, int h, std::shared_ptr<octillion::WorldMap> map)
{
    // world map
    if (map_ == nullptr)
    {
        map_ = map;
        
        // create world coordinate for all cubes
        for (auto it = map_->cubes_.begin(); it != map_->cubes_.end(); it++)
        {
            octillion::CubePosition loc = it->first;
            cubes_.insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>
                (it->first, std::make_shared<Cube3d>(it->second, 10))
            );
        }
    }

    // init the width, height and allocate bitmap buffer
    if (w != w_ || h != h_)
    {
        buffer_ = std::make_shared<std::vector<BYTE>>();
        buffer_->resize(w * h * Config::get_instance().pixel_size());
        w_ = w;
        h_ = h;
        width_size_byte_ = w * Config::get_instance().pixel_size();
    }
}

std::shared_ptr<std::vector<BYTE>> MazeBitmapCreator::render(octillion::CubePosition loc,  Matrix<double> matrix, int depth, int flag)
{
    bool need_traversal = !(last_pos_ == loc && last_depth_ == depth);
    bool need_render = need_traversal || !(last_matrix_.equal( matrix ) && last_render_flag_ == flag);

    // bunch of 3d points prepare for draw on the buffer
    std::vector<Point3d> pts;

    if (need_render)
    {
        std::memset(buffer_->data(), 0, buffer_->size());
    }
    else
    {
        return buffer_;
    }

    // traverse the cubes
    if (need_traversal)
    {
        cubes_viewable_.clear();
        cubes_same_.clear();

        cubes_upper_.clear();
        cubes_upper_.resize((size_t)depth);

        cubes_lower_.clear();
        cubes_lower_.resize((size_t)depth);

        traversal(TRAVERSAL_FLAG_ALL, depth, loc, cubes_, cubes_viewable_);

        last_pos_ = loc;
        last_depth_ = depth;
    }

    // seperate by level, don't do this if no need to traversal the cubes
    for (auto it = cubes_viewable_.begin(); it != cubes_viewable_.end(); it++)
    {
        if (!need_traversal)
            break;

        int floor = (*it).first.z() - loc.z();

        if (std::abs(floor) >= depth)
            continue; // should not happen in BFS

        if (floor == 0)
        {
            cubes_same_.insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(
                (*it).first, (*it).second));
        }
        else if (floor > 0 )
        {
            cubes_upper_.at(floor-1).insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(
                (*it).first, (*it).second));
        }
        else
        {
            cubes_lower_.at(std::abs(floor-1)).insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(
                (*it).first, (*it).second));
        }
    }

    // render the current plain
    for (auto it = cubes_same_.begin(); it != cubes_same_.end(); it++)
    {
        (*it).second->render(matrix, pts);
    }

    // render the higher plain
    for (auto itvec = cubes_upper_.begin(); itvec != cubes_upper_.end(); itvec++)
    {
        if (flag == RENDER_FLAG_PLAIN)
            break;

        // render each layer
        for (auto it = (*itvec).begin(); it != (*itvec).end(); it++)
        {
            (*it).second->render(matrix, pts);
        }
    }

    // render the lower plain
    for (auto itvec = cubes_lower_.begin(); itvec != cubes_lower_.end(); itvec++)
    {
        if (flag == RENDER_FLAG_PLAIN)
            break;

        // render each layer
        for (auto it = (*itvec).begin(); it != (*itvec).end(); it++)
        {
            (*it).second->render(matrix, pts);
        }
    }

    // TODO, change color for each cube or each floor later
    int color = 0xFFFFFF;
    for (auto it = pts.begin(); it != pts.end(); it++)
    {
        if ((*it).x_ >= w_ || (*it).x_ < 0)
            continue;

        size_t anchor = (*it).y_ * width_size_byte_ + (*it).x_ * Config::get_instance().pixel_size();
        if (anchor >= buffer_->size() || anchor < 0)
            continue;

        BYTE* data = buffer_->data();
        memcpy(data + anchor, &color, 3);
    }

    // draw center floor
    pts.clear();
    auto it = cubes_same_.find(loc);
    if (it != cubes_same_.end())
    {
        (*it).second->render_center(matrix, pts);
    }

    color = 0xFF0000;
    for (auto it = pts.begin(); it != pts.end(); it++)
    {
        size_t anchor = (*it).y_ * width_size_byte_ + (*it).x_ * Config::get_instance().pixel_size();
        if (anchor >= buffer_->size() || anchor < 0)
            continue;

        BYTE* data = buffer_->data();
        memcpy(data + anchor, &color, 3);
    }

    last_matrix_ = matrix;
    last_render_flag_ = flag;

    return buffer_;
}

std::shared_ptr<std::vector<BYTE>> MazeBitmapCreator::render(octillion::CubePosition loc, Matrix<double> m1, Matrix<double> m2, Matrix<double> m3, int depth, int flag, HDC hdc)
{
    bool need_traversal = !(last_pos_ == loc && last_depth_ == depth);
    bool need_render = need_traversal || !(last_matrix_.equal(m3) && last_render_flag_ == flag);

    // bunch of 3d points prepare for draw on the buffer
    std::vector<Point3d> pts;

    if (need_render)
    {
        std::memset(buffer_->data(), 0, buffer_->size());
    }
    else
    {
        return buffer_;
    }

    // traverse the cubes
    if (need_traversal)
    {
        cubes_viewable_.clear();
        cubes_same_.clear();

        cubes_upper_.clear();
        cubes_upper_.resize((size_t)depth);

        cubes_lower_.clear();
        cubes_lower_.resize((size_t)depth);

        traversal(TRAVERSAL_FLAG_ALL, depth, loc, cubes_, cubes_viewable_);

        last_pos_ = loc;
        last_depth_ = depth;
    }

    // seperate by level, don't do this if no need to traversal the cubes
    for (auto it = cubes_viewable_.begin(); it != cubes_viewable_.end(); it++)
    {
        if (!need_traversal)
            break;

        int floor = (*it).first.z() - loc.z();

        if (std::abs(floor) >= depth)
            continue; // should not happen in BFS

        if (floor == 0)
        {
            cubes_same_.insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(
                    (*it).first, (*it).second));
        }
        else if (floor > 0)
        {
            cubes_upper_.at(floor - 1).insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(
                    (*it).first, (*it).second));
        }
        else
        {
            cubes_lower_.at(std::abs(floor - 1)).insert(
                std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(
                    (*it).first, (*it).second));
        }
    }

    // render the current plain
    for (auto it = cubes_same_.begin(); it != cubes_same_.end(); it++)
    {
        (*it).second->render(m1, m2, m3, pts, hdc);
    }

    // render the higher plain
    for (auto itvec = cubes_upper_.begin(); itvec != cubes_upper_.end(); itvec++)
    {
        if (flag == RENDER_FLAG_PLAIN)
            break;

        // render each layer
        for (auto it = (*itvec).begin(); it != (*itvec).end(); it++)
        {
            (*it).second->render(m1, m2, m3, pts, hdc);
        }
    }

    // render the lower plain
    for (auto itvec = cubes_lower_.begin(); itvec != cubes_lower_.end(); itvec++)
    {
        if (flag == RENDER_FLAG_PLAIN)
            break;

        // render each layer
        for (auto it = (*itvec).begin(); it != (*itvec).end(); it++)
        {
            (*it).second->render(m1, m2, m3, pts, hdc);
        }
    }

    // TODO, change color for each cube or each floor later
    int color = 0xFFFFFF;
    for (auto it = pts.begin(); it != pts.end(); it++)
    {
        if ((*it).x_ >= w_ || (*it).x_ < 0)
            continue;

        size_t anchor = (*it).y_ * width_size_byte_ + (*it).x_ * Config::get_instance().pixel_size();
        if (anchor >= buffer_->size() || anchor < 0)
            continue;

        BYTE* data = buffer_->data();
        memcpy(data + anchor, &color, 3);
    }

    // draw center floor
    pts.clear();
    auto it = cubes_same_.find(loc);
    if (it != cubes_same_.end())
    {
        (*it).second->render_center(m3, pts);
    }

    color = 0xFF0000;
    for (auto it = pts.begin(); it != pts.end(); it++)
    {
        size_t anchor = (*it).y_ * width_size_byte_ + (*it).x_ * Config::get_instance().pixel_size();
        if (anchor >= buffer_->size() || anchor < 0)
            continue;

        BYTE* data = buffer_->data();
        memcpy(data + anchor, &color, 3);
    }

    last_matrix_ = m3;
    last_render_flag_ = flag;

    return buffer_;
}

int MazeBitmapCreator::width()
{
    return w_;
}

int MazeBitmapCreator::height()
{
    return h_;
}

std::shared_ptr<Cube3d> MazeBitmapCreator::cube(octillion::CubePosition pos)
{
    auto it = cubes_.find(pos);
    if (it == cubes_.end())
    {
        return nullptr;
    }
    
    return it->second;
}

void MazeBitmapCreator::traversal(
    int flag, int depth,
    octillion::CubePosition loc, 
    std::unordered_map<octillion::CubePosition, std::shared_ptr<Cube3d>>& cubes, 
    std::unordered_map<octillion::CubePosition, std::shared_ptr<Cube3d>>& out)
{
    if (depth == 0)
        return;

    auto it_loc = cubes.find(loc);
    if (it_loc == cubes.end())
        return; // no such cube

    if (out.find(loc) == out.end())
    {
        // insert current loc
        out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(it_loc->first, it_loc->second));
    }

    // Breadth-First Search
    for (int i = 1; i < depth; i++)
    {
        std::size_t out_size = out.size();
        for (auto it_out = out.begin(); it_out != out.end(); it_out++)
        {
            auto ite = cubes.find(octillion::CubePosition(it_out->first, octillion::Cube::X_INC));
            auto itw = cubes.find(octillion::CubePosition(it_out->first, octillion::Cube::X_DEC));
            auto itn = cubes.find(octillion::CubePosition(it_out->first, octillion::Cube::Y_INC));
            auto its = cubes.find(octillion::CubePosition(it_out->first, octillion::Cube::Y_DEC));
            auto itu = cubes.find(octillion::CubePosition(it_out->first, octillion::Cube::Z_INC));
            auto itd = cubes.find(octillion::CubePosition(it_out->first, octillion::Cube::Z_DEC));
            switch (flag)
            {
            case TRAVERSAL_FLAG_ALL:
                if (itu != cubes.end() && out.find(itu->first) == out.end())
                    out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(itu->first, itu->second));
                if (itd != cubes.end() && out.find(itd->first) == out.end())
                    out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(itd->first, itd->second));
            case TRAVERSAL_FLAG_PLAIN:
                if (ite != cubes.end() && out.find(ite->first) == out.end())
                    out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(ite->first, ite->second));
                if (itw != cubes.end() && out.find(itw->first) == out.end())
                    out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(itw->first, itw->second));
                if (itn != cubes.end() && out.find(itn->first) == out.end())
                    out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(itn->first, itn->second));
                if (its != cubes.end() && out.find(its->first) == out.end())
                    out.insert(std::pair<octillion::CubePosition, std::shared_ptr<Cube3d>>(its->first, its->second));
            }
        }

        // no more available cubes, we break
        if (out_size == out.size())
        {
            break;
        }
    }
}

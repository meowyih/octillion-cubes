#include "3d/matrix.hpp"
#include "error/macrolog.hpp"

#include "Config.hpp"
#include "Cube3d.hpp"
#include "TextBitmapCreator.hpp"
#include "Cache.hpp"

void Cube3d::render(Matrix<double> m1, Matrix<double> m2, Matrix<double> m3, std::vector<Point3d>& points, HDC hdc)
{
#if 0 // a demo shows how to draw character on floor
    // render floor text
    int inner_floor_length, text_w, text_h, offset_x, offset_y;
    int padding_x, padding_y;
    int max_w;
    int max_h;
    std::vector<Point3d> out;
    
    // 1--2--4--3, 4 points of floor
    Point3d p1 = render_pt(m1, 1);
    Point3d p2 = render_pt(m1, 2);

    // calculate inner floor length for text drawing
    inner_floor_length = std::abs((int)(p2.x_ - p1.x_));
    max_w = max_h = text_w = text_h = inner_floor_length / 2;

    // draw text in the floor
    offset_x = (int)(p1.x_);
    offset_y = (int)(p1.y_);

    if (Cache::get_instance().readCache(text_w, text_h, L'\u0042', out) == false)
    {
        TextBitmapCreator::getBitmapFixHeight(hdc, text_w, text_h, out, L'\u0042');
        Cache::get_instance().writeCache(max_w, max_h, text_w, text_h, L'\u0042', out );
    }

    padding_x = (inner_floor_length - text_w) / 2;
    padding_y = (inner_floor_length - text_h) / 2;

    offset_x += padding_x;
    offset_y += padding_y;

    for (auto it = out.begin(); it != out.end(); it++)
    {
        Point3d pt, pt_new;
        Matrix<double> pm((double)(*it).x_ + offset_x, (double)(*it).y_ + offset_y, (double)(*it).z_);
        Matrix<double> pm_new = pm * m2;
        pt_new.x_ = (int_fast32_t)pm_new.at(0);
        pt_new.y_ = (int_fast32_t)pm_new.at(1);
        pt_new.z_ = (int_fast32_t)pm_new.at(2);

        points.push_back(pt_new);
    }
#endif    
    // render cube
    render(m3, points);
}

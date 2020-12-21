#ifndef OCTILLION_ENGINE3D_CUBE3D_HEADER
#define OCTILLION_ENGINE3D_CUBE3D_HEADER

#include <cstdint>
#include <cmath>
#include <vector>
#include <vector>
#include "3d/matrix.hpp"
#include "world/cube.hpp"

class Point3d
{
public:
    Point3d() : x_(0), y_(0), z_(0) {}
    Point3d(int_fast32_t x, int_fast32_t y, int_fast32_t z) : x_(x), y_(y), z_(z) {}
    void set(int_fast32_t x, int_fast32_t y, int_fast32_t z)
    {
        x_ = x;
        y_ = y;
        z_ = z;
    }
    int_fast32_t x_, y_, z_;

    bool equal(const Point3d& rhs)
    {
        return (x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_);
    }
};

class Cube3d
{
public:
    const static int number_of_point = 38;

public:
    Cube3d() : x_(0), y_(0), z_(0) {}

    Cube3d(std::shared_ptr<octillion::Cube> cube, size_t length )
    {
        cube_ = cube;
        octillion::CubePosition loc = cube_->loc();
        set(loc.x(), loc.y(), loc.z(), length);
    }

    void set(int_fast32_t x, int_fast32_t y, int_fast32_t z, size_t length)
    {
        x_ = x;
        y_ = y;
        z_ = z;

        size_t gap = length / 10;
        size_t side_length = length - gap * 2;

        std::memset(p, 0, sizeof p);
        std::memset(p_ceil, 0, sizeof p_ceil);

        // lower outer square
        p[0].set(x * length, y * length, z * length);
        p[23].set(p[0].x_ + length, p[0].y_ + gap, p[0].z_);
        p[24].set(p[0].x_, p[0].y_ + length, p[0].z_);
        p[25].set(p[0].x_ + length, p[0].y_ + length, p[0].z_);

        p[30].set(p[0].x_ + gap, p[0].y_, p[0].z_);
        p[31].set(p[0].x_, p[0].y_ + gap, p[0].z_);

        p[32].set(p[30].x_ + side_length, p[30].y_, p[30].z_);
        p[33].set(p[32].x_ + gap, p[32].y_ + gap, p[32].z_);

        p[34].set(p[31].x_, p[31].y_ + side_length, p[31].z_);
        p[35].set(p[34].x_ + gap, p[34].y_ + gap, p[34].z_);

        p[36].set(p[25].x_, p[25].y_ - gap, p[25].z_);
        p[37].set(p[25].x_ - gap, p[25].y_, p[25].z_);

        // lower inner square
        p[1].set(p[0].x_ + gap, p[0].y_ + gap, p[0].z_);
        p[2].set(p[1].x_ + side_length, p[1].y_, p[0].z_);
        p[3].set(p[1].x_, p[1].y_ + side_length, p[0].z_);
        p[4].set(p[2].x_, p[3].y_, p[0].z_);

        p[17].set(p[4].x_ - 2, p[4].y_ - 1, p[4].z_);
        p[18].set(p[17].x_ - 1, p[17].y_ - 1, p[17].z_);
        p[19].set(p[17].x_ + 1, p[17].y_ - 1, p[17].z_);

        p[20].set(p[1].x_ + 2, p[1].y_ + 1, p[1].z_);
        p[21].set(p[20].x_ - 1, p[20].y_ + 1, p[20].z_);
        p[22].set(p[20].x_ + 1, p[20].y_ + 1, p[20].z_);

        std::memcpy(p_ceil, p, sizeof p);

        for (size_t i = 0; i < number_of_point; i++)
        {
            p_ceil[i].z_ += length;
        }
    }

/*

    24  35           37  25
    34   3------------4  36
         |            |
         |            |
         |            |
    31   1------------2  33
     0  30           32  23

*/
// no use: 5/6/7/8/
//         9/10/11/12/13/14/15/16
//         26/27/28/29
/* up-down arrow
          17
        18  19

 21  22
   20
 */

public:
    int_fast32_t x_, y_, z_;
    Point3d p[number_of_point];
    Point3d p_ceil[number_of_point];
    std::shared_ptr<octillion::Cube> cube_ = nullptr;

public:
    Point3d render_pt(Matrix<double> matrix, int idx)
    {
        if (idx >= number_of_point || idx < 0)
        {
            return Point3d();
        }

        Matrix<double> pt((double)p[idx].x_, (double)p[idx].y_, (double)p[idx].z_);
        Matrix<double> pt_new = pt * matrix;
        
        return Point3d((int_fast32_t)pt_new.at(0), (int_fast32_t)pt_new.at(1), (int_fast32_t)pt_new.at(2));
    }

    void render_center(Matrix<double> matrix, std::vector<Point3d>& points)
    {
        std::vector<Point3d> out;

        Point3d p1, p2, p3, p4;
        Matrix<double> m1((double)p[1].x_, (double)p[1].y_, (double)p[1].z_);
        Matrix<double> m2((double)p[2].x_, (double)p[2].y_, (double)p[2].z_);
        Matrix<double> m3((double)p[3].x_, (double)p[3].y_, (double)p[3].z_);
        Matrix<double> m4((double)p[4].x_, (double)p[4].y_, (double)p[4].z_);

        Matrix<double> mt1 = m1 * matrix;
        Matrix<double> mt2 = m2 * matrix;
        Matrix<double> mt3 = m3 * matrix;
        Matrix<double> mt4 = m4 * matrix;

        p1.x_ = (int_fast32_t)mt1.at(0); p1.y_ = (int_fast32_t)mt1.at(1); p1.z_ = (int_fast32_t)mt1.at(2);
        p2.x_ = (int_fast32_t)mt2.at(0); p2.y_ = (int_fast32_t)mt2.at(1); p2.z_ = (int_fast32_t)mt2.at(2);
        p3.x_ = (int_fast32_t)mt3.at(0); p3.y_ = (int_fast32_t)mt3.at(1); p3.z_ = (int_fast32_t)mt3.at(2);
        p4.x_ = (int_fast32_t)mt4.at(0); p4.y_ = (int_fast32_t)mt4.at(1); p4.z_ = (int_fast32_t)mt4.at(2);

        out.clear();
        bresenham3d(p1, p2, out);
        points.insert(points.end(), out.begin(), out.end());

        out.clear();
        bresenham3d(p2, p4, out);
        points.insert(points.end(), out.begin(), out.end());

        out.clear();
        bresenham3d(p3, p4, out);
        points.insert(points.end(), out.begin(), out.end());

        out.clear();
        bresenham3d(p1, p3, out);
        points.insert(points.end(), out.begin(), out.end());
    }

    void render(Matrix<double> matrix, std::vector<Point3d>& points)
    {
        std::vector<Point3d> out;
        Point3d p_new[number_of_point];
        Point3d p_new_ceil[number_of_point];
        Point3d p_nw, p_ne, p_sw, p_se;
        Point3d p_nw_ceil, p_ne_ceil, p_sw_ceil, p_se_ceil;
        bool    draw_nw_pillar = false;
        bool    draw_ne_pillar = false;
        bool    draw_sw_pillar = false;
        bool    draw_se_pillar = false;

        for (int i = 0; i < number_of_point; i++)
        {
            Matrix<double> pt((double)p[i].x_, (double)p[i].y_, (double)p[i].z_);
            Matrix<double> pt_translated = pt * matrix;
            p_new[i].x_ = (int_fast32_t)pt_translated.at(0);
            p_new[i].y_ = (int_fast32_t)pt_translated.at(1);
            p_new[i].z_ = (int_fast32_t)pt_translated.at(2);
        }

        for (int i = 0; i < number_of_point; i++)
        {
            Matrix<double> pt((double)p_ceil[i].x_, (double)p_ceil[i].y_, (double)p_ceil[i].z_);
            Matrix<double> pt_translated = pt * matrix;
            p_new_ceil[i].x_ = (int_fast32_t)pt_translated.at(0);
            p_new_ceil[i].y_ = (int_fast32_t)pt_translated.at(1);
            p_new_ceil[i].z_ = (int_fast32_t)pt_translated.at(2);
        }

        // determine the 4 walls' position (ne/nw/se/sw) on floor
        if (cube_->exits_[octillion::Cube::X_DEC] == 0)
        {
            // when there is a west wall
            if (cube_->exits_[octillion::Cube::Y_INC] == 0)
            {
                p_nw = p_new[3]; // has north wall
                p_nw_ceil = p_new_ceil[3];
            }
            else
            {
                p_nw = p_new[35]; // no north wall
                p_nw_ceil = p_new_ceil[35];
            }

            if (cube_->exits_[octillion::Cube::Y_DEC] == 0)
            {
                p_sw = p_new[1]; // has south wall
                p_sw_ceil = p_new_ceil[1];
            }
            else
            {
                p_sw = p_new[30]; // no south wall
                p_sw_ceil = p_new_ceil[30];
            }
        }

        if (cube_->exits_[octillion::Cube::X_INC] == 0)
        {
            // when there is a east wall
            if (cube_->exits_[octillion::Cube::Y_INC] == 0)
            {
                p_ne = p_new[4]; // has north wall
                p_ne_ceil = p_new_ceil[4];
            }
            else
            {
                p_ne = p_new[37]; // no north wall
                p_ne_ceil = p_new_ceil[37];
            }

            if (cube_->exits_[octillion::Cube::Y_DEC] == 0)
            {
                p_se = p_new[2]; // has south wall
                p_se_ceil = p_new_ceil[2];
            }
            else
            {
                p_se = p_new[32]; // no south wall
                p_se_ceil = p_new_ceil[32];
            }
        }

        if (cube_->exits_[octillion::Cube::Y_INC] == 0)
        {
            // when there is a north wall
            if (cube_->exits_[octillion::Cube::X_INC] == 0)
            {
                p_ne = p_new[4]; // has east wall
                p_ne_ceil = p_new_ceil[4];
            }
            else
            {
                p_ne = p_new[36]; // has east wall
                p_ne_ceil = p_new_ceil[36];
            }

            if (cube_->exits_[octillion::Cube::X_DEC] == 0)
            {
                p_nw = p_new[3]; // has west wall
                p_nw_ceil = p_new_ceil[3];
            }
            else
            {
                p_nw = p_new[34]; // no west wall
                p_nw_ceil = p_new_ceil[34];
            }
        }
#
        if (cube_->exits_[octillion::Cube::Y_DEC] == 0)
        {
            // when there is a south wall
            if (cube_->exits_[octillion::Cube::X_INC] == 0)
            {
                p_se = p_new[2]; // has east wall
                p_se_ceil = p_new_ceil[2];
            }
            else
            {
                p_se = p_new[23]; // no east wall
                p_se_ceil = p_new_ceil[23];
            }

            if (cube_->exits_[octillion::Cube::X_DEC] == 0)
            {
                p_sw = p_new[1]; // has west wall
                p_sw_ceil = p_new_ceil[1];
            }
            else
            {
                p_sw = p_new[31]; // no west wall
                p_sw_ceil = p_new_ceil[31];
            }
        }

        // draw corner
        if ( cube_->adjacent_cubes_[octillion::Cube::X_INC_Y_INC] == 0 )
        {
            // when there is a north east cube
            if (cube_->exits_[octillion::Cube::X_INC] != 0 && 
                cube_->exits_[octillion::Cube::Y_INC] != 0 )
            {
                // when there has no wall at north and east
                out.clear();
                bresenham3d(p_new[4], p_new[37], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new[4], p_new[36], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new[4], p_new_ceil[4], out);
                points.insert(points.end(), out.begin(), out.end());
            }
        }

        if (cube_->adjacent_cubes_[octillion::Cube::X_DEC_Y_INC] == 0)
        {
            // when there is a north west cube
            if (cube_->exits_[octillion::Cube::X_DEC] != 0 &&
                cube_->exits_[octillion::Cube::Y_INC] != 0)
            {
                // when there has no wall at north and west
                out.clear();
                bresenham3d(p_new[3], p_new[34], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new[3], p_new[35], out);
                points.insert(points.end(), out.begin(), out.end());

                // ceiling
                out.clear();
                bresenham3d(p_new_ceil[3], p_new_ceil[34], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new_ceil[3], p_new_ceil[35], out);
                points.insert(points.end(), out.begin(), out.end());

                // pillar
                out.clear();
                bresenham3d(p_new[3], p_new_ceil[3], out);
                points.insert(points.end(), out.begin(), out.end());
            }
        }

        if (cube_->adjacent_cubes_[octillion::Cube::X_INC_Y_DEC] == 0)
        {
            // when there is a south east cube
            if (cube_->exits_[octillion::Cube::X_INC] != 0 &&
                cube_->exits_[octillion::Cube::Y_DEC] != 0)
            {
                // when there has no wall at south and east
                out.clear();
                bresenham3d(p_new[2], p_new[33], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new[2], p_new[32], out);
                points.insert(points.end(), out.begin(), out.end());

                // ceiling
                out.clear();
                bresenham3d(p_new_ceil[2], p_new_ceil[33], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new_ceil[2], p_new_ceil[32], out);
                points.insert(points.end(), out.begin(), out.end());

                // pillar
                out.clear();
                bresenham3d(p_new[2], p_new_ceil[2], out);
                points.insert(points.end(), out.begin(), out.end());
            }
        }

        if (cube_->adjacent_cubes_[octillion::Cube::X_DEC_Y_DEC] == 0)
        {
            // when there is a south west cube
            if (cube_->exits_[octillion::Cube::X_DEC] != 0 &&
                cube_->exits_[octillion::Cube::Y_DEC] != 0)
            {
                // when there has no wall at south and east
                out.clear();
                bresenham3d(p_new[1], p_new[30], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new[1], p_new[31], out);
                points.insert(points.end(), out.begin(), out.end());

                // ceiling
                out.clear();
                bresenham3d(p_new_ceil[1], p_new_ceil[30], out);
                points.insert(points.end(), out.begin(), out.end());

                out.clear();
                bresenham3d(p_new_ceil[1], p_new_ceil[31], out);
                points.insert(points.end(), out.begin(), out.end());

                // pillar
                out.clear();
                bresenham3d(p_new[1], p_new_ceil[1], out);
                points.insert(points.end(), out.begin(), out.end());
            }
        }

        // draw west floor
        if (cube_->exits_[octillion::Cube::X_DEC] == 0)
        {
            out.clear();
            bresenham3d(p_nw, p_sw, out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_nw_ceil, p_sw_ceil, out);
            points.insert(points.end(), out.begin(), out.end());

            draw_nw_pillar = p_nw.equal(p_new[3]) ? true : false;
            draw_sw_pillar = p_sw.equal(p_new[1]) ? true : false;
        }

        // draw east floor
        if (cube_->exits_[octillion::Cube::X_INC] == 0)
        {
            out.clear();
            bresenham3d(p_ne, p_se, out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_ne_ceil, p_se_ceil, out);
            points.insert(points.end(), out.begin(), out.end());

            draw_ne_pillar = p_ne.equal(p_new[4]) ? true : false;
            draw_se_pillar = p_se.equal(p_new[2]) ? true : false;
        }

        // draw north floor
        if (cube_->exits_[octillion::Cube::Y_INC] == 0)
        {
            out.clear();
            bresenham3d(p_ne, p_nw, out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_ne_ceil, p_nw_ceil, out);
            points.insert(points.end(), out.begin(), out.end());

            draw_ne_pillar = p_ne.equal(p_new[4]) ? true : false;
            draw_nw_pillar = p_nw.equal(p_new[3]) ? true : false;
        }

        // draw south floor
        if (cube_->exits_[octillion::Cube::Y_DEC] == 0)
        {
            out.clear();
            bresenham3d(p_se, p_sw, out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_se_ceil, p_sw_ceil, out);
            points.insert(points.end(), out.begin(), out.end());

            draw_se_pillar = p_se.equal(p_new[2]) ? true : false;
            draw_sw_pillar = p_sw.equal(p_new[1]) ? true : false;
        }

        // draw pillar
        if (draw_ne_pillar)
        {
            out.clear();
            bresenham3d(p_new[4], p_new_ceil[4], out);
            points.insert(points.end(), out.begin(), out.end());
        }

        if (draw_nw_pillar)
        {
            out.clear();
            bresenham3d(p_new[3], p_new_ceil[3], out);
            points.insert(points.end(), out.begin(), out.end());
        }

        if (draw_se_pillar)
        {
            out.clear();
            bresenham3d(p_new[2], p_new_ceil[2], out);
            points.insert(points.end(), out.begin(), out.end());
        }

        if (draw_sw_pillar)
        {
            out.clear();
            bresenham3d(p_new[1], p_new_ceil[1], out);
            points.insert(points.end(), out.begin(), out.end());
        }
        
        // up arrow
        if (cube_->exits_[octillion::Cube::Z_INC] > 0)
        {
            out.clear();
            bresenham3d(p_new[17], p_new[18], out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_new[17], p_new[19], out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_new[18], p_new[19], out);
            points.insert(points.end(), out.begin(), out.end());
        }

        // down arrow
        if (cube_->exits_[octillion::Cube::Z_DEC] > 0)
        {
            out.clear();
            bresenham3d(p_new[20], p_new[21], out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_new[20], p_new[22], out);
            points.insert(points.end(), out.begin(), out.end());

            out.clear();
            bresenham3d(p_new[21], p_new[22], out);
            points.insert(points.end(), out.begin(), out.end());
        }
    }

    // line3d uses Bresenham's algorithm to generate the 3 dimensional points on a
    // line from (x1, y1, z1) to (x2, y2, z2), reference implementation found here
    // http://www.ict.griffith.edu.au/anthony/info/graphics/bresenham.procs (3D)
    inline static void bresenham3d(Point3d pt1, Point3d pt2, std::vector<Point3d>& out)
    {
        Point3d pos(pt1);
        Point3d delta( pt2.x_ - pt1.x_, pt2.y_ - pt1.y_, pt2.z_ - pt1.z_);
        Point3d abs_delta(std::abs((int)delta.x_), std::abs((int)delta.y_), std::abs((int)delta.z_));
        Point3d double_delta(abs_delta.x_ << 1, abs_delta.y_ << 1, abs_delta.z_ << 1);

        int_fast32_t x_inc, y_inc, z_inc;

        out.reserve(abs_delta.x_ + abs_delta.y_ + abs_delta.z_);

        x_inc = (delta.x_ < 0) ? -1 : 1;
        y_inc = (delta.y_ < 0) ? -1 : 1;
        z_inc = (delta.z_ < 0) ? -1 : 1;

        if (abs_delta.x_ >= abs_delta.y_ && abs_delta.x_ >= abs_delta.z_)
        {
            int_fast32_t err_1 = double_delta.y_ - abs_delta.x_;
            int_fast32_t err_2 = double_delta.z_ - abs_delta.x_;

            for (int_fast32_t i = 0; i < abs_delta.x_; i++)
            {
                out.push_back(pos);

                if (err_1 > 0)
                {
                    pos.y_ += y_inc;
                    err_1 -= double_delta.x_;
                }

                if (err_2 > 0)
                {
                    pos.z_ += z_inc;
                    err_2 -= double_delta.x_;
                }

                err_1 += double_delta.y_;
                err_2 += double_delta.z_;

                pos.x_ += x_inc;
            }
        }
        else if (abs_delta.y_ >= abs_delta.x_ && abs_delta.y_ >= abs_delta.z_)
        {
            int_fast32_t err_1 = double_delta.x_ - abs_delta.y_;
            int_fast32_t err_2 = double_delta.z_ - abs_delta.y_;

            for (int_fast32_t i = 0; i < abs_delta.y_; i++)
            {
                out.push_back(pos);

                if (err_1 > 0)
                {
                    pos.x_ += x_inc;
                    err_1 -= double_delta.y_;
                }

                if (err_2 > 0)
                {
                    pos.z_ += z_inc;
                    err_2 -= double_delta.y_;
                }

                err_1 += double_delta.x_;
                err_2 += double_delta.z_;

                pos.y_ += y_inc;
            }
        }
        else
        {
            int_fast32_t err_1 = double_delta.y_ - abs_delta.z_;
            int_fast32_t err_2 = double_delta.x_ - abs_delta.z_;

            for (int_fast32_t i = 0; i < abs_delta.z_; i++)
            {
                // if x, y are the same, we don't have to store the pixel
                out.push_back(pos);

                if (err_1 > 0)
                {
                    pos.y_ += y_inc;
                    err_1 -= double_delta.z_;
                }

                if (err_2 > 0)
                {
                    pos.x_ += x_inc;
                    err_2 -= double_delta.z_;
                }

                err_1 += double_delta.y_;
                err_2 += double_delta.x_;

                pos.z_ += z_inc;
            }
        }

        out.push_back(pos);
    }
};

#endif
#pragma once

#include "framework.h"

#include <memory>
#include <vector>
#include <list>
#include <cstdint>
#include <unordered_map>

#include "world/cube.hpp"
#include "world/script.hpp"
#include "world/worldmap.hpp"
#include "world/storage.hpp"
#include "3d/matrix.hpp"
#include "Cube3d.hpp"
#include "TaskAnimation.hpp"
#include "BitmapRoller.hpp"
#include "MazeBitmapCreator.hpp"
#include "TextBitmapCreator.hpp"
#include "TextBitmapCreatorRev.hpp"
#include "ListBitmapCreator.hpp"

class DrawClass
{
public:
    DrawClass();
    ~DrawClass();

public:
    BOOL  OnPaint(HWND hwnd, HDC hdc, BOOL recalc);

    BOOL keypress(int keycode);

    BOOL point(int x, int y);

private:
    inline LONG width();
    inline LONG height();
    inline VOID setRed(LONG w, LONG h, BYTE value);
    inline VOID setBlue(LONG w, LONG h, BYTE value);
    inline VOID setGreen(LONG w, LONG h, BYTE value);
    inline VOID setColor(LONG w, LONG h, int_fast32_t color);
    inline LONG realHeight(LONG height);
    inline void refresh_script();
    inline void setCenterCube(std::shared_ptr<octillion::Cube> dest);
    inline void handle_action(std::shared_ptr<std::vector<octillion::Action>> p_acts);

private:
    void draw_title(HDC hdc);
    void add_console(std::shared_ptr<octillion::StringData>& strdata);
    void draw_console(HDC hdc);
    void draw_map(HDC hdc, octillion::CubePosition loc, Matrix<double> matrix, int flag);
    void draw_map(HDC hdc, octillion::CubePosition loc, Matrix<double> m1, Matrix<double> m2, Matrix<double> m3, int flag);
    void draw_interactive(HDC hdc);

private:
    std::shared_ptr<std::vector<BYTE>> buffer_ = nullptr;
    LONG width_, height_, widthBytes_;
    WORD bytesPerPixel_;

    // block animation
    std::list<TaskAnimation> animation_blocking_;

    // move history
    std::list<std::shared_ptr<octillion::Cube>> move_history_;

	// real drawable size is 
	//    (top_border_ + bottom_border_, 0)
	// -> (height - 1, width - left_border - right_border - 1 )
    LONG left_border_, right_border_, bottom_border_, top_border_;

    // world map
    std::shared_ptr<octillion::WorldMap> map_;

    // player's location
    std::shared_ptr<octillion::Cube> cube_center_;

    // storage
    octillion::Storage storage_;
public:
    int degree_x_, degree_y_, degree_z_;
    double scale_;
    HANDLE hMyFont;
    BitmapRoller b_roller_;
    MazeBitmapCreator maze_bitmap_;
    TextBitmapCreatorRev text_bitmap_rev_;
    ListBitmapCreator list_bitmap_;

private:
    template <class T>
    static void reset_matrix(Matrix<T>& matrix)
    {
        T data[] = { 
            (T)1, 0, 0, 0,
            0, (T)1, 0, 0,
            0, 0, (T)1, 0,
            0, 0, 0, (T)1 };

        matrix.set(4, 4, data);
    }

    template<class T>
    static void set_scale(Matrix<T>& matrix, T scale)
    {
        T data[16] = {
            scale, 0, 0, 0,
            0, scale, 0, 0,
            0, 0, scale, 0,
            0, 0, 0,     (T)1 };

        matrix.set(data);
    }

    template <class T>
    static void set_translate(Matrix<T>& matrix, T x, T y, T z)
    {
        double data[16] = {
            (T)1, 0, 0, 0,
            0, (T)1, 0, 0,
            0, 0, (T)1, 0,
            x, y, z, (T)1 };

        matrix.set(data);
    }

    static void set_rotateX(Matrix<double>& matrix, double radian)
    {
        double data[16] = {
            1, 0, 0, 0,
            0, cos(radian), sin(radian), 0,
            0, -sin(radian), cos(radian), 0,
            0, 0, 0, 1 };

        matrix.set(data);
    }

    static void set_rotateY(Matrix<double>& matrix, double radian)
    {
        double data[16] = {
            cos(radian), 0, -sin(radian), 0,
            0, 1, 0, 0,
            sin(radian), 0, cos(radian), 0,
            0, 0, 0, 1 };

        matrix.set(data);
    }

    static void set_rotateZ(Matrix<double>& matrix, double radian)
    {
        double data[16] = {
            cos(radian), sin(radian), 0, 0,
            -sin(radian), cos(radian), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1 };

        matrix.set(data);
    }
};


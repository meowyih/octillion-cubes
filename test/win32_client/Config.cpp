#include "Config.hpp"

void Config::init(int screen_width, int screen_height)
{
    w_ = screen_width;
    h_ = screen_height;

    // title bar
    title_height_ = screen_height / 20;
    title_text_height_ = title_height_ - 2;
    area_name_width_ = screen_width * 2 / 5;
    cube_name_width_ = screen_width * 2 / 5;
    cube_status_width_ = screen_width / 5;

    // console
    console_window_width_ = screen_width / 3;
    console_window_height_ = screen_height * 2 / 3;
    console_window_pos_x_ = (w_ - console_window_width_) / 2;
    console_window_pos_y_ = 0;
    console_text_height_ = screen_height / 15;

    // set flag
    init_ = true;
}

Config::Config()
{
    init_ = false;
}

Config::~Config()
{
}

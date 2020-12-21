
#include <vector>

#include "error/macrolog.hpp"

#include "Config.hpp"
#include "BitmapRoller.hpp"

BitmapRoller::BitmapRoller()
{
}

void BitmapRoller::set_canvas(int screenwidth, int screenheight)
{
    if (w_ == screenwidth && h_ == screenheight)
    {
        // no need to reset buffer
        return;
    }

;   w_ = screenwidth;
    h_ = screenheight;
    buffer_.resize(w_ * h_ * Config::get_instance().pixel_size());
    memset((void*)buffer_.data(), 0, buffer_.size());
}

void BitmapRoller::add(std::vector<BYTE>& data, int w, int h, int align)
{
    int x, y;
    std::shared_ptr<std::vector<BYTE>> out 
        = std::make_shared<std::vector<BYTE>>();

    out->assign(data.begin(), data.end());

    last_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    // int x, int y, int w, int h, std::shared_ptr<std::vector<BYTE>> data, int align = BitmapParagraph::CENTER
    if (align == BitmapRoller::CENTER)
    {
        x = (w_ - w) / 2;
    }
    else
    {
        x = 0;
    }

    if (bmp_list_.size() == 0)
    {
        y = -1 * h;
    }
    else
    {
        y = bmp_list_.back().y_ - h;
    }

    bmp_list_.push_back(BitmapParagraph(x, y, w, h, out ));
}

void BitmapRoller::render(std::vector<BYTE>& out)
{
    std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    int inc_y = (int)((now - last_).count()) / ms_per_line_;

    out.clear();

    // duration is not enough to move
    if (inc_y == 0)
    {
        out.assign(buffer_.begin(), buffer_.end());
        return;
    }

    // no data
    if (bmp_list_.size() == 0)
    {
        out.assign(buffer_.begin(), buffer_.end());
        return;
    }

    // roll out to the end
    if (bmp_list_.back().y_ >= 0 )
    {
        out.assign(buffer_.begin(), buffer_.end());
        return;
    }

    // increase the y position to all bitmap
    for (auto it = bmp_list_.begin(); it != bmp_list_.end(); it++)
    {
        (*it).y_ += inc_y;
    }

    // remove the bitmap that not viewable
    while (bmp_list_.size() > 0)
    {
        auto it = bmp_list_.begin();
        if ((*it).y_ > h_)
        {
            bmp_list_.erase(it);
        }
        else
        {
            break;
        }
    }

    // redraw the buffer
    memset(buffer_.data(), 0, buffer_.size());

    for (auto it = bmp_list_.begin(); it != bmp_list_.end(); it++)
    {
        draw(buffer_, w_, h_, *((*it).data_), (*it).w_, (*it).h_, (*it).x_, (*it).y_);
    }

    // post sfx, reduce the color by height
    for (int i = 0; i < h_; i++)
    {
        double ratio = 1 - i * 1.0 / h_;
        for (int j = 0; j < w_; j++)
        {
            int anchor = (i * w_ + j) * Config::get_instance().pixel_size();
            buffer_.at(anchor) = (BYTE)(buffer_.at(anchor) * ratio);
            buffer_.at(anchor+1) = (BYTE)(buffer_.at(anchor+1) * ratio);
            buffer_.at(anchor+2) = (BYTE)(buffer_.at(anchor+2) * ratio);
        }
    }

    out.assign(buffer_.begin(), buffer_.end());
    return;
}

void BitmapRoller::set_roll_speed(int ms_per_line)
{
    ms_per_line_ = ms_per_line;
}

int BitmapRoller::width()
{
    return w_;
}

int BitmapRoller::height()
{
    return h_;
}

void BitmapRoller::draw(
    std::vector<BYTE>& data1, int w1, int h1, 
    std::vector<BYTE>& data2,  int w2, int h2, int x, int y)
{
    int outside_left, outside_right;
    int pixel_per_line;
    int bottom_line, top_line;
    int byte_per_line;

    outside_left = x < 0 ? -1 * x : 0;
    outside_right = x + w2 > w1 ? x + w2 - w1 : 0;
    pixel_per_line = w2 - outside_right - outside_left;
    byte_per_line = pixel_per_line * Config::get_instance().pixel_size();

    bottom_line = y < 0 ? -1 * y : 0;
    top_line = y + h2 > h1 ? h1 - y : h2;

    for (int i = bottom_line; i < top_line; i++)
    {
        // anchor is the start point for each line
        int anchor1 =
            (i - bottom_line + ( y > 0 ? y : 0 )) * w1 *Config::get_instance().pixel_size() +
            x * Config::get_instance().pixel_size();
        int anchor2 = 
            i * w2 * Config::get_instance().pixel_size() + 
            outside_left * Config::get_instance().pixel_size();

        memcpy(data1.data() + anchor1, data2.data() + anchor2, byte_per_line);
    }
}

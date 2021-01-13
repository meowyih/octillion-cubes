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

void BitmapRoller::add(std::vector<BYTE>& data, int w, int h, int strid, int align)
{
    is_rolling_ = true;
    add_top(data, w, h, strid, align);
}

void BitmapRoller::add_bottom(std::vector<BYTE>& data, int w, int h, int strid, int align)
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

    bmp_list_.push_back(BitmapParagraph(x, y, w, h, strid, group_idx_, out));
}

void BitmapRoller::add_top(std::vector<BYTE>& data, int w, int h, int strid, int align)
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
        y = h_ + h;
    }
    else
    {
        y = bmp_list_.back().y_ + bmp_list_.back().h_;
    }

    // insert the bitmap
    bmp_list_.push_back(BitmapParagraph(x, y, w, h, strid, group_idx_, out));
}

void BitmapRoller::render(std::vector<BYTE>& out)
{
    render_top(out);
}

void BitmapRoller::render_bottom(std::vector<BYTE>& out)
{
    int last_y1, last_y2;

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
        group_idx_++;
        out.assign(buffer_.begin(), buffer_.end());
        is_rolling_ = false;
        return;
    }

    // reach the end
    if (bmp_list_.back().y_ >= 0 )
    {
        group_idx_++;
        out.assign(buffer_.begin(), buffer_.end());
        is_rolling_ = false;
        return;
    }

    // increase the y position to all bitmap
    for (auto it = bmp_list_.begin(); it != bmp_list_.end(); it++)
    {
        (*it).y_ += inc_y;
    }

    // if we add too many space in y dir, we need to pull extra height
    if (bmp_list_.back().y_ > 0)
    {
        int y_gap = bmp_list_.back().y_;

        for (auto it = bmp_list_.begin(); it != bmp_list_.end(); it++)
        {
            (*it).y_ -= y_gap;
        }
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

    // draw boundary at the last
    get_height_bottom(last_y1, last_y2);

    last_y1 = last_y1 < 0 ? 0 : last_y1;
    last_y1 = last_y1 >= h_ ? h_-1 : last_y1;

    last_y2 = last_y2 < 0 ? 0 : last_y2;
    last_y2 = last_y2 >= h_ ? h_ - 1 : last_y2;

    for (int i = last_y1; i <= last_y2; i++)
    {
        BYTE* anchor = buffer_.data();
        anchor = anchor + i * w_ * Config::get_instance().pixel_size() + 2;
        *anchor = 0xFF;
        anchor = anchor + (w_ - 1) * Config::get_instance().pixel_size();
        *anchor = 0xFF;
    }

    for (int i = 0; i < w_; i++)
    {
        BYTE* anchor = buffer_.data();
        anchor = anchor + i * Config::get_instance().pixel_size() + 2;
        *anchor = 0xFF;
        anchor = anchor + last_y2 * (w_) * Config::get_instance().pixel_size();
        *anchor = 0xFF;
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

void BitmapRoller::render_top(std::vector<BYTE>& out)
{
    int last_y1, last_y2, diff_y1_y2;

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
        group_idx_++;
        out.assign(buffer_.begin(), buffer_.end());
        is_rolling_ = false;
        return;
    }

    // reach the top
    if (bmp_list_.back().y_ + bmp_list_.back().h_ <= h_)
    {
        group_idx_++;
        out.assign(buffer_.begin(), buffer_.end());
        is_rolling_ = false;
        return;
    }

    // decrease the y position to all bitmap
    for (auto it = bmp_list_.begin(); it != bmp_list_.end(); it++)
    {
        (*it).y_ -= inc_y;
    }

    // if we add too many space in y dir, we need to pull extra height
    if (bmp_list_.back().y_ + bmp_list_.back().h_ < h_)
    {
        int y_gap = h_ - (bmp_list_.back().y_ + bmp_list_.back().h_);

        for (auto it = bmp_list_.begin(); it != bmp_list_.end(); it++)
        {
            (*it).y_ += y_gap;
        }
    }

    // remove the bitmap that not viewable
    while (bmp_list_.size() > 0)
    {
        auto it = bmp_list_.begin();
        if ((*it).y_ < 0 )
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

    // draw boundary at the last
    get_height_top(last_y1, last_y2);

    last_y1 = last_y1 < 0 ? 0 : last_y1;
    last_y1 = last_y1 >= h_ ? h_ - 1 : last_y1;

    last_y2 = last_y2 < 0 ? 0 : last_y2;
    last_y2 = last_y2 >= h_ ? h_ - 1 : last_y2;

    diff_y1_y2 = last_y2 - last_y1;

    for (int i = last_y1; i <= last_y2; i++)
    {
        BYTE* anchor = buffer_.data();
        anchor = anchor + i * w_ * Config::get_instance().pixel_size() + 2;
        *anchor = 0xFF;
        anchor = anchor + (w_ - 1) * Config::get_instance().pixel_size();
        *anchor = 0xFF;
    }

    for (int i = 0; i < w_; i++)
    {
        BYTE* anchor = buffer_.data();

        anchor = anchor + ( last_y1 * (w_) + i ) * Config::get_instance().pixel_size() + 2;
        *anchor = 0xFF;
        anchor = anchor + diff_y1_y2 * w_ *Config::get_instance().pixel_size();
        *anchor = 0xFF;
    }
#if 0
    // post sfx, reduce the color by height
    for (int i = 0; i < h_; i++)
    {
        double ratio = 1 - i * 1.0 / h_;
        for (int j = 0; j < w_; j++)
        {
            int anchor = (i * w_ + j) * Config::get_instance().pixel_size();
            buffer_.at(anchor) = (BYTE)(buffer_.at(anchor) * ratio);
            buffer_.at(anchor + 1) = (BYTE)(buffer_.at(anchor + 1) * ratio);
            buffer_.at(anchor + 2) = (BYTE)(buffer_.at(anchor + 2) * ratio);
        }
    }
#endif
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

bool BitmapRoller::is_rolling()
{
    return is_rolling_;
}

bool BitmapRoller::get_height_bottom(int& y1, int& y2)
{
    // case 0: empty data
    if (bmp_list_.size() == 0)
    {
        // LOG_D(tag_) << "get_height case 0 ret false";
        return false;
    }
    
    auto it = bmp_list_.end();

    while (it != bmp_list_.begin())
    {
        it--;
        if ((*it).group_id_ == group_idx_)
            break;
    }

    // case 1: no last group_idx_ in the list
    if (it == bmp_list_.begin() && (*it).group_id_ != group_idx_)
    {
        // LOG_D(tag_) << "get_height case 1 ret false, (*it).group_id_:" << (*it).group_id_ << " group_idx_:" << group_idx_;
        return false;
    }

    // case 2: only first element contains strid
    if (it == bmp_list_.begin() && (*it).group_id_ == group_idx_)
    {
        y2 = (*it).y_ + (*it).h_;
        y1 = (*it).y_;
        // LOG_D(tag_) << "get_height case 2 group idx:" << group_idx_ << " y1:" << y1 << " y2:" << y2;
        return true;
    }

    // case 3: we found y2 and search for y1
    y2 = (*it).y_ + (*it).h_;
    y1 = (*it).y_;
    it--;

    while (it != bmp_list_.begin() && (*it).group_id_ == group_idx_)
    {
        y2 += (*it).h_;
        it--;
    }

    if (it == bmp_list_.begin() && (*it).group_id_ == group_idx_)
    {
        y2 += (*it).h_;
    }

    // LOG_D(tag_) << "get_height case 3 group idx:" << group_idx_ << " y1:" << y1 << " y2:" << y2;

    return true;
}

bool BitmapRoller::get_height_top(int& y1, int& y2)
{
    // case 0: empty data
    if (bmp_list_.size() == 0)
    {
        // LOG_D(tag_) << "get_height_top case 0 ret false";
        return false;
    }

    auto it = bmp_list_.end();

    while (it != bmp_list_.begin())
    {
        it--;
        if ((*it).group_id_ == group_idx_)
            break;
    }

    // case 1: no last group_idx_ in the list
    if (it == bmp_list_.begin() && (*it).group_id_ != group_idx_)
    {
        // LOG_D(tag_) << "get_height_top case 1 ret false, (*it).group_id_:" << (*it).group_id_ << " group_idx_:" << group_idx_;
        return false;
    }

    // case 2: only first element contains strid
    if (it == bmp_list_.begin() && (*it).group_id_ == group_idx_)
    {
        y2 = (*it).y_ + (*it).h_;
        y1 = (*it).y_;
        // LOG_D(tag_) << "get_height_top case 2 group idx:" << group_idx_ << " y1:" << y1 << " y2:" << y2;
        return true;
    }

    // case 3: we found y2 and search for y1
    y2 = (*it).y_ + (*it).h_;
    y1 = (*it).y_;
    it--;

    while (it != bmp_list_.begin() && (*it).group_id_ == group_idx_)
    {
        y1 -= (*it).h_;
        it--;
    }

    if (it == bmp_list_.begin() && (*it).group_id_ == group_idx_)
    {
        y1 -= (*it).h_;
    }

    // LOG_D(tag_) << "get_height_top case 3 group idx:" << group_idx_ << " y1:" << y1 << " y2:" << y2;

    return true;
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

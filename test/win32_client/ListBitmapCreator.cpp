#include "error/macrolog.hpp"

#include "ListBitmapCreator.hpp"
#include "TextBitmapCreator.hpp"
#include "Config.hpp"

ListBitmapCreator::ListBitmapCreator()
{
    is_new_data_ = false;
}

void ListBitmapCreator::set(int w, int h, std::vector<std::shared_ptr<octillion::Interactive>>& data)
{
    is_new_data_ = true;
    w_ = w;
    h_ = h;
    bitmap_ = std::make_shared<std::vector<BYTE>>();
    bitmap_->resize(w * h * Config::get_instance().pixel_size());
    data_.assign(data.begin(), data.end());
}

std::shared_ptr<std::vector<BYTE>> ListBitmapCreator::render(HDC hdc)
{
    TextBitmapCreator tmc;
    int text_height = Config::get_instance().interactive_text_height();
    int pixel_size = Config::get_instance().pixel_size();

    if (data_.size() == 0 || ! is_new_data_)
        return bitmap_;

    pos_.clear();

    for (size_t idx = 0; idx < data_.size(); idx ++ )
    {
        ListBitmapItem item;
        int pos_y = h_;
        int w = w_;
        int h = text_height;
        int strid = data_.at(idx)->id();
        bool ret;
        std::vector<BYTE> line;
        tmc.addLine(strid, data_.at(idx)->title()->wstr_.at(0), false, false);
        ret = tmc.getBitmapFixHeight(hdc, w, h, strid, line);

        if (ret == false)
        {
            LOG_E(tag_) << "render get bad bitmap from TextBitmapCreator::getBitmapFixHeight";
            continue;
        }

        pos_y = pos_y - h;

        if (pos_y < 0)
        {
            LOG_W(tag_) << " render detect total " << data_.size() << " items, but space can only contains " << (idx - 1);
            break;
        }

        item.x_ = 0;
        item.y_ = pos_y;
        item.w_ = w;
        item.h_ = h;
        item.interactive_ = data_.at(idx);
        pos_.push_back(item);

        // draw data
        for (int i = 0; i < h; i++)
        {
            BYTE* anchor1 = bitmap_->data() + ( i + pos_y ) * w_ * pixel_size;
            BYTE* anchor2 = line.data() + i * w * pixel_size;
            memcpy(anchor1, anchor2, w * pixel_size);
        }
    }

    is_new_data_ = false;
    return bitmap_;
}

std::shared_ptr<octillion::Interactive> ListBitmapCreator::point(int x, int y)
{
    LOG_D(tag_) << "point " << x << "," << y << " list contains:" << pos_.size();

    for (auto it = pos_.begin(); it != pos_.end(); it++)
    {
        LOG_D(tag_) << (*it).x_ << "," << (*it).y_ << " " << (*it).w_ << "," << (*it).h_;
        if (x >= (*it).x_ && x <= (*it).x_ + (*it).w_ &&
            y >= (*it).y_ && y <= (*it).y_ + (*it).h_)
        {
            return (*it).interactive_;
        }
    }

    return nullptr;
}

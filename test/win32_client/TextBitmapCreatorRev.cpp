#include "error/macrolog.hpp"
#include "TextBitmapCreatorRev.hpp"

TextBitmapCreatorRev::TextBitmapCreatorRev()
{
}

void TextBitmapCreatorRev::addLine(int strid, std::wstring wstr, bool head_margin, bool tail_margin)
{
    if (head_margin)
    {
        std::wstring blank = L"\n";
        strs_.push_back(std::make_pair(strid, blank));
    }
    strs_.push_back(std::make_pair(strid, wstr));
    if (tail_margin)
    {
        std::wstring blank = L"\n";
        strs_.push_back(std::make_pair(strid, blank));
    }
}

void TextBitmapCreatorRev::render(HDC hdc, int width, int height)
{
    // LOG_D(tag_) << "render start, strs_.size() " << strs_.size();
    int first_strid;

    while (strs_.size() > 0)
    {
        TextBitmap bm;
        int bm_w = width, bm_h = height, bm_strid;
        std::vector<BYTE> bm_data;
        getBitmapFixHeight(hdc, bm_w, bm_h, bm_strid, bm_data);

        bm.w_ = bm_w;
        bm.h_ = bm_h;
        bm.strid_ = bm_strid;
        bm.data_ = std::make_shared<std::vector<BYTE>>();
        bm.data_->assign(bm_data.begin(), bm_data.end());
        bm_list_.push_back(bm);
    }

    if (bm_list_.size() == 0)
    {
        // LOG_D(tag_) << "render end because no bitmap in bm_list_";
        return;
    }

    bm_list_it_ = bm_list_.begin();

    first_strid = bm_list_.begin()->strid_;

    // move to next element different str id
    while (bm_list_it_ != bm_list_.end())
    {
        if (bm_list_it_->strid_ != first_strid)
            break;
        bm_list_it_++;
    }

    // LOG_D(tag_) << "render end";
}

bool TextBitmapCreatorRev::getBitmap(int& width, int& height, int& strid, std::vector<BYTE>& out)
{
    // LOG_D(tag_) << "getBitmap start";

    if (bm_list_.size() == 0)
    {
        // LOG_D(tag_) << "getBitmap end due to no data in bm_list_";
        return false;
    }

    if (bm_list_it_ == bm_list_.begin())
    {
        int first_strid = bm_list_.begin()->strid_;

        // move to next element different str id
        while (bm_list_it_ != bm_list_.end())
        {
            if (bm_list_it_->strid_ != first_strid)
                break;
            bm_list_it_++;
        }
    }

    auto it = bm_list_it_;
    it--;
    
    width = it->w_;
    height = it->h_;
    strid = it->strid_;
    out.assign(it->data_->begin(), it->data_->end());

    bm_list_.erase(it);

    // LOG_D(tag_) << "getBitmap end, bm_list_ size:" << bm_list_.size() << " bitmap size:" << out.size();

    return true;
}

bool TextBitmapCreatorRev::isEmpty()
{
    return (bm_list_.size() == 0);
}

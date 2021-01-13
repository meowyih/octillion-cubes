#include "error/macrolog.hpp"
#include "TextBitmapCreator.hpp"
#include "Config.hpp"

TextBitmapCreator::TextBitmapCreator()
{
}

void TextBitmapCreator::addLine(int strid, std::wstring wstr, bool head_margin, bool tail_margin)
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

bool TextBitmapCreator::getBitmapFixHeight(HDC hdc, int& width, int& height, int& strid, std::vector<BYTE>& out)
{
    int    cHeight = height;
    int    cWidth = 0;
    int    cEscapement = 0;
    int    cOrientation = 0;
    SIZE wsize;
    std::wstring text;
    // std::wstring fontname = TEXT("·½¬u¶êÅé B");
    std::wstring fontname = Config::get_instance().console_fontname_wstr();

    if (strs_.size() == 0)
    {
        return FALSE;
    }

    text = strs_.front().second;
    strid = strs_.front().first;

    if (text.compare(L"\n") == 0)
    {
        // a blank new line, we only need half of the height
        cHeight = height / 2;
    }

    HDC vhdc = CreateCompatibleDC(hdc);
    HFONT hFont = CreateFont(
        cHeight, cWidth, cEscapement, cOrientation, FW_DONTCARE,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, VARIABLE_PITCH,
        fontname.c_str());

    // store the original font and change to new font
    HFONT hOldFont = (HFONT)SelectObject(vhdc, hFont);    
    SetTextColor(vhdc, RGB(0, 255, 0)); // text color
    SetBkMode(vhdc, TRANSPARENT);

    // wsize.cx / wsize.cy is text string's width and length
    do
    {
        GetTextExtentPoint(vhdc, text.c_str(), text.length(), &wsize);

        if (wsize.cx > width)
        {
            text.pop_back();

            if (text.length() == 0)
            {
                // switch font back and release the created resource
                SelectObject(vhdc, hOldFont);
                DeleteObject(hFont);
                DeleteObject(vhdc);
                return false;
            }
        }
        else
        {
            break;
        }
    } while (text.length() > 0);

    // draw to vhdc
    width = wsize.cx;
    height = wsize.cy;

    HBITMAP hbmp = CreateCompatibleBitmap(hdc, width, height);
    BITMAPINFO bmpi = {
        {sizeof(BITMAPINFOHEADER),width,height,1,32,BI_RGB,0,0,0,0,0},
        {0,0,0,0}
    };
    SelectObject(vhdc, hbmp);

    // allocate the bitmap
    out.resize(height * width * 4); // 32 bits pixel
    
    // draw bitmap
    TextOut(vhdc, 0, 0, text.c_str(), text.length());
    GetDIBits(vhdc, hbmp, 0, height, out.data(), &bmpi, BI_RGB);
    DeleteObject(hbmp);

    // switch font back and release the created resource
    SelectObject(vhdc, hOldFont);
    DeleteObject(hFont);
    DeleteObject(vhdc);

    // remove the string
    if (text.length() != strs_.front().second.length())
    {
        strs_.front().second = strs_.front().second.substr(text.length());
    }
    else
    {
        strs_.erase(strs_.begin());
    }

    return true;
}

bool TextBitmapCreator::getBitmapFixHeight(HDC hdc, int& width, int& height, std::vector<Point3d>& out, wchar_t wchar)
{
    int    padding_x, padding_y;
    int    cHeight = height;
    int    cWidth = 0;
    int    cEscapement = 0;
    int    cOrientation = 0;
    std::vector<BYTE> bitmap;
    SIZE wsize;
    std::wstring text;
    std::wstring fontname = Config::get_instance().symbol_fontname_wstr();
    text += wchar;

    HDC vhdc = CreateCompatibleDC(hdc);
    HFONT hFont = CreateFont(
        cHeight, cWidth, cEscapement, cOrientation, FW_DONTCARE,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, VARIABLE_PITCH,
        fontname.c_str());

    // store the original font and change to new font
    HFONT hOldFont = (HFONT)SelectObject(vhdc, hFont);

    SetTextColor(vhdc, RGB(255, 255, 255)); // text color
    SetBkMode(vhdc, TRANSPARENT);

    GetTextExtentPoint(vhdc, text.c_str(), text.length(), &wsize);

    if (wsize.cx > height || wsize.cy > height)
    {
        LOG_E("TextBitmapCreator") << "Cube3d::getBitmapFixHeight size overflow, max: " << height << " w,h:" << wsize.cx << "," << wsize.cy;
        return false;
    }

    // get offset
    padding_x = (width - wsize.cx) / 2;
    padding_y = (height - wsize.cy) / 2;

    // draw to vhdc
    width = wsize.cx;
    height = wsize.cy;

    HBITMAP hbmp = CreateCompatibleBitmap(hdc, width, height);
    BITMAPINFO bmpi = {
        {sizeof(BITMAPINFOHEADER),width,height,1,32,BI_RGB,0,0,0,0,0},
        {0,0,0,0}
    };
    SelectObject(vhdc, hbmp);

    // allocate the bitmap
    bitmap.resize(width * height * 4); // 32 bits pixel
    out.reserve(width * height / 3); // estimate the average pixel for one char

    // draw bitmap
    TextOut(vhdc, 0, 0, text.c_str(), text.length());
    GetDIBits(vhdc, hbmp, 0, height, bitmap.data(), &bmpi, BI_RGB);
    DeleteObject(hbmp);

    // switch font back and release the created resource
    SelectObject(vhdc, hOldFont);
    DeleteObject(hFont);
    DeleteObject(vhdc);

    // add offset to each point
    // LOG_D(tag_) << "char w:" << wsize.cx << " h:" << wsize.cy;
    BYTE* rawbitmap = bitmap.data();
    int total_bitmap_pixel = width * height;
    for (int idx = 0; idx < total_bitmap_pixel * 4; idx = idx + 4)
    {
        if ((int)(rawbitmap[idx]) != 0)
        {
            Point3d pt;
            pt.x_ = (idx / 4) % width + padding_x;
            pt.y_ = (idx / 4) / width + padding_y;
            // LOG_D(tag_) << idx << "->x,y: " << ((idx / 4) % width) << "," << ((idx / 4) / width);
            // LOG_D(tag_) << "pt x,y: " << pt.x_ << "," << pt.y_;
            out.push_back(pt);
        }
    }

    return true;
}

bool TextBitmapCreator::isEmpty()
{
    return ( strs_.size() == 0 );
}

void TextBitmapCreator::reset()
{
    strs_.clear();
}

#include "error/macrolog.hpp"
#include "TextBitmapCreator.hpp"

TextBitmapCreator::TextBitmapCreator()
{
}

void TextBitmapCreator::addLine(std::wstring wstr)
{
    strs_.push_back(wstr);
}

bool TextBitmapCreator::getBitmapFixHeight(HDC hdc, int& width, int& height, std::vector<BYTE>& out)
{
    int    cHeight = height;
    int    cWidth = 0;
    int    cEscapement = 0;
    int    cOrientation = 0;
    SIZE wsize;
    std::wstring text;
    std::wstring fontname = TEXT("·½¬u¶êÅé B");

    if (strs_.size() == 0)
    {
        return FALSE;
    }

    text = strs_.front();

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
    if (text.length() != strs_.front().length())
    {
        strs_.front() = strs_.front().substr(text.length());
    }
    else
    {
        strs_.erase(strs_.begin());
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

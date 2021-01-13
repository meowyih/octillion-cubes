#ifndef TEXTBITMAPCREATOR_HEADER
#define TEXTBITMAPCREATOR_HEADER

#include <string>
#include <list>
#include <vector>
#include <memory>

#include "Point3d.hpp"

class TextBitmapCreator
{
private:
    const std::string tag_ = "TextBitmapCreator";

public:
    TextBitmapCreator();

    virtual void addLine(int strid, std::wstring wstr, bool head_margin, bool tail_margin);

    // get a bitmap of text size the fixed height, which does NOT include
    // internal-leading value, check return height for the actual bitmap height.
    // If text is longer than width, class will cut it and put the remaining
    // text into memory for the next call,
    // [in, out] width - input max width and return actual
    // [in, out] height - input max height and return actual
    // [out] out - bitmap
    bool getBitmapFixHeight(HDC hdc, int& width, int& height, int& strid, std::vector<BYTE>& out );

    // similar as original version but is a static 
    // stateless function for single character output.
    // Output type is a sequence of point position instead of RGB bytes.
    // [in] wchar - single character
    // [in] offset_x, offset_y - offset for each point
    static bool getBitmapFixHeight(
        HDC hdc, int& width, int& height,
        std::vector<Point3d>& out,
        wchar_t wchar);

    virtual bool isEmpty();

    void reset();

protected:
    std::list<std::pair<int,std::wstring>> strs_;


};
#endif // TEXTBITMAPCREATOR_HEADER

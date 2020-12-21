#ifndef TEXTBITMAPCREATOR_HEADER
#define TEXTBITMAPCREATOR_HEADER

#include <string>
#include <list>
#include <vector>

class TextBitmapCreator
{
public:
    TextBitmapCreator();

    void addLine(std::wstring wstr);

    // get a bitmap of text size the fixed height, which does NOT include
    // internal-leading value, check return height for the actual bitmap height.
    // If text is longer than width, class will cut it and put the remaining
    // text into memory for the next call,
    // [in, out] width - input max width and return actual
    // [in, out] height - input max height and return actual
    // [out] out - bitmap
    bool getBitmapFixHeight(HDC hdc, int& width, int& height, std::vector<BYTE>& out );

    bool isEmpty();

    void reset();

private:
    std::list<std::wstring> strs_;
};
#endif // TEXTBITMAPCREATOR_HEADER

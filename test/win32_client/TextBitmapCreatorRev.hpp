#ifndef TEXTBITMAPCREATERREV_HEADER
#define TEXTBITMAPCREATERREV_HEADER

#include <memory>
#include <vector>
#include <list>

#include "TextBitmapCreator.hpp"

class TextBitmapCreatorRev : public TextBitmapCreator
{
private:
    const std::string tag_ = "TextBitmapCreatorRev";

public:
    TextBitmapCreatorRev();

    void addLine(int strid, std::wstring wstr, bool head_margin, bool tail_margin) override;

    void render(HDC hdc, int width, int height);

    bool getBitmap(int& width, int& height, int& strid, std::vector<BYTE>& out);

    bool isEmpty() override;

public:
    class TextBitmap
    {
    public:
        TextBitmap()
        {
        }
    public:
        int w_ = 0;
        int h_ = 0;
        int strid_ = 0;
        std::shared_ptr<std::vector<BYTE>> data_;
    };

private:
    std::list<TextBitmap> bm_list_;
    std::list<TextBitmap>::iterator bm_list_it_;
};

#endif

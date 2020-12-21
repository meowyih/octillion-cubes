#ifndef TEXTBITMAPROLLOER
#define TEXTBITMAPROLLOER

#include <list>
#include <vector>
#include <chrono>
#include <memory>

class BitmapParagraph
{
public:
    BitmapParagraph(int x, int y, int w, int h, std::shared_ptr<std::vector<BYTE>> data) :
        x_(x), y_(y), w_(w), h_(h), data_(data) {}
public:
    int x_, y_, w_, h_;
    std::shared_ptr<std::vector<BYTE>> data_;
};

class BitmapRoller
{
public:
    const static int CENTER = 1;
    const static int LEFT = 2;

public:
    BitmapRoller();

    void set_canvas(int screenwidth, int screenheight);
    void add(std::vector<BYTE>& data, int w, int h, int align );
    void render(std::vector<BYTE>& out);
    void set_roll_speed(int ms_per_line);
    int width();
    int height();

private:
    void draw(std::vector<BYTE>& data1, int w1, int h1, 
        std::vector<BYTE>& data2, int w2, int h2, int x, int y );

private:
    std::vector<BYTE> buffer_;
    std::list<BitmapParagraph> bmp_list_;
    std::chrono::milliseconds last_;   
    int w_ = 0;
    int h_ = 0;
    int ms_per_line_ = 10;
};
#endif
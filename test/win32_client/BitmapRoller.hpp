#ifndef TEXTBITMAPROLLOER
#define TEXTBITMAPROLLOER

#include <list>
#include <vector>
#include <chrono>
#include <memory>
#include <string>

class BitmapParagraph
{
private:
    const std::string tag_ = "BitmapParagraph";

public:
    BitmapParagraph(int x, int y, int w, int h, int strid, int group_id, std::shared_ptr<std::vector<BYTE>> data) :
        x_(x), y_(y), w_(w), h_(h), strid_(strid), group_id_(group_id), data_(data) {}
public:
    int strid_;
    int group_id_;
    int x_, y_, w_, h_;
    std::shared_ptr<std::vector<BYTE>> data_;
};

class BitmapRoller
{
private:
    const std::string tag_ = "BitmapRoller";

public:
    const static int CENTER = 1;
    const static int LEFT = 2;

public:
    BitmapRoller();

    void set_canvas(int screenwidth, int screenheight);
    void add(std::vector<BYTE>& data, int w, int h, int strid, int align );
    void render(std::vector<BYTE>& out);
    void set_roll_speed(int ms_per_line);
    int width();
    int height();

    bool is_rolling();

private:
    bool get_height_bottom(int& y1, int& y2);
    bool get_height_top(int& y1, int& y2);

private:
    void add_top(std::vector<BYTE>& data, int w, int h, int strid, int align);
    void add_bottom(std::vector<BYTE>& data, int w, int h, int strid, int align);

    void render_top(std::vector<BYTE>& out);
    void render_bottom(std::vector<BYTE>& out);

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
    int group_idx_ = 0;
    bool is_rolling_ = false;
};
#endif
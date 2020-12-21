#include "DrawClass.hpp"

#include "framework.h"
#include "strsafe.h"
#include "drawing.h"

#include <memory>
#include <vector>

#include "error/ocerror.hpp"
#include "error/macrolog.hpp"

#include "Cube3d.hpp"
#include "TextBitmapCreator.hpp"
#include "Config.hpp"

#define M_PI 3.141659

DrawClass::DrawClass()
{
    buffer_ = nullptr;
    map_ = std::make_shared<octillion::WorldMap>();
    bool result = map_->load_external_data_file("./data/");
    degree_x_ = degree_y_ = degree_z_ = 0;

    cube_center_ = map_->reborn_;
    scale_ = 5;

    // load font
    HANDLE hMyFont = INVALID_HANDLE_VALUE; // Here, we will (hopefully) get our font handle
    HINSTANCE hInstance = ::GetModuleHandle(nullptr); // Or could even be a DLL's HINSTANCE
    HRSRC  hFntRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_BINARY1), L"Binary");
    if (hFntRes) { // If we have found the resource ... 
        HGLOBAL hFntMem = LoadResource(hInstance, hFntRes); // Load it
        if (hFntMem != nullptr) 
        {
            void* FntData = LockResource(hFntMem); // Lock it into accessible memory
            DWORD nFonts = 0, len = SizeofResource(hInstance, hFntRes);
            // you can use GenSenRounded JP B after this!
            hMyFont = AddFontMemResourceEx(FntData, len, nullptr, &nFonts); // Fake install font!
        }
    }
}

DrawClass::~DrawClass()
{
    RemoveFontMemResourceEx(hMyFont);
}

BOOL DrawClass::OnPaint(HWND hwnd, HDC hdc, BOOL recalc)
{    
    Matrix<double>  scale_matrix(4, 4);
    Matrix<double>  translate_matrix(4, 4), translate_matrix2(4, 4);
    Matrix<double>  rotate_matrixX(4, 4);
    Matrix<double>  rotate_matrixY(4, 4);
    Matrix<double>  rotate_matrixZ(4, 4);

    // recalculate the window size and allocate a new buffer
    if (recalc)
    {
        BITMAP structBitmapHeader;
        memset(&structBitmapHeader, 0, sizeof(BITMAP));

        HGDIOBJ hBitmap = GetCurrentObject(hdc, OBJ_BITMAP);
        GetObject(hBitmap, sizeof(BITMAP), &structBitmapHeader);

        // window's width and height, NOTE: include window title
        width_ = structBitmapHeader.bmWidth;
        height_ = structBitmapHeader.bmHeight;
        widthBytes_ = structBitmapHeader.bmWidthBytes;

        // bits per pixel, it should be 24 (3 bytes) or 32 (4 bytes)
        bytesPerPixel_ = structBitmapHeader.bmBitsPixel / 8;

        // allocate pixel buffer for drawing
        // Warning! widthBytes might not equal to width * bytesPerPixel
        // widthBytes MUST be even number by adding padding if needed 
        // i.e. bitmap should be WORD (16bits) aligned
        LONG bufferSize = widthBytes_ * height_;

        buffer_ = std::make_shared<std::vector<BYTE>>(bufferSize);

        // get real client window size (without window title)
        RECT wrect;
        GetWindowRect(hwnd, &wrect);

        RECT crect;
        GetClientRect(hwnd, &crect);

        POINT lefttop = { crect.left, crect.top }; // Practicaly both are 0
        ClientToScreen(hwnd, &lefttop);
        POINT rightbottom = { crect.right, crect.bottom };
        ClientToScreen(hwnd, &rightbottom);

        left_border_ = lefttop.x - wrect.left; // Windows 10: includes transparent part
        right_border_ = wrect.right - rightbottom.x; // As above
        bottom_border_ = wrect.bottom - rightbottom.y; // As above
        top_border_ = lefttop.y - wrect.top; // There is no transparent part

        // calculate the screen UI metric
        Config::get_instance().init(width(), height());

        // set bitmap roller
        b_roller_.set_canvas(
            Config::get_instance().console_window_width(),
            Config::get_instance().console_window_height());

        maze_bitmap_.set(
            width(), height(), map_
        );
    }

    // directly draw on the buffer
    for (int i = 0; i < width(); i++)
    {
        for (int j = 0; j < height(); j++)
        {
            setColor(i, j, 0);
        }
    }

    // draw a maximam rectangle for test 
    for (int i = 0; i < width(); i++)
    {
        setColor(i, 0, 0xFF0000);
        setColor(i, height() - 1, 0xFF0000);
    }

    for (int i = 0; i < height(); i++)
    {
        setColor(0, i, 0xFF0000);
        setColor(width() - 1, i, 0xFF0000);
    }

    // draw cube
    set_rotateX(rotate_matrixX, (degree_x_ % 360) * 2 * M_PI / 360);
    set_rotateY(rotate_matrixY, (degree_y_ % 360) * 2 * M_PI / 360);
    set_rotateZ(rotate_matrixZ, (degree_z_ % 360) * 2 * M_PI / 360);
    set_scale(scale_matrix, scale_);
    set_translate(translate_matrix, 
        -1 * maze_bitmap_.cube(cube_center_->loc())->p[0].x_,
        -1 * maze_bitmap_.cube(cube_center_->loc())->p[0].y_,
        -1 * maze_bitmap_.cube(cube_center_->loc())->p[0].z_);
    set_translate(translate_matrix2, width_ / 2 - 5*scale_, height_ / 2, 0);

    // final matrix, be careful of the order
    Matrix<double> world_matrix =
        translate_matrix *
        rotate_matrixX *
        rotate_matrixY * rotate_matrixZ *
        scale_matrix *
        translate_matrix2;

    // initialize the waiting animation
    if (animation_blocking_.size() > 0
        && animation_blocking_.front().status_ == TaskAnimation::ANI_WAITING)
    {
        // start running front animation
        animation_blocking_.front().status_ = TaskAnimation::ANI_RUNNING;
        animation_blocking_.front().ms_start_ = 
            std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());

        if (animation_blocking_.front().type_ == TaskAnimation::MOVE_2D)
        {
            std::vector<Point3d> out1, out2;
            std::shared_ptr<Cube3d> cube3d_start, cube3d_dest;
            Point3d pt_start, pt_to;
            cube3d_start = maze_bitmap_.cube(cube_center_->loc());
            cube3d_dest = maze_bitmap_.cube(animation_blocking_.front().cube_dest_->loc());

            pt_start = cube3d_start->render_pt(world_matrix, 0);
            pt_to = cube3d_dest->render_pt(world_matrix, 0);

            animation_blocking_.front().interval_x_ = (int)(pt_start.x_ - pt_to.x_);
            animation_blocking_.front().interval_y_ = (int)(pt_start.y_ - pt_to.y_);
        }
        else if (animation_blocking_.front().type_ == TaskAnimation::ROTATE)
        {
            animation_blocking_.front().start_degree_x_ = degree_x_;
            animation_blocking_.front().start_degree_y_ = degree_y_;
        }
    }

    if (animation_blocking_.size() > 0 
        && animation_blocking_.front().type_ == TaskAnimation::ROTATE )
    {
        std::chrono::milliseconds now = 
            std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        std::chrono::milliseconds duration =
            now - animation_blocking_.front().ms_start_;

        if (duration.count() >= animation_blocking_.front().duration_)
        {
            // re-calculate the world matrix
            degree_x_ = animation_blocking_.front().dest_degree_x_;
            degree_y_ = animation_blocking_.front().dest_degree_y_;

            set_rotateX(rotate_matrixX, (degree_x_ % 360) * 2 * M_PI / 360);
            set_rotateY(rotate_matrixY, (degree_y_ % 360) * 2 * M_PI / 360);
            world_matrix =
                translate_matrix *
                rotate_matrixX *
                rotate_matrixY * rotate_matrixZ *
                scale_matrix *
                translate_matrix2;
            animation_blocking_.pop_front();
        }
        else
        {
            double num = duration.count() * 1.0;
            double dem = animation_blocking_.front().duration_ * 1.0;
            double ratio = num  / dem;
            degree_x_ = animation_blocking_.front().start_degree_x_ + (int)(ratio * ( animation_blocking_.front().dest_degree_x_ - animation_blocking_.front().start_degree_x_));
            degree_y_ = animation_blocking_.front().start_degree_y_ + (int)(ratio * (animation_blocking_.front().dest_degree_y_ - animation_blocking_.front().start_degree_y_));
            set_rotateX(rotate_matrixX, (degree_x_ % 360) * 2 * M_PI / 360);
            set_rotateY(rotate_matrixY, (degree_y_ % 360) * 2 * M_PI / 360);
            world_matrix =
                translate_matrix *
                rotate_matrixX *
                rotate_matrixY * rotate_matrixZ *
                scale_matrix *
                translate_matrix2;
        }
    }
    else if (animation_blocking_.size() > 0
        && animation_blocking_.front().type_ == TaskAnimation::MOVE_2D)
    {
        std::chrono::milliseconds now =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());
        std::chrono::milliseconds duration =
            now - animation_blocking_.front().ms_start_;

        if (duration.count() >= animation_blocking_.front().duration_)
        {
            Matrix<double>  trans_ani(4, 4);
            cube_center_ = animation_blocking_.front().cube_dest_;
            set_translate(trans_ani,
                animation_blocking_.front().interval_x_,
                animation_blocking_.front().interval_y_,
                0);
            world_matrix = world_matrix * trans_ani;
            animation_blocking_.pop_front();
        }
        else
        {
            Matrix<double>  trans_ani(4, 4);

            double num = duration.count() * 1.0;
            double dem = animation_blocking_.front().duration_ * 1.0;
            double ratio = num / dem;

            set_translate(trans_ani,
                animation_blocking_.front().interval_x_ * ratio,
                animation_blocking_.front().interval_y_ * ratio,
                0);
            world_matrix = world_matrix * trans_ani;
        }
    }

    // draw bitmap
    if (degree_x_ == degree_y_ == degree_z_ == 0)
    {
        draw_map(hdc, cube_center_->loc(), world_matrix, MazeBitmapCreator::RENDER_FLAG_PLAIN);
    }
    else
    {
        draw_map(hdc, cube_center_->loc(), world_matrix, MazeBitmapCreator::RENDER_FLAG_ALL);
    }

    // draw title bar
    draw_title( hdc );

    // add text
    draw_console(hdc);

    // update the screen with buffer
    BITMAPINFOHEADER bih;
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biBitCount = bytesPerPixel_ * 8;
    bih.biClrImportant = 0;
    bih.biClrUsed = 0;
    bih.biCompression = BI_RGB;
    bih.biWidth = width_;
    bih.biHeight = height_;
    bih.biPlanes = 1;
    bih.biSizeImage = widthBytes_ * height_;

    BITMAPINFO bi;
    bi.bmiHeader = bih;
    SetDIBitsToDevice(
        hdc, 0, 0,
        width_, height_,
        0, 0, 0, height_,
        buffer_->data(), &bi, DIB_RGB_COLORS);

    return TRUE;
}

BOOL DrawClass::keypress(int keycode)
{
    BOOL isConsumed = FALSE;
    TaskAnimation task;
    std::shared_ptr<octillion::Cube> cube_dest = nullptr;
    bool add_rotate_animation = false;

    // all blocking animation need to be done before accept another input
    if (animation_blocking_.size() > 0)
    {
        return FALSE;
    }

    if (keycode == VK_RIGHT && cube_center_->exits_[octillion::Cube::X_INC] > 0 )
    {
        auto it = cube_center_->find(map_->cubes_, octillion::Cube::X_INC);
        if (it == nullptr)
            return TRUE;
        auto it2 = map_->cubes_.find((*it).loc());
        if (it2 == map_->cubes_.end())
            return TRUE;
        cube_dest = (*it2).second;
        isConsumed = TRUE;
    }
    else if (keycode == VK_LEFT && cube_center_->exits_[octillion::Cube::X_DEC] > 0)
    {
        auto it = cube_center_->find(map_->cubes_, octillion::Cube::X_DEC);
        if (it == nullptr)
            return TRUE;
        auto it2 = map_->cubes_.find((*it).loc());
        if (it2 == map_->cubes_.end())
            return TRUE;
        cube_dest = (*it2).second;
        isConsumed = TRUE;
    }
    else if (keycode == VK_UP && cube_center_->exits_[octillion::Cube::Y_INC] > 0)
    {
        auto it = cube_center_->find(map_->cubes_, octillion::Cube::Y_INC);
        if (it == nullptr)
            return TRUE;
        auto it2 = map_->cubes_.find((*it).loc());
        if (it2 == map_->cubes_.end())
            return TRUE;
        cube_dest = (*it2).second;
        isConsumed = TRUE;
    }
    else if (keycode == VK_DOWN && cube_center_->exits_[octillion::Cube::Y_DEC] > 0)
    {
        auto it = cube_center_->find(map_->cubes_, octillion::Cube::Y_DEC);
        if (it == nullptr)
            return TRUE;
        auto it2 = map_->cubes_.find((*it).loc());
        if (it2 == map_->cubes_.end())
            return TRUE;
        cube_dest = (*it2).second;
        isConsumed = TRUE;
    }
    else if (keycode == 0x55 && cube_center_->exits_[octillion::Cube::Z_INC] > 0) // 'u'
    {
        add_rotate_animation = true;

        auto it = cube_center_->find(map_->cubes_, octillion::Cube::Z_INC);
        if (it == nullptr)
            return TRUE;
        auto it2 = map_->cubes_.find((*it).loc());
        if (it2 == map_->cubes_.end())
            return TRUE;
        cube_dest = (*it2).second;
        isConsumed = TRUE;
    }
    else if (keycode == 0x44 && cube_center_->exits_[octillion::Cube::Z_DEC] > 0)// 'd'
    {
        add_rotate_animation = true;

        auto it = cube_center_->find(map_->cubes_, octillion::Cube::Z_DEC);
        if (it == nullptr)
            return TRUE;
        auto it2 = map_->cubes_.find((*it).loc());
        if (it2 == map_->cubes_.end())
            return TRUE;
        cube_dest = (*it2).second;
        isConsumed = TRUE;
    }

    if (isConsumed == FALSE)
    {
        return FALSE;
    }
    
    if (cube_dest == nullptr)
    {
        return TRUE;
    }

    // add text into console
    text_bitmap_.addLine(cube_dest->wtitle());

    // add animation
    task.type_ = TaskAnimation::MOVE_2D;
    task.duration_ = 200;    
    task.cube_dest_ = cube_dest;

    LOG_D("DrawClass") << "keypress cube_dest:" << task.cube_dest_->loc().str();

    // add animation into list
    animation_blocking_.push_back(task);

    if (add_rotate_animation)
    {
        TaskAnimation task_rotate, task_reset;

        // add rotate animation at begin
        task_rotate.type_ = TaskAnimation::ROTATE;
        task_rotate.dest_degree_x_ = -100;
        task_rotate.dest_degree_y_ = 20;
        task_rotate.duration_ = 300;
        animation_blocking_.insert(animation_blocking_.begin(), task_rotate);
        
        task_reset.type_ = TaskAnimation::ROTATE;
        task_rotate.dest_degree_x_ = 0;
        task_rotate.dest_degree_y_ = 0;
        task_rotate.duration_ = 200;
        animation_blocking_.push_back(task_rotate);
    }

    return isConsumed;
}

inline LONG DrawClass::width()
{
    return width_ - left_border_ - right_border_;
}

inline LONG DrawClass::height()
{
    return height_ - bottom_border_ - top_border_;
}

inline VOID DrawClass::setRed(LONG w, LONG h, BYTE value)
{
    size_t anchor = realHeight(h) * widthBytes_ + w * bytesPerPixel_ + 2;
    if (buffer_->size() > anchor && anchor >= 0)
    {
        buffer_->at(realHeight(h) * widthBytes_ + w * bytesPerPixel_ + 2) = value;
    }
}

inline VOID DrawClass::setBlue(LONG w, LONG h, BYTE value)
{
    size_t anchor = realHeight(h) * widthBytes_ + w * bytesPerPixel_;
    if (buffer_->size() > anchor && anchor >= 0)
    {
        buffer_->at(realHeight(h) * widthBytes_ + w * bytesPerPixel_) = value;
    }
}

inline VOID DrawClass::setGreen(LONG w, LONG h, BYTE value)
{
    size_t anchor = realHeight(h) * widthBytes_ + w * bytesPerPixel_ + 1;
    if (buffer_->size() > anchor && anchor >= 0)
    {
        buffer_->at(realHeight(h) * widthBytes_ + w * bytesPerPixel_ + 1) = value;
    }
}

inline VOID DrawClass::setColor(LONG w, LONG h, int_fast32_t color)
{
    size_t anchor = realHeight(h) * widthBytes_ + w * bytesPerPixel_;
    if (buffer_->size() > anchor + 2 && anchor >= 0)
    {
        BYTE* data = buffer_->data();
        memcpy(data + anchor, &color, 3);
    }
}

inline LONG DrawClass::realHeight(LONG height)
{
    return height + top_border_ + bottom_border_;
}

void DrawClass::draw_title(HDC hdc)
{
    int area_id;
    int x, y;
    int w, h;
    int x_margin, y_margin;
    std::vector<BYTE> cubename;
    TextBitmapCreator tc;
    std::wstring wstr;

    // draw area name
    w = Config::get_instance().area_name_width();
    h = Config::get_instance().title_text_height();
    area_id = cube_center_->area();
    wstr = map_->areas_.at(area_id)->wtitle();
    tc.reset();
    tc.addLine(wstr);
    tc.getBitmapFixHeight(hdc, w, h, cubename);

    x_margin = (Config::get_instance().area_name_width() - w) / 2;
    y_margin = (Config::get_instance().title_height() - h) / 2;
    x = x_margin;
    y = height() - h - y_margin;

    // merge text bitmap and main bitmap
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            size_t anchor = (i * w + j) * 4;
            int_fast32_t color;
            memcpy(&color, cubename.data() + anchor, 4);
            setColor(j + x, i + y, color);
        }
    }

    // draw cube name
    w = Config::get_instance().cube_name_width();
    h = Config::get_instance().title_text_height();
    wstr = cube_center_->wtitle();
    tc.reset();
    tc.addLine(wstr);
    tc.getBitmapFixHeight(hdc, w, h, cubename);
    x_margin = 0;
    y_margin = (Config::get_instance().title_height() - h) / 2;
    x = x_margin + Config::get_instance().area_name_width();
    y = height() - h - y_margin;

    // merge text bitmap and main bitmap
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            size_t anchor = (i * w + j) * 4;
            int_fast32_t color;
            memcpy(&color, cubename.data() + anchor, 4);
            setColor(j + x, i + y, color);
        }
    }
}

void DrawClass::draw_console(HDC hdc)
{
    std::vector<BYTE> out;

    int w = b_roller_.width();
    int h = Config::get_instance().console_text_height();

    while (! text_bitmap_.isEmpty())
    {
        text_bitmap_.getBitmapFixHeight(hdc, w, h, out);
        b_roller_.add(out, w, h, BitmapRoller::CENTER);
    }

    b_roller_.render(out);

    for (int i = 0; i < b_roller_.height(); i++)
    {
        int anchor1 = (
            ( i + Config::get_instance().console_window_pos_y()) * width() 
            + Config::get_instance().console_window_pos_x() ) 
            * Config::get_instance().pixel_size();
        int anchor2 = i * b_roller_.width() * Config::get_instance().pixel_size();
        int bitmap_line_size = b_roller_.width() * Config::get_instance().pixel_size();

        for (int j = 0; j < bitmap_line_size; j++)
        {
            BYTE data = (*(buffer_->data() + anchor1 + j) | *(out.data() + anchor2 + j));
            memcpy(buffer_->data() + anchor1 + j, &data, sizeof(BYTE));
        }
    }
}

void DrawClass::draw_map(HDC hdc, octillion::CubePosition loc, Matrix<double> matrix, int flag)
{
    std::shared_ptr<std::vector<BYTE>> bitmap =
        maze_bitmap_.render(cube_center_->loc(), matrix, 10, flag);

    int bitmap_line_size = maze_bitmap_.width() > width() ? 
        width() * Config::get_instance().pixel_size() : 
        maze_bitmap_.width() * Config::get_instance().pixel_size();

    int lines = maze_bitmap_.height() > height() ?
        height() : maze_bitmap_.height();

    for (int i = 0; i < lines; i++)
    {
        int anchor1 = i * width() * Config::get_instance().pixel_size();
        int anchor2 = i * maze_bitmap_.width() * Config::get_instance().pixel_size();
        BYTE* canvas_px = buffer_->data() + anchor1;
        BYTE* bitmap_px = bitmap->data() + anchor2;
        memcpy(canvas_px, bitmap_px, bitmap_line_size);
    }
}

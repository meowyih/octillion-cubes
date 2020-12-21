#ifndef CONFIG_HEADER
#define CONFIG_HEADER
class Config
{
public:
    void init(int screen_width, int screen_height);
    bool is_init() { return init_; }

    // pixel
    int pixel_size() { return 4; }

    // title bar
    int title_height() { return title_height_; }
    int title_text_height() { return title_text_height_;  }
    int area_name_width() { return area_name_width_; }
    int cube_name_width() { return cube_name_width_; }
    int cube_status_width() { return cube_status_width_; }

    // console
    int console_window_width() { return console_window_width_; }
    int console_window_height() { return console_window_height_; }
    int console_window_pos_x() { return console_window_pos_x_; }
    int console_window_pos_y() { return console_window_pos_y_; }
    int console_text_height() { return console_text_height_; }

private:
    bool init_ = false;
    int w_ = 0, h_ = 0;

    // title bar
    int title_height_ = 0;
    int title_text_height_ = 0;
    int area_name_width_ = 0;
    int cube_name_width_ = 0;
    int cube_status_width_ = 0;

     // console
    int console_window_width_ = 0;
    int console_window_height_ = 0;
    int console_window_pos_x_ = 0;
    int console_window_pos_y_ = 0;
    int console_text_height_ = 0;

// singleton
public:
    static Config& get_instance()
    {
        static Config instance;
        return instance;
    }

private:
    Config();
    ~Config();

public:
    // avoid accidentally copy
    Config(Config const&) = delete;
    void operator = (Config const&) = delete;
};
#endif
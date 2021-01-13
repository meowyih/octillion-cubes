#ifndef CONFIG_HEADER
#define CONFIG_HEADER

#include <string>

class Config
{
private:
    const std::string tag_ = "Config";

public:
    void init(int screen_width, int screen_height);
    bool is_init() { return init_; }

    // pixel
    int pixel_size() { return 4; }

    // 3d map
    int map_window_width() { return map_window_width_; }
    int map_window_height() { return map_window_height_; }

    // console
    int console_window_width() { return console_window_width_; }
    int console_window_height() { return console_window_height_; }
    int console_window_pos_x() { return console_window_pos_x_; }
    int console_window_pos_y() { return console_window_pos_y_; }
    int console_text_height() { return console_text_height_; }

    std::string console_fontname_str() { return console_font_name_str_; }
    std::wstring console_fontname_wstr() { return console_font_name_wstr_; }

    // interative
    int interactive_window_width() { return interactive_window_width_; }
    int interactive_window_height() { return interactive_window_height_; }
    int interactive_window_pos_x() { return interactive_window_pos_x_; }
    int interactive_window_pos_y() { return interactive_window_pos_y_; }
    int interactive_text_height() { return interactive_text_height_; }

    std::string interactive_fontname_str() { return interactive_font_name_str_; }
    std::wstring interactive_fontname_wstr() { return interactive_font_name_wstr_; }

    // symbol font name
    std::string symbol_fontname_str() { return symbol_font_name_str_; }
    std::wstring symbol_fontname_wstr() { return symbol_font_name_wstr_; }
private:
    bool init_ = false;
    int w_ = 0, h_ = 0;

    // map
    float map_window_width_ratio_ = 0.0;
    int map_window_width_ = 0;

    float map_window_height_ratio_ = 0.0;
    int map_window_height_ = 0;

     // console
    float console_window_width_ratio_ = 0.0;
    int console_window_width_ = 0;

    float console_window_height_ratio_ = 0.0;
    int console_window_height_ = 0;

    float console_text_height_ratio_ = 0.0;
    int console_text_height_ = 0;

    std::string console_font_name_str_;
    std::wstring console_font_name_wstr_;

    int console_window_pos_x_ = 0;
    int console_window_pos_y_ = 0;

    // interactive
    float interactive_window_width_ratio_ = 0.0;
    int interactive_window_width_ = 0;

    float interactive_window_height_ratio_ = 0.0;
    int interactive_window_height_ = 0;

    float interactive_text_height_ratio_ = 0.0;
    int interactive_text_height_ = 0;

    std::string interactive_font_name_str_;
    std::wstring interactive_font_name_wstr_;

    int interactive_window_pos_x_ = 0;
    int interactive_window_pos_y_ = 0;
    
    std::string symbol_font_name_str_;
    std::wstring symbol_font_name_wstr_;

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
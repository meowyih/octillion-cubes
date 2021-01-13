#include <memory>
#include <string>
#include <fstream>

#include "Config.hpp"

#include "error/macrolog.hpp"
#include "jsonw/jsonw.hpp"

void Config::init(int screen_width, int screen_height)
{
    if (init_)
    {
        // only reinit if changed size
        if (w_ == screen_width && h_ == screen_height)
        {
            return;
        }
    }

    std::string jsonfile = "config.json";
    std::ifstream fin(jsonfile);
    JsonW jconfig(fin);
    std::shared_ptr<JsonW> jdata;

    w_ = screen_width;
    h_ = screen_height;

    // read config file
    if (!jconfig.valid())
    {
        LOG_W(tag_) << "Bad config json file";
        return;
    }

    // read map
    jdata = jconfig.get(u8"map");

    if (map_window_width_ratio_ == 0.0 && jdata != nullptr && jdata->get("width%") != nullptr)
    {
        std::shared_ptr<JsonW> jwidth = jdata->get("width%");
        if (jwidth->type() == JsonW::FLOAT)
        {
            map_window_width_ratio_ = (float)jwidth->frac();
            map_window_width_ = (int)(w_ * map_window_width_ratio_);
        }
    }

    if (map_window_height_ratio_ == 0.0 && jdata != nullptr && jdata->get("height%") != nullptr)
    {
        std::shared_ptr<JsonW> jheight = jdata->get("height%");
        if (jheight->type() == JsonW::FLOAT)
        {
            map_window_height_ratio_ = (float)jheight->frac();
            map_window_height_ = (int)(h_ * map_window_height_ratio_);
        }
    }

    // read console
    jdata = jconfig.get(u8"console");

    if (console_window_width_ratio_ == 0.0 && jdata != nullptr && jdata->get("width%") != nullptr)
    {
        std::shared_ptr<JsonW> jwidth = jdata->get("width%");
        if (jwidth->type() == JsonW::FLOAT)
        {
            console_window_width_ratio_ = (float)jwidth->frac();
            console_window_width_ = (int)(w_ * console_window_width_ratio_);
        }
    }

    if (console_window_height_ratio_ == 0.0 && jdata != nullptr && jdata->get("height%") != nullptr)
    {
        std::shared_ptr<JsonW> jheight = jdata->get("height%");
        if (jheight->type() == JsonW::FLOAT)
        {
            console_window_height_ratio_ = (float)jheight->frac();
            console_window_height_ = (int)(h_ * console_window_height_ratio_);
        }
    }

    if (console_text_height_ratio_ == 0.0 && jdata != nullptr && jdata->get("text height%") != nullptr)
    {
        std::shared_ptr<JsonW> jheight = jdata->get("text height%");
        if (jheight->type() == JsonW::FLOAT)
        {
            console_text_height_ratio_ = (float)jheight->frac();
            console_text_height_ = (int)(h_ * console_text_height_ratio_);
        }
    }

    console_window_pos_x_ = (w_ - console_window_width_) / 2;
    console_window_pos_y_ = 0;

    // read console font name
    if (console_font_name_str_.length() == 0 && jdata != nullptr && jdata->get("font-default") != nullptr)
    {
        std::shared_ptr<JsonW> jfont = jdata->get("font-default");
        if (jfont->type() == JsonW::STRING)
        {
            console_font_name_str_ = jfont->str();
            console_font_name_wstr_ = jfont->wstr();
        }
    }

    // read interactive
    jdata = jconfig.get(u8"interactive");

    if (interactive_window_width_ratio_ == 0.0 && jdata != nullptr && jdata->get("width%") != nullptr)
    {
        std::shared_ptr<JsonW> jwidth = jdata->get("width%");
        if (jwidth->type() == JsonW::FLOAT)
        {
            interactive_window_width_ratio_ = (float)jwidth->frac();
            interactive_window_width_ = (int)(w_ * interactive_window_width_ratio_);
        }
    }

    if (interactive_window_height_ratio_ == 0.0 && jdata != nullptr && jdata->get("height%") != nullptr)
    {
        std::shared_ptr<JsonW> jheight = jdata->get("height%");
        if (jheight->type() == JsonW::FLOAT)
        {
            interactive_window_height_ratio_ = (float)jheight->frac();
            interactive_window_height_ = (int)(h_ * interactive_window_height_ratio_);
        }
    }

    if (interactive_text_height_ratio_ == 0.0 && jdata != nullptr && jdata->get("text height%") != nullptr)
    {
        std::shared_ptr<JsonW> jheight = jdata->get("text height%");
        if (jheight->type() == JsonW::FLOAT)
        {
            interactive_text_height_ratio_ = (float)jheight->frac();
            interactive_text_height_ = (int)(h_ * interactive_text_height_ratio_);
        }
    }

    interactive_window_pos_x_ = 0;
    interactive_window_pos_y_ = 0;

    // read interactive font name
    if (interactive_font_name_str_.length() == 0 && jdata != nullptr && jdata->get("font-default") != nullptr)
    {
        std::shared_ptr<JsonW> jfont = jdata->get("font-default");
        if (jfont->type() == JsonW::STRING)
        {
            interactive_font_name_str_ = jfont->str();
            interactive_font_name_wstr_ = jfont->wstr();
        }
    }

    // read symbol font name
    if (symbol_font_name_str_.length() == 0 && jconfig.get("font-symbol") != nullptr)
    {
        std::shared_ptr<JsonW> jfont = jconfig.get("font-symbol");
        if (jfont->type() == JsonW::STRING)
        {
            symbol_font_name_str_ = jfont->str();
            symbol_font_name_wstr_ = jfont->wstr();
        }
    }

    // set flag
    init_ = true;

    return;
}

Config::Config()
{
    init_ = false;
}

Config::~Config()
{
}

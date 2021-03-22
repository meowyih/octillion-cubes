#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#include <string>
#include <memory>
#include <chrono>

#include "world/worldmap.hpp"
#include "world/storage.hpp"

#include "D2BrushCreator.hpp"
#include "D2TextFormatCreator.hpp"

#include "WorldLoader.hpp"
#include "MapFrameLoader.hpp"

#include "MyWriteTextRenderer.hpp"

#include "SectionMenu.hpp"
#include "SectionText.hpp"
#include "SectionMap.hpp"

using std::shared_ptr;
using octillion::WorldMap;
using std::chrono::system_clock;

class Kernel
{
private:
    const std::string tag_ = "Kernel";

private:
    const static int STATE_UNSET = 0;
    const static int STATE_CREATED = 1;
    const static int STATE_LOADING_WORLD_DATA = 2;
    const static int STATE_LOADING_MAP_FRAME = 3;

    // temparary state for dev usage 
    const static int STATE_GAMING = 10;

    // fatal error, display the error page and wait for terminate (or report?)
    const static int STATE_FATAL_ERROR = 255;

private:
    int state_;
    WorldLoader world_loader_;
    MapFrameLoader map_loader_;
    shared_ptr<WorldMap> world_data_;
    system_clock::duration time_stamp_;

    // for test
    ID2D1GeometryGroup* geogroup_;

public:
    Kernel();
    ~Kernel();

    // step 1: create directx resoruce once and only once when window init
    bool init_d2d();

    // step 2: read data/load user information/render map in *different thread*
    // shows loading or company logo here
    void load_world();

    // step 3: render map in *different thread* shows loading page here
    void load_map();

    // step 4: called everytime when window size changed include the first launch
    // ** we know there are at least one WM_SIZE called before first WM_PAINT **
    bool init_screen(HWND hwnd);

    // step 5: paint the screen when WM_PAINT set
    void paint();

    void paint_loading();
    void paint_error();
    void paint_default();

    // timer callback
    void timer_callback();

    bool init( HWND hwnd );
    bool fini();

    void resize(UINT w, UINT h);
    void update();

    void keypress(UINT keycode);

    void move_to(std::shared_ptr<octillion::Cube> dest_cube);
    void handle_action(std::shared_ptr<std::vector<octillion::Action>> p_acts);
    void add_text(std::shared_ptr<octillion::StringData>& strdata);
    void handle_action_set(octillion::Action& action);

private:
    // direct2d basic (d2d1.h/d2d1.lib)
    ID2D1Factory* d2d1_factory_;
    ID2D1HwndRenderTarget* d2d1_target_;

    // direct write (dwrite.h/dwrite.lib)
    IDWriteFactory* dw_factory_;

    bool is_init_;

    D2BrushCreator brush_;
    D2TextFormatCreator txtformat_;

    SectionMenu section_menu_;
    SectionText section_text_;
    SectionMap section_map_;

    octillion::WorldMap world_;
    std::shared_ptr<octillion::Cube> center_cube_;

    octillion::Storage storage_;
};


#include <d2d1.h>
#include <wincodec.h>
#include <Windows.h>
#include <comdef.h>
#include <atlstr.h>
#include <d3d11.h>
#include <d2d1_1.h>

#include "resource.h"

#include "Kernel.hpp"
#include "D2BrushCreator.hpp"
#include "SectionSubText.hpp"
#include "AreaContainer.hpp"
#include "MapFrame.hpp"

#include "error/macrolog.hpp"

#include "MyWriteTextRenderer.hpp"

using std::make_shared;

Kernel::Kernel()
{
    d2d1_factory_ = NULL;
    d2d1_target_ = NULL;
    dw_factory_ = NULL;
    geogroup_ = NULL;
    state_ = STATE_CREATED;
    time_stamp_ = system_clock::now().time_since_epoch();

    is_init_ = false;
}

Kernel::~Kernel()
{
    fini();
}

bool Kernel::init_d2d()
{
    HRESULT hr;

    LOG_D(tag_) << "init_d2d() enter";

    // create direct2d factory and directword
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1_factory_);

    if (SUCCEEDED(hr))
    {
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&dw_factory_)
        );
    }

    if (SUCCEEDED(hr))
    {
        LOG_D(tag_) << "init_d2d() successfully leave";
        return true;
    }
    else
    {
        LOG_D(tag_) << "init_d2d() failed, leave";
        return false;
    }
}

void Kernel::load_world()
{
    // check parameters' validation
    if (!world_loader_.is_idle())
    {
        state_ = STATE_FATAL_ERROR;
        return;
    }

    if (world_data_ != nullptr)
    {
        state_ = STATE_FATAL_ERROR;
        return;
    }

    // start loading thread
    world_data_ = make_shared<WorldMap>();
    world_loader_.init(world_data_, "./data/");

    return;
}

void Kernel::load_map()
{
    // check parameters' validation
    if (!map_loader_.is_idle())
    {
        state_ = STATE_FATAL_ERROR;
        return;
    }

    if (geogroup_ != nullptr)
    {
        state_ = STATE_FATAL_ERROR;
        return;
    }

    // start loading thread
    map_loader_.init(d2d1_factory_, &geogroup_, world_data_->reborn_);

    return;
}

bool Kernel::init_screen(HWND hwnd)
{
    HRESULT hr = S_OK;

    // get drawable window size (client rect)
    RECT crect;
    GetClientRect(hwnd, &crect);

    // calculate the sub-element size
    UINT width = crect.right - crect.left;
    UINT height = crect.bottom - crect.top;

    FLOAT menu_w = (FLOAT)100.0f;
    FLOAT menu_h = (FLOAT)height;
    FLOAT menu_x = (FLOAT)width * 1.2f / 2.618f;

    FLOAT txt_padding = 5.f;
    FLOAT txt_w = (FLOAT)width - (menu_x + menu_w) - txt_padding * 2.f;
    FLOAT txt_h = (FLOAT)height - txt_padding * 2.f;
    FLOAT txt_x = menu_x + menu_w + txt_padding;
    FLOAT txt_y = txt_padding;

    // create or resize the d2d render target
    if (d2d1_target_ == NULL)
    {
        hr = d2d1_factory_->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd,
                D2D1::SizeU( width, height)
            ),
            &d2d1_target_);
    }
    else
    {
        d2d1_target_->Resize(D2D1::SizeU( width, height));
    }

    // init the subelements
    section_menu_.init(menu_w, menu_h, menu_x, 0);
    section_text_.init(D2D1::RectF(txt_x, txt_y, txt_x + txt_w, txt_y + txt_h));

    return false;
}

void Kernel::paint()
{
    HRESULT hr;

    d2d1_target_->BeginDraw();

    d2d1_target_->SetTransform(D2D1::Matrix3x2F::Identity());

    switch (state_)
    {
    case STATE_LOADING_WORLD_DATA:
    case STATE_LOADING_MAP_FRAME:
        paint_loading();
        break;
    case STATE_FATAL_ERROR:
        paint_error();
        break;
    case STATE_GAMING:
    default:
        paint_default();
    }

    hr = d2d1_target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        LOG_D(tag_) << "EndDraw return D2DERR_RECREATE_TARGET, call fini()";
        fini();
    }
}

void Kernel::paint_loading()
{
    D2D1_RECT_F rect;

    D2D1_SIZE_F rtSize = d2d1_target_->GetSize();

    // test if boundary is correct
    rect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);

    d2d1_target_->FillRectangle(rect,
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_BACKGROUND));

    d2d1_target_->DrawText(L"Loading", 7, 
        txtformat_.get(dw_factory_, D2TextFormatCreator::DEFAULT), 
        D2D1::RectF(0.f, 0.f, 100.f, 50.f),
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_RED));
}

void Kernel::paint_error()
{
    D2D1_RECT_F rect;

    D2D1_SIZE_F rtSize = d2d1_target_->GetSize();

    // test if boundary is correct
    rect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);

    d2d1_target_->FillRectangle(rect,
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_BACKGROUND));

    d2d1_target_->DrawText(L"Error", 5, 
        txtformat_.get(dw_factory_, D2TextFormatCreator::DEFAULT), 
        D2D1::RectF(0.f, 0.f, 100.f, 50.f),
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_RED));
}

void Kernel::paint_default()
{
    D2D1_RECT_F rect;

    D2D1_SIZE_F rtSize = d2d1_target_->GetSize();

    // test if boundary is correct
    rect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);

    d2d1_target_->FillRectangle(rect,
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_BACKGROUND));

    d2d1_target_->DrawText(L"Gaming", 6,
        txtformat_.get(dw_factory_, D2TextFormatCreator::DEFAULT),
        D2D1::RectF(0.f, 0.f, 100.f, 50.f),
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_RED));

    d2d1_target_->DrawGeometry(
        geogroup_, 
        brush_.get(d2d1_target_, D2BrushCreator::SOLID_MIDDLE_SEPERATOR));
}

bool Kernel::fini()
{
    LOG_D(tag_) << "fini enter";

    brush_.fini();

    if (d2d1_target_ != NULL)
        d2d1_target_->Release();

    if (d2d1_factory_ != NULL)
        d2d1_factory_->Release();

    if (dw_factory_ != NULL)
        dw_factory_->Release();

    d2d1_factory_ = NULL;
    d2d1_target_ = NULL;
    dw_factory_ = NULL;
    is_init_ = false;

    LOG_D(tag_) << "fini leave";

    return true;
}

// state machine
void Kernel::timer_callback()
{
    int old_state = state_;

    // handle the pending state
    if (state_ == STATE_LOADING_WORLD_DATA
        && world_loader_.state() == BaseLoader::STATE_LOADING)
    {
        return;
    }
    else if (state_ == STATE_LOADING_MAP_FRAME
        && map_loader_.state() == BaseLoader::STATE_LOADING)
    {
        return;
    }

    // handle the error state
    if (state_ == STATE_LOADING_WORLD_DATA 
        && world_loader_.state() == BaseLoader::STATE_FAILED)
    {
        state_ = STATE_FATAL_ERROR;
        LOG_D(tag_) << "change state from STATE_LOADING_WORLD_DATA to STATE_FATAL_ERROR";
        return;
    }
    else if (state_ == STATE_LOADING_MAP_FRAME
        && map_loader_.state() == BaseLoader::STATE_FAILED)
    {
        state_ = STATE_FATAL_ERROR;
        LOG_D(tag_) << "change state from STATE_LOADING_MAP_FRAME to STATE_FATAL_ERROR";
        return;
    }

    // then handle the normal state
    switch (state_)
    {
    case STATE_CREATED:
        state_ = STATE_LOADING_WORLD_DATA;
        load_world();
        break;

    case STATE_LOADING_WORLD_DATA:
        state_ = STATE_LOADING_MAP_FRAME;
        load_map();
        break;

    case STATE_LOADING_MAP_FRAME:
        state_ = STATE_GAMING;
        break;

    default:
        break;
    }

    if (state_ != old_state)
    {
        LOG_D(tag_) << "change state from " << old_state << " to " << state_;
    }

    return;
}

bool Kernel::init(HWND hwnd)
{
    HRESULT hret;
    RECT wrect, crect;

    LOG_D(tag_) << "init enter";

    if (is_init_)
    {
        LOG_D(tag_) << "init, already initialized";
        return true;
    }
    
    GetWindowRect(hwnd, &wrect);
    GetClientRect(hwnd, &crect);

    hret = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1_factory_);

    if (hret != S_OK)
    {
        LOG_D(tag_) << "init, D2D1CreateFactory failed, ret " << hret;
        fini();
        return false;
    }

    hret = d2d1_factory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd,
            D2D1::SizeU(
                crect.right - crect.left,
                crect.bottom - crect.top)
         ),
        &d2d1_target_);

    if (hret != S_OK)
    {
        LOG_D(tag_) << "init, CreateHwndRenderTarget failed, ret " << hret;
        fini();
        return false;
    }

    hret = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&dw_factory_)
    );
    if (hret != S_OK)
    {
        LOG_D(tag_) << "init, DWriteCreateFactory failed, ret " << hret;
        fini();
        return false;
    }

    if (world_.load_external_data_file("./data/") == false)
    {
        return false;
    }

    move_to( world_.reborn_ );

    AreaContainer ac;
    auto it_world_areas = world_.areas_.find(10);

    if (it_world_areas != world_.areas_.end())
    {
        std::vector<std::shared_ptr<octillion::Area>> area_array;
        area_array.push_back(it_world_areas->second);
        ac.set(area_array);
    }

    section_map_.init(d2d1_factory_, D2D1::RectF(0, 0, 400.f, 400.f), world_.reborn_);

    is_init_ = true;

    return true;
}

void Kernel::resize(UINT w, UINT h)
{
    FLOAT menu_w = (FLOAT)100.0f;
    FLOAT menu_h = (FLOAT)h;
    FLOAT menu_x = (FLOAT)w * 1.2f / 2.618f;

    FLOAT txt_padding = 5.f;
    FLOAT txt_w = (FLOAT)w - (menu_x + menu_w) - txt_padding * 2.f;
    FLOAT txt_h = (FLOAT)h - txt_padding * 2.f;
    FLOAT txt_x = menu_x + menu_w + txt_padding;
    FLOAT txt_y = txt_padding;

    if (d2d1_target_ != NULL)
    {
        d2d1_target_->Resize(D2D1::SizeU(w, h));
    }

    section_menu_.init(menu_w, menu_h, menu_x, 0);
    section_text_.init(D2D1::RectF(txt_x, txt_y, txt_x + txt_w, txt_y + txt_h));
}

void Kernel::update()
{
    HRESULT hret;
    D2D1_RECT_F rect;

    if (!is_init_)
    {
        LOG_E(tag_) << "update() failed, due to not initalize";
        return;
    }

    d2d1_target_->BeginDraw();

    d2d1_target_->SetTransform(D2D1::Matrix3x2F::Identity());

    D2D1_SIZE_F rtSize = d2d1_target_->GetSize();

    // test if boundary is correct
    rect = D2D1::RectF( 0, 0, rtSize.width, rtSize.height );

    d2d1_target_->FillRectangle(rect, 
       brush_.get(d2d1_target_, D2BrushCreator::SOLID_BACKGROUND));

    section_menu_.update(d2d1_target_, brush_);
 
    section_text_.update(d2d1_factory_, d2d1_target_, brush_);

    FLOAT side_length = 30.f;
    d2d1_target_->DrawRectangle(D2D1::RectF(0, 0, 400.f, 400.f), brush_.get(d2d1_target_, D2BrushCreator::SOLID_WHITE));
    d2d1_target_->DrawRectangle(D2D1::RectF(199.f - side_length, 199.f, 201.f - side_length, 201.f), brush_.get(d2d1_target_, D2BrushCreator::SOLID_RED));
    d2d1_target_->DrawRectangle(D2D1::RectF(199.f, 199.f, 201.f, 201.f), brush_.get(d2d1_target_, D2BrushCreator::SOLID_RED));
    d2d1_target_->DrawRectangle(D2D1::RectF(199.f + side_length, 199.f, 201.f + side_length, 201.f), brush_.get(d2d1_target_, D2BrushCreator::SOLID_RED));
    section_map_.update(d2d1_target_, brush_, center_cube_, D2D1::Point2F(0.f, 0.f), side_length);
   
    hret = d2d1_target_->EndDraw();
    if (hret == D2DERR_RECREATE_TARGET)
    {
        LOG_D(tag_) << "EndDraw return D2DERR_RECREATE_TARGET, call fini()";
        fini();
    }

    return;
}

void Kernel::keypress(UINT keycode)
{
    if (keycode == 0x58) // x
    {
        // section_text_.add(L"X", dw_factory_, txtformat_);
        section_text_.add(L"你張開眼睛，發現自己身處於一個巨大的白色空間中，四周空一物。你想不起來自己是怎麼到達這個地方的。", dw_factory_, txtformat_);
        // section_text_.pos_inc(1.f);
        // section_map_.side_length_ += 1.f;
    }
    else if (keycode == 0x59) // y
    {
        section_text_.add(L"建構者", L"此時，一個聲音在你腦中響起：「年輕的靈魂，先試著移動自己看看。」", dw_factory_, txtformat_);
        // section_text_.add(L"y", dw_factory_, txtformat_);
        // section_text_.pos_inc(-1.f);
        // section_map_.side_length_ -= 1.f;
    }
    else if (keycode == VK_RIGHT && !section_map_.is_animation_running() &&
        center_cube_->exits_[octillion::Cube::X_INC] != 0)
    {
        section_map_.start_animation(200, 
            center_cube_, 
            D2D1::Point2F(0.f, 0.f), 30.f,
            center_cube_->adjacent_cubes_[octillion::Cube::X_INC],
            D2D1::Point2F(0.f, 0.f), 30.f);
        move_to( center_cube_->adjacent_cubes_[octillion::Cube::X_INC] );
    }
    else if (keycode == VK_LEFT && !section_map_.is_animation_running() && 
        center_cube_->exits_[octillion::Cube::X_DEC] != 0)
    {
        section_map_.start_animation(200,
            center_cube_,
            D2D1::Point2F(0.f, 0.f), 30.f,
            center_cube_->adjacent_cubes_[octillion::Cube::X_DEC],
            D2D1::Point2F(0.f, 0.f), 30.f);
        move_to( center_cube_->adjacent_cubes_[octillion::Cube::X_DEC] );
    }
    else if (keycode == VK_UP && !section_map_.is_animation_running() && 
        center_cube_->exits_[octillion::Cube::Y_DEC] != 0)
    {
        section_map_.start_animation(200,
            center_cube_,
            D2D1::Point2F(0.f, 0.f), 30.f,
            center_cube_->adjacent_cubes_[octillion::Cube::Y_DEC],
            D2D1::Point2F(0.f, 0.f), 30.f);
        move_to( center_cube_->adjacent_cubes_[octillion::Cube::Y_DEC] );
    }
    else if (keycode == VK_DOWN && !section_map_.is_animation_running() && 
        center_cube_->exits_[octillion::Cube::Y_INC] != 0)
    {
        section_map_.start_animation(200,
            center_cube_,
            D2D1::Point2F(0.f, 0.f), 30.f,
            center_cube_->adjacent_cubes_[octillion::Cube::Y_INC],
            D2D1::Point2F(0.f, 0.f), 30.f);
        move_to( center_cube_->adjacent_cubes_[octillion::Cube::Y_INC] );
    }
}

void Kernel::move_to(std::shared_ptr<octillion::Cube> dest_cube)
{
    std::shared_ptr<octillion::StringData> exit_desc = nullptr;

    // get cube exit desc
    if (center_cube_ != nullptr)
    {
        int dir = octillion::Cube::dir(center_cube_, dest_cube);
        exit_desc = center_cube_->desc_exits_[dir];
    }

    // increase move count for that area
    if (center_cube_ != nullptr)
    {
        storage_.move_count_inc(dest_cube->area());
    }

    center_cube_ = dest_cube;
    storage_.areaid_ = dest_cube->area();
    storage_.loc_ = dest_cube->loc();

    if (exit_desc != nullptr)
    {
        add_text(exit_desc);
    }
    
    // read and handle the script
    size_t total_script = world_.areas_.at(center_cube_->area())->scripts_move_.size();
    std::shared_ptr<octillion::Area> area = world_.areas_.at(center_cube_->area());
    for (size_t i = 0; i < total_script; i++)
    {
        std::shared_ptr<std::vector<octillion::Action>> p_acts =
            area->scripts_move_.at(i).handle(storage_);

        if (p_acts != nullptr)
        {
            handle_action(p_acts);
        }
    }
}

void Kernel::handle_action(std::shared_ptr<std::vector<octillion::Action>> p_acts)
{
    for (auto it = p_acts->begin(); it != p_acts->end(); it++)
    {
        int type = it->action_type_;
        int var = it->variable_type_;

        if (type == octillion::Action::ACTION_TYPE_TEXT && var == octillion::Action::VARIABLE_TYPE_TEXT)
        {
            int area_id = center_cube_->area();
            int string_id = it->i_rhs_;

            std::shared_ptr<octillion::StringData> it = world_.areas_.at(area_id)->string_table_.find(string_id);

            if (it == nullptr)
            {
                LOG_D(tag_) << "string id: " << string_id << " does not valid in area id " << area_id;
                return;
            }

            add_text(it);
        }
        else if (type == octillion::Action::ACTION_TYPE_RESET_TIMER)
        {
            storage_.reset_timer();
        }
        else if (type == octillion::Action::ACTION_TYPE_SET)
        {
            handle_action_set(*it);
        }
    }
}

void Kernel::add_text(std::shared_ptr<octillion::StringData>& strdata)
{
    if (strdata == nullptr || (*strdata).wstr_.size() == 0)
        return;

    if ((*strdata).wstr_.size() == 1)
    {
        section_text_.add((*strdata).wstr_.at(0), dw_factory_, txtformat_);
    }
    else if ((*strdata).select_ == octillion::StringTable::SELECT_RANDOMLY)
    {
        int idx = rand() % (*strdata).wstr_.size();
        section_text_.add((*strdata).wstr_.at(idx), dw_factory_, txtformat_);
    }
    else if ((*strdata).select_ == octillion::StringTable::SELECT_ORDERLY)
    {
        int idx = (*strdata).last_index_ % (*strdata).wstr_.size();
        section_text_.add((*strdata).wstr_.at(idx), dw_factory_, txtformat_);
        (*strdata).last_index_++;
    }
    else if ((*strdata).select_ == octillion::StringTable::SELECT_ALL)
    {
        LOG_D(tag_) << "sting data size: " << (*strdata).wstr_.size();

        for (auto itstr = (*strdata).wstr_.begin(); itstr != (*strdata).wstr_.end(); itstr++)
        {
            section_text_.add(*itstr, dw_factory_, txtformat_);
        }
    }
    else
    {
        LOG_E(tag_) << "unsupport string data select type: " << (*strdata).select_;
    }
}

void Kernel::handle_action_set(octillion::Action& action)
{
    switch (action.variable_type_)
    {
    case octillion::Action::VARIABLE_TYPE_PERMANENT_INTEGER_1:
        storage_.integer_set(center_cube_->area(), 1, action.i_rhs_);
        break;
    default:
        LOG_E(tag_) << "handle_action_set() unsuuport action var type:" << action.variable_type_;
    }
}
#include "error/macrolog.hpp"
#include "D2BrushCreator.hpp"

D2BrushCreator::D2BrushCreator()
{
}

D2BrushCreator::~D2BrushCreator()
{
}

void D2BrushCreator::fini()
{
    for (auto it = brushes_.begin(); it != brushes_.end(); it++)
    {
        LOG_D(tag_) << "fini(), Release brush id: " << (*it).first;
        (*it).second->Release();
    }

    brushes_.clear();
}

ID2D1Brush* D2BrushCreator::get(ID2D1HwndRenderTarget* target, int id)
{
    HRESULT hr = E_FAIL;
    ID2D1SolidColorBrush* sc_brush = NULL;
    ID2D1BitmapBrush* bb_brush = NULL;
    ID2D1LinearGradientBrush* lg_brush = NULL;
    ID2D1RadialGradientBrush* rg_brush = NULL;

    auto it = brushes_.find(id);

    if (it != brushes_.end())
    {
        // find the brush in store
        return (*it).second;
    }

    switch (id)
    {
    case SOLID_DEFAULT:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(221.0f / 255.0f, 210.0f / 255.0f, 193.0f / 255.0f), &sc_brush);
        break;
    case SOLID_BACKGROUND:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(221.0f / 255.0f, 210.0f / 255.0f, 193.0f / 255.0f), &sc_brush);
        break;
    case SOLID_MIDDLE_SEPERATOR:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(70.0f / 255.0f, 70.0f / 255.0f, 70.0f / 255.0f), &sc_brush);
        break;
    case SOLID_BLACK:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &sc_brush);
        break;
    case SOLID_WHITE:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &sc_brush);
        break;
    case SOLID_RED:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f), &sc_brush);
        break;
    case SOLID_GENERAL_TEXT:
        hr = target->CreateSolidColorBrush(D2D1::ColorF(40.0f / 255.0f, 20.0f / 255.0f, 3.0f / 255.0f), &sc_brush);
        break;
    default:
        break;
    }

    if (SUCCEEDED(hr))
    {
        ID2D1Brush* brush = NULL;

        if (sc_brush != NULL)
            brush = sc_brush;
        else if (bb_brush != NULL)
            brush = bb_brush;
        else if (lg_brush != NULL)
            brush = lg_brush;
        else if (rg_brush != NULL)
            brush = rg_brush;
        else
            return NULL;

        brushes_.insert(std::pair<int, ID2D1Brush*>(id, brush));
        LOG_D(tag_) << "create and insert brush id: " << id;
        return brush;
    }

    return NULL;
}

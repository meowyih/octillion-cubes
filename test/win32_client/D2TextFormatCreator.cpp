#include "error/macrolog.hpp"

#include "D2TextFormatCreator.hpp"

D2TextFormatCreator::D2TextFormatCreator()
{
}

D2TextFormatCreator::~D2TextFormatCreator()
{
    fini();
}

IDWriteTextFormat* D2TextFormatCreator::get(IDWriteFactory* factory, int id)
{
    HRESULT hr = E_FAIL;
    IDWriteTextFormat* textformat;
    FLOAT font_height_, line_height_;

    auto it = formats_.find(id);

    if (it != formats_.end())
    {
        // find the brush in store
        return (*it).second;
    }

    switch (id)
    {
    case DEFAULT:
        font_height_ = 20.f;
        line_height_ = 20.f;
        hr = factory->CreateTextFormat(L"Ariel", NULL,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            font_height_,
            L"", //locale name,
            &textformat);
        textformat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, line_height_, font_height_);
        break;
    case SECTION_TEXT_LEADING:
        font_height_ = 25.f;
        line_height_ = 25.f;
        hr = factory->CreateTextFormat(L"Ariel", NULL,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            font_height_,
            L"", //locale name,
            &textformat);
        textformat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, line_height_, font_height_);
        break;
    default:
        LOG_E(tag_) << "get() failed, unknown id " << id;
        break;
    }

    if (SUCCEEDED(hr))
    {
        formats_.insert(std::pair<int, IDWriteTextFormat*>(id, textformat));
        LOG_D(tag_) << "create and insert text format id: " << id;
        return textformat;
    }

    LOG_E(tag_) << "get() failed, CreateTextFormat return  " << hr;
    return NULL;
}

void D2TextFormatCreator::fini()
{
    for (auto it = formats_.begin(); it != formats_.end(); it++)
    {
        it->second->Release();
    }

    formats_.clear();
}

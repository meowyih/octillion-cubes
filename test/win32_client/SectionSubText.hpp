#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <string>

#include "D2BrushCreator.hpp"
#include "D2TextFormatCreator.hpp"

class SectionSubText : public IDWriteTextRenderer
{
private:
    const std::string tag_ = "SectionSubText";

public:
    const static int DRAW_FULL = 0;
    const static int DRAW_VERTICAL_PARTIAL = 1;

public:
    SectionSubText();
    ~SectionSubText();

    bool init(IDWriteFactory* factory, 
        IDWriteTextFormat* lead_format, IDWriteTextFormat* format, 
        FLOAT w, FLOAT h, 
        std::wstring lead, std::wstring str);

    bool init(IDWriteFactory* d2d1_dwfactory, IDWriteTextFormat* format, FLOAT w, FLOAT h, std::wstring str );
    void fini();

    // if option is DRAW_FULL, canvas will not be used
    void draw(
        ID2D1Factory* factory,
        ID2D1HwndRenderTarget* target,
        D2BrushCreator& brushes,
        FLOAT x,
        FLOAT y,
        D2D1_RECT_F canvas,
        int option = DRAW_FULL);

    bool get_rect(D2D1_RECT_F& out);

private:
    bool init_lead(IDWriteFactory* d2d1_dwfactory, IDWriteTextFormat* format, FLOAT w, FLOAT h, std::wstring str);

    void draw(
        ID2D1Factory* factory,
        ID2D1HwndRenderTarget* target,
        D2BrushCreator& brushes,
        IDWriteTextLayout* layout,
        FLOAT x,
        FLOAT y
        );

    void draw_frame(
        ID2D1HwndRenderTarget* target,
        D2BrushCreator& brushes,
        D2D1_RECT_F size);

    bool get_rect(IDWriteTextLayout* layout, D2D1_RECT_F& out);

private:
    FLOAT w_;
    FLOAT h_;
    FLOAT lead_gap_;
    FLOAT lead_margin_;

    IDWriteTextLayout* lead_textlayout_;
    IDWriteTextLayout* textlayout_;

    //
    // IDWriteTextRenderer interface
    //
public:
    IFACEMETHOD(IsPixelSnappingDisabled)(
        __maybenull void* clientDrawingContext,
        __out BOOL* isDisabled
        );

    IFACEMETHOD(GetCurrentTransform)(
        __maybenull void* clientDrawingContext,
        __out DWRITE_MATRIX* transform
        );

    IFACEMETHOD(GetPixelsPerDip)(
        __maybenull void* clientDrawingContext,
        __out FLOAT* pixelsPerDip
        );

    IFACEMETHOD(DrawGlyphRun)(
        __maybenull void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        __in DWRITE_GLYPH_RUN const* glyphRun,
        __in DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawUnderline)(
        __maybenull void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        __in DWRITE_UNDERLINE const* underline,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawStrikethrough)(
        __maybenull void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        __in DWRITE_STRIKETHROUGH const* strikethrough,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawInlineObject)(
        __maybenull void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD_(unsigned long, AddRef) ();
    IFACEMETHOD_(unsigned long, Release) ();
    IFACEMETHOD(QueryInterface) (
        IID const& riid,
        void** ppvObject
        );

//
// IDWriteTextRenderer local & callback variable
//
private:
    unsigned long cRefCount_;

    class RendererBundle
    {
    public:
        ID2D1Geometry* canvas_;
        ID2D1Factory* factory_;
        ID2D1HwndRenderTarget* target_;
        ID2D1Brush* brush_fill_;
        ID2D1Brush* brush_stroke_;
    };

private:
    template <class T> void SafeRelease(T** ppT)
    {
        if (*ppT)
        {
            (*ppT)->Release();
            *ppT = NULL;
        }
    }
};
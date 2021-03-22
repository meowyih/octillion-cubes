#include "error/macrolog.hpp"

#include "SectionSubText.hpp"

SectionSubText::SectionSubText()
{
    cRefCount_ = 0;
    textlayout_ = NULL;
    lead_textlayout_ = NULL;
    w_ = 0;
    h_ = 0;
    lead_gap_ = 10.f;
    lead_margin_ = 10.f;
}

SectionSubText::~SectionSubText()
{
    fini();
}

bool SectionSubText::init(IDWriteFactory* factory,
    IDWriteTextFormat* lead_format, IDWriteTextFormat* format,
    FLOAT w, FLOAT h,
    std::wstring lead, std::wstring str)
{
    D2D1_RECT_F lead_rect;

    if (init_lead(factory, lead_format, w, h, lead) == false)
    {
        LOG_E(tag_) << "init failed, init_lead failed";
        fini();
        return false;
    }

    if (!get_rect(lead_textlayout_, lead_rect))
    {
        LOG_E(tag_) << "init failed, get_rect(lead_textlayout_) failed";
        fini();
        return false;
    }

    if (init(factory, format, w - lead_rect.right - lead_gap_, h, str) == false)
    {
        LOG_E(tag_) << "init failed, init text failed";
        fini();
        return false;
    }

    return true;
}

bool SectionSubText::init_lead(IDWriteFactory* d2d1_dwfactory, IDWriteTextFormat* format, FLOAT w, FLOAT h, std::wstring str)
{
    HRESULT hr = E_FAIL;

    w_ = w;
    h_ = h;

    if (d2d1_dwfactory == NULL || format == NULL)
    {
        LOG_E(tag_) << "init_lead failed, dwork factory or text format is null";
        return false;
    }

    hr = d2d1_dwfactory->CreateTextLayout(str.c_str(), str.length(), format, w, h, &lead_textlayout_);

    if (!SUCCEEDED(hr))
    {
        fini();
        LOG_E(tag_) << "init_lead failed, CreateTextLayout return " << hr;
        return false;
    }

    return true;
}

bool SectionSubText::init(IDWriteFactory* d2d1_dwfactory, IDWriteTextFormat* format, FLOAT w, FLOAT h, std::wstring str)
{
    HRESULT hr = E_FAIL;

    w_ = w;
    h_ = h;

    if (d2d1_dwfactory == NULL || format == NULL)
    {
        LOG_E(tag_) << "init failed, dwork factory or text format is null";
        return false;
    }

    hr = d2d1_dwfactory->CreateTextLayout(str.c_str(), str.length(), format, w, h, &textlayout_);

    if (!SUCCEEDED(hr))
    {
        fini();
        return false;
    }

    return true;
}

void SectionSubText::fini()
{
    if (lead_textlayout_ != NULL )
        lead_textlayout_->Release();

    if (textlayout_ != NULL)
        textlayout_->Release();

    lead_textlayout_ = NULL;
    textlayout_ = NULL;
}

void SectionSubText::draw(
    ID2D1Factory* factory,
    ID2D1HwndRenderTarget* target,
    D2BrushCreator& brushes,
    FLOAT x,
    FLOAT y,
    D2D1_RECT_F canvas,
    int option)
{
    HRESULT hr = S_OK;
    ID2D1Layer* layer = NULL;
    ID2D1RectangleGeometry* mask = NULL;
    D2D1_RECT_F lead_rect, text_rect;

    if (lead_textlayout_ == NULL && textlayout_ == NULL)
    {
        LOG_E(tag_) << "draw() failed, both layouts not init yet";
        return;
    }

    if (lead_textlayout_ != NULL)
    {
        if (get_rect(lead_textlayout_, lead_rect) == false)
        {
            hr = E_FAIL;
        }
    }

    if (textlayout_ != NULL)
    {
        if (get_rect(textlayout_, text_rect) == false)
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        // create layer and set the mask geometry if needed
        hr = target->CreateLayer(NULL, &layer);
    }

    if (SUCCEEDED(hr) && option == DRAW_VERTICAL_PARTIAL)
    {
        hr = factory->CreateRectangleGeometry(canvas, &mask);
    }

    if (SUCCEEDED(hr))
    {
        // Push the layer with the geometric mask.
        target->PushLayer(
            D2D1::LayerParameters(canvas, mask),
            layer
        );
    }

    // no lead text
    if (SUCCEEDED(hr) && lead_textlayout_ == NULL)
    {
        draw(factory, target, brushes, textlayout_, x - text_rect.left, y - text_rect.top);
    }

    // has lead text
    if (SUCCEEDED(hr) && lead_textlayout_ != NULL)
    {
        FLOAT frame_w = lead_rect.right - lead_rect.left + lead_margin_ * 2;
        FLOAT frame_h = lead_rect.bottom - lead_rect.top + lead_margin_ * 2;
        FLOAT lead_txt_x = x - lead_rect.left + lead_margin_;
        FLOAT lead_txt_y = y - lead_rect.top + lead_margin_;
        FLOAT text_x = x - text_rect.left + frame_w + lead_gap_;
        FLOAT text_y = y - text_rect.top;

        // draw lead frame
        draw_frame(target, brushes, D2D1::RectF(x, y, x + frame_w, y + frame_h));

        // draw lead text
        draw(factory, target, brushes, lead_textlayout_, lead_txt_x, lead_txt_y);

        // draw text
        draw(factory, target, brushes, textlayout_, text_x, text_y);
    }

    if (layer != NULL)
    {
        target->PopLayer();
        layer->Release();
    }

    if (mask != NULL)
        mask->Release();
}

bool SectionSubText::get_rect(D2D1_RECT_F& out)
{
    D2D1_RECT_F lead, text;

    if (get_rect(textlayout_, text) == false)
    {
        return false;
    }

    if (lead_textlayout_ == NULL)
    {
        out = D2D1::RectF(
            0.f, 0.f,
            text.right - text.left,
            text.bottom - text.top
        );
        return true;
    }

    if (get_rect(lead_textlayout_, lead) == false)
    {
        return false;
    }

    out = D2D1::RectF(
        0.f, 0.f,
        lead_margin_ * 2 + lead.right - lead.left + lead_gap_ + text.right - text.left,
        ( lead.bottom - lead.top + lead_margin_ * 2 ) > ( text.bottom - text.top ) ?
        ( lead.bottom - lead.top + lead_margin_ * 2 ) : ( text.bottom - text.top)
    );

    return true;
}

void SectionSubText::draw(
    ID2D1Factory* factory, ID2D1HwndRenderTarget* target, D2BrushCreator& brushes, IDWriteTextLayout* layout, 
    FLOAT x, FLOAT y)
{
    ID2D1Brush* brush_stroke = brushes.get(target, D2BrushCreator::SOLID_GENERAL_TEXT);

    if (layout == NULL)
    {
        LOG_E(tag_) << "draw() failed, layout not init yet";
        return;
    }

    target->DrawTextLayout(D2D1::Point2F(x, y), layout, brush_stroke);
    return;
}

bool SectionSubText::get_rect(IDWriteTextLayout* layout, D2D1_RECT_F& out)
{
    HRESULT hr = E_FAIL;
    DWRITE_TEXT_METRICS metrics;
    DWRITE_OVERHANG_METRICS overhang_metrics;

    if (layout == NULL)
    {
        LOG_E(tag_) << "get_rect(), err: layout is null";
        return false;
    }

    // note: negative value in overhang indicates the white space inside 
    //       the text layout, so the real bottom is the layout height
    //       minus the white space in overhang.bottom
    hr = layout->GetOverhangMetrics(&overhang_metrics);
    if (SUCCEEDED(hr))
    {
        hr = layout->GetMetrics(&metrics);
    }

    if (SUCCEEDED(hr))
    {
        out = D2D1::RectF(
            metrics.left - overhang_metrics.left,
            metrics.top - overhang_metrics.top,
            metrics.layoutWidth + overhang_metrics.right,
            metrics.layoutHeight + overhang_metrics.bottom);
        return true;
    }

    LOG_E(tag_) << "get_rect(), GetMetrics() GetOverhangMetrics() return " << hr;

    return false;
}

void SectionSubText::draw_frame(
    ID2D1HwndRenderTarget* target,
    D2BrushCreator& brushes,
    D2D1_RECT_F size)
{
    FLOAT text_margin = 5.f;
    FLOAT margin = lead_margin_ > text_margin ? (lead_margin_ - text_margin) : 0.f;
    D2D1_RECT_F frame = D2D1::RectF(size.left + margin, size.top + margin, size.right - margin, size.bottom - margin);
    FLOAT stroke_width = 1.f;

    target->DrawRectangle(frame, brushes.get(target, D2BrushCreator::SOLID_GENERAL_TEXT), stroke_width);
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::DrawGlyphRun                               *
*                                                                 *
*  Gets GlyphRun outlines via IDWriteFontFace::GetGlyphRunOutline *
*  and then draws and fills them using Direct2D path geometries   *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::DrawGlyphRun(
    __maybenull void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    __in DWRITE_GLYPH_RUN const* glyphRun,
    __in DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect
)
{
    HRESULT hr = S_OK;

    // get the parameters from bundle
    RendererBundle* bundle = static_cast<RendererBundle*>(clientDrawingContext);

    // sink for everyone
    ID2D1GeometrySink* pSink = NULL;

    // text path geometry, transform geometry and the final geometry
    ID2D1PathGeometry* pPathCombine = NULL;
    ID2D1PathGeometry* pPathText = NULL;
    ID2D1TransformedGeometry* pTransformedGeometry = NULL;

    // Initialize a matrix to translate the origin of the glyph run.
    D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
        1.0f, 0.0f,
        0.0f, 1.0f,
        baselineOriginX, baselineOriginY
    );

    if (bundle == NULL)
        return E_FAIL;

    // Create the path geometry.
    hr = bundle->factory_->CreatePathGeometry(&pPathText);
    if (SUCCEEDED(hr))
    {
        hr = pPathText->Open(&pSink);
    }

    if (SUCCEEDED(hr))
    {
        hr = glyphRun->fontFace->GetGlyphRunOutline(
            glyphRun->fontEmSize,
            glyphRun->glyphIndices,
            glyphRun->glyphAdvances,
            glyphRun->glyphOffsets,
            glyphRun->glyphCount,
            glyphRun->isSideways,
            glyphRun->bidiLevel % 2,
            pSink
        );
    }

    if (SUCCEEDED(hr))
    {
        hr = pSink->Close();
        SafeRelease(&pSink);
    }

    // Create the transformed geometry
    if (SUCCEEDED(hr))
    {
        hr = bundle->factory_->CreateTransformedGeometry(
            pPathText,
            &matrix,
            &pTransformedGeometry
        );
    }

    // combine pTransformedGeometry and canvas geometry
    if (SUCCEEDED(hr))
    {
        hr = bundle->factory_->CreatePathGeometry(&pPathCombine);
    }

    if (SUCCEEDED(hr))
    {
        hr = pPathCombine->Open(&pSink);
    }

    if (SUCCEEDED(hr))
    {
        hr = pTransformedGeometry->CombineWithGeometry(
            bundle->canvas_,
            D2D1_COMBINE_MODE_INTERSECT,
            NULL,
            NULL,
            pSink);
    }

    if (SUCCEEDED(hr))
    {
        hr = pSink->Close();
        SafeRelease(&pSink);
    }

    // Fill in the glyph run
    if (SUCCEEDED(hr))
    {
        bundle->target_->FillGeometry(
            pPathCombine,
            bundle->brush_stroke_
        );
    }

    SafeRelease(&pSink);
    SafeRelease(&pPathText);
    SafeRelease(&pPathCombine);
    SafeRelease(&pTransformedGeometry);

    return hr;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::DrawUnderline                              *
*                                                                 *
*  Draws underlines below the text using a Direct2D recatangle    *
*  geometry                                                       *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::DrawUnderline(
    __maybenull void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    __in DWRITE_UNDERLINE const* underline,
    IUnknown* clientDrawingEffect
)
{
    HRESULT hr;

    // get the parameters from bundle
    RendererBundle* bundle = static_cast<RendererBundle*>(clientDrawingContext);

    D2D1_RECT_F rect = D2D1::RectF(
        0,
        underline->offset,
        underline->width,
        underline->offset + underline->thickness
    );

    if (bundle == NULL)
        return E_FAIL;

    ID2D1RectangleGeometry* pRectangleGeometry = NULL;
    hr = bundle->factory_->CreateRectangleGeometry(
        &rect,
        &pRectangleGeometry
    );

    // Initialize a matrix to translate the origin of the underline
    D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
        1.0f, 0.0f,
        0.0f, 1.0f,
        baselineOriginX, baselineOriginY
    );

    ID2D1TransformedGeometry* pTransformedGeometry = NULL;
    if (SUCCEEDED(hr))
    {
        hr = bundle->factory_->CreateTransformedGeometry(
            pRectangleGeometry,
            &matrix,
            &pTransformedGeometry
        );
    }

    // Draw the outline of the rectangle
    bundle->target_->DrawGeometry(
        pTransformedGeometry,
        bundle->brush_stroke_
    );

    // Fill in the rectangle
    bundle->target_->FillGeometry(
        pTransformedGeometry,
        bundle->brush_fill_
    );

    SafeRelease(&pRectangleGeometry);
    SafeRelease(&pTransformedGeometry);

    return S_OK;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::DrawStrikethrough                          *
*                                                                 *
*  Draws strikethroughs below the text using a Direct2D           *
*  recatangle geometry                                            *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::DrawStrikethrough(
    __maybenull void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    __in DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown* clientDrawingEffect
)
{
    HRESULT hr;

    // get the parameters from bundle
    RendererBundle* bundle = static_cast<RendererBundle*>(clientDrawingContext);

    if (bundle == NULL)
        return E_FAIL;

    D2D1_RECT_F rect = D2D1::RectF(
        0,
        strikethrough->offset,
        strikethrough->width,
        strikethrough->offset + strikethrough->thickness
    );

    ID2D1RectangleGeometry* pRectangleGeometry = NULL;
    hr = bundle->factory_->CreateRectangleGeometry(
        &rect,
        &pRectangleGeometry
    );

    // Initialize a matrix to translate the origin of the strikethrough
    D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
        1.0f, 0.0f,
        0.0f, 1.0f,
        baselineOriginX, baselineOriginY
    );

    ID2D1TransformedGeometry* pTransformedGeometry = NULL;
    if (SUCCEEDED(hr))
    {
        hr = bundle->factory_->CreateTransformedGeometry(
            pRectangleGeometry,
            &matrix,
            &pTransformedGeometry
        );
    }

    // Draw the outline of the rectangle
    bundle->target_->DrawGeometry(
        pTransformedGeometry,
        bundle->brush_stroke_
    );

    // Fill in the rectangle
    bundle->target_->FillGeometry(
        pTransformedGeometry,
        bundle->brush_fill_
    );

    SafeRelease(&pRectangleGeometry);
    SafeRelease(&pTransformedGeometry);

    return S_OK;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::DrawInlineObject                           *
*                                                                 *
*  This function is not implemented for the purposes of this      *
*  sample.                                                        *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::DrawInlineObject(
    __maybenull void* clientDrawingContext,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject* inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    IUnknown* clientDrawingEffect
)
{
    // Not implemented
    return E_NOTIMPL;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::AddRef                                     *
*                                                                 *
*  Increments the ref count                                       *
*                                                                 *
******************************************************************/
IFACEMETHODIMP_(unsigned long) SectionSubText::AddRef()
{
    // we will use this for auto recycle
    return InterlockedIncrement(&cRefCount_);
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::Release                                    *
*                                                                 *
*  Decrements the ref count and deletes the instance if the ref   *
*  count becomes 0                                                *
*                                                                 *
******************************************************************/
IFACEMETHODIMP_(unsigned long) SectionSubText::Release()
{
    unsigned long newCount = InterlockedDecrement(&cRefCount_);
    if (newCount == 0)
    {
        // delete this; // we will use this for auto recycle
        return 0;
    }

    return newCount;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::IsPixelSnappingDisabled                    *
*                                                                 *
*  Determines whether pixel snapping is disabled. The recommended *
*  default is FALSE, unless doing animation that requires         *
*  subpixel vertical placement.                                   *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::IsPixelSnappingDisabled(
    __maybenull void* clientDrawingContext,
    __out BOOL* isDisabled
)
{
    *isDisabled = FALSE;
    return S_OK;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::GetCurrentTransform                        *
*                                                                 *
*  Returns the current transform applied to the render target..   *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::GetCurrentTransform(
    __maybenull void* clientDrawingContext,
    __out DWRITE_MATRIX* transform
)
{
    // get the parameters from bundle
    RendererBundle* bundle = static_cast<RendererBundle*>(clientDrawingContext);

    if (bundle == NULL)
        return E_FAIL;

    //forward the render target's transform
    bundle->target_->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
    return S_OK;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::GetPixelsPerDip                            *
*                                                                 *
*  This returns the number of pixels per DIP.                     *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::GetPixelsPerDip(
    __maybenull void* clientDrawingContext,
    __out FLOAT* pixelsPerDip
)
{
    *pixelsPerDip = 1.0f;
    return S_OK;
}

/******************************************************************
*                                                                 *
*  CustomTextRenderer::QueryInterface                             *
*                                                                 *
*  Query interface implementation                                 *
*                                                                 *
******************************************************************/

IFACEMETHODIMP SectionSubText::QueryInterface(
    IID const& riid,
    void** ppvObject
)
{
    if (__uuidof(IDWriteTextRenderer) == riid)
    {
        *ppvObject = this;
    }
    else if (__uuidof(IDWritePixelSnapping) == riid)
    {
        *ppvObject = this;
    }
    else if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = this;
    }
    else
    {
        *ppvObject = NULL;
        return E_FAIL;
    }

    this->AddRef();

    return S_OK;
}
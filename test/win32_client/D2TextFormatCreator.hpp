#pragma once

#include <string>
#include <map>
#include <memory>

#include <d2d1.h>
#include <dwrite.h>

class D2TextFormatCreator
{
public:
    const static int DEFAULT = 1;
    const static int SECTION_TEXT_LEADING = 2;

private:
    const std::string tag_ = "D2TextFormatCreator";

public:
    D2TextFormatCreator();
    ~D2TextFormatCreator();

public:
    // avoid accidentally copy
    D2TextFormatCreator(D2TextFormatCreator const&) = delete;
    void operator = (D2TextFormatCreator const&) = delete;

public:
    IDWriteTextFormat* get(IDWriteFactory* factory, int id);
    void fini();

private:
    std::map<int, IDWriteTextFormat*> formats_;
};


#pragma once

#include <memory>
#include <vector>
#include <string>
#include "world/interactive.hpp"

class ListBitmapItem
{
public:
    int x_ = 0;
    int y_ = 0;
    int h_ = 0;
    int w_ = 0;
    std::shared_ptr<octillion::Interactive>  interactive_;
};

class ListBitmapCreator
{
public:
    const std::string tag_ = "ListBitmapCreator";
public:
    ListBitmapCreator();

    void set(int w, int h, std::vector<std::shared_ptr<octillion::Interactive>>& data );

    std::shared_ptr<std::vector<BYTE>> render(HDC hdc);

    std::shared_ptr<octillion::Interactive> point(int x, int y);

private:
    int w_, h_;
    bool is_new_data_;
    std::vector<std::shared_ptr<octillion::Interactive>> data_;
    std::vector< ListBitmapItem> pos_;
    std::shared_ptr<std::vector<BYTE>> bitmap_;
};


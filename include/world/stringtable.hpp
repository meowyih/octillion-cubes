#ifndef OCTILLION_STRINGTABLE_HEADER
#define OCTILLION_STRINGTABLE_HEADER

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <memory>

#include "jsonw/jsonw.hpp"

namespace octillion
{
    class StringData;
    class StringTable;
}

class octillion::StringData
{
public:
    int id_;
    std::string str_;
    std::wstring wstr_;
    std::unordered_set<int> attrs_; 
};

class octillion::StringTable
{
private:
    const std::string tag_ = "WorldMap";

public:
    StringTable();
    bool add(std::shared_ptr<JsonW> json);
    std::shared_ptr<octillion::StringData> find(int id);

private:
    std::unordered_map<int, std::shared_ptr<octillion::StringData>> data_;
};

#endif
#ifndef OCTILLION_STRINGTABLE_HEADER
#define OCTILLION_STRINGTABLE_HEADER

#include <vector>
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
    int id_ = 0;
    std::vector<std::string> str_;
    std::vector<std::wstring> wstr_;
    int select_ = 0;
    int last_index_ = 0;
};

class octillion::StringTable
{
public:
    const static int SELECT_ALL = 0; // default value, no need to add
    const static int SELECT_RANDOMLY = 1;
    const static int SELECT_ORDERLY = 2;
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
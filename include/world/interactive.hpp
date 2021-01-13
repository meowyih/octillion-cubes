#ifndef OCTILLION_INTERACTIVE_HEADER
#define OCTILLION_INTERACTIVE_HEADER

#include "world/stringtable.hpp"
#include "world/script.hpp"
#include "world/cubeposition.hpp"

namespace octillion
{
    class Interactive;
}

class octillion::Interactive
{
public:
    const std::string tag_ = "Interactive";

public:
    Interactive();

    bool init(
        int areaid, 
        std::shared_ptr<JsonW> json, 
        std::map<std::string, octillion::CubePosition>& markmap, 
        uint_fast32_t offset_x,
        uint_fast32_t offset_y,
        uint_fast32_t offset_z,
        octillion::StringTable& table);

public:
    std::shared_ptr<octillion::StringData> title();
    octillion::CubePosition loc();
    int area();
    int id();

private:
    std::shared_ptr<octillion::StringData> title_;
    std::vector<octillion::Script> scripts_;
    octillion::CubePosition loc_;
    int area_id_ = 0;
    int obj_id_ = 0;
};

#endif
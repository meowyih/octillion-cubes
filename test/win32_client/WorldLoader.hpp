#pragma once

#include <memory>
#include <thread>
#include <string>

#include "world/worldmap.hpp"

#include "BaseLoader.hpp"

using std::shared_ptr;
using std::string;

using octillion::WorldMap;

class WorldLoader : public BaseLoader
{
private:
    const string tag_ = "WorldLoader";

public:
    WorldLoader();
    ~WorldLoader();

    void init(shared_ptr<WorldMap> worldmap, string filename);

private:
    void core();

private:
    shared_ptr<WorldMap> worldmap_;
    string filename_;
};

#pragma once

#include <memory>
#include <thread>
#include <string>

#include <d2d1.h>

#include "BaseLoader.hpp"

#include "world/worldmap.hpp"

using octillion::WorldMap;

using std::unique_ptr;
using std::shared_ptr;
using std::string;
using std::thread;

using octillion::Cube;

class MapFrameLoader : public BaseLoader
{
private:
    const string tag_ = "MapFrameLoader";

public:
    MapFrameLoader();
    ~MapFrameLoader();

    void init(
        ID2D1Factory* factory,
        ID2D1GeometryGroup** geogroup,
        shared_ptr<Cube> cube);

private:
    void core();

private:
    ID2D1Factory* factory_;
    ID2D1GeometryGroup** geogroup_;
    shared_ptr<Cube> cube_;
};


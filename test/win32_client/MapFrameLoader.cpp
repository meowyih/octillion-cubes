#include "MapFrameLoader.hpp"
#include "MapFrame.hpp"

#include "error/macrolog.hpp"

using std::make_unique;

MapFrameLoader::MapFrameLoader()
{
    factory_ = NULL;
    geogroup_ = NULL;
    cube_ = nullptr;
}

MapFrameLoader::~MapFrameLoader()
{
}

void MapFrameLoader::init(ID2D1Factory* factory, ID2D1GeometryGroup** geogroup, shared_ptr<Cube> cube)
{
    factory_ = factory;
    geogroup_ = geogroup;
    cube_ = cube;

    if (factory_ == NULL)
    {
        state_ = STATE_FAILED;
        return;
    }

    if (geogroup_ == NULL)
    {
        state_ = STATE_FAILED;
        return;
    }

    // previous job is still running
    if (state_ == STATE_LOADING)
    {
        state_ = STATE_FAILED;
        return;
    }

    // start the thread
    state_ = STATE_LOADING;
    pthread_ = make_unique<thread>(&MapFrameLoader::core, this);
}

void MapFrameLoader::core()
{
    bool result;

    LOG_D(tag_) << "core, enter";

    if (factory_ == nullptr)
    {
        LOG_E(tag_) << "core, error: worldmap is null";
        state_ = STATE_FAILED;
        return;
    }

    LOG_D(tag_) << "core, render map";
    result = MapFrame::render_map(factory_, geogroup_, cube_);

    if (result)
    {
        state_ = STATE_SUCCEEDED;
        LOG_D(tag_) << "core, leave with STATE_SUCCEEDED";
    }
    else
    {
        state_ = STATE_FAILED;
        LOG_D(tag_) << "core, leave with STATE_FAILED";
    }

    return;
}

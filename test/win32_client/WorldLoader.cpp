#include "WorldLoader.hpp"

#include "error/macrolog.hpp"

using std::make_unique;

WorldLoader::WorldLoader()
{
}

WorldLoader::~WorldLoader()
{
}

void WorldLoader::init(shared_ptr<WorldMap> worldmap, string filename)
{
    worldmap_ = worldmap;
    filename_ = filename;

    if (worldmap_ == nullptr)
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
    pthread_ = make_unique<thread>(&WorldLoader::core, this);
}

void WorldLoader::core()
{
    bool result;

    LOG_D(tag_) << "core, enter";

    if (worldmap_ == nullptr)
    {
        LOG_E(tag_) << "core, error: worldmap is null";
        state_ = STATE_FAILED;
        return;
    }

    LOG_D(tag_) << "core, load json map";
    result = worldmap_->load_external_data_file(filename_);

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

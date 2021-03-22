#include "DataLoader.hpp"

#include "error/macrolog.hpp"

using std::make_unique;

DataLoader::DataLoader()
{
    state_ = STATE_IDLE;
}

DataLoader::~DataLoader()
{
    if (pthread_ != nullptr)
    {
        pthread_->detach();
        pthread_ = nullptr;
    }
}

bool DataLoader::is_idle()
{
    if (state_ == STATE_LOADING)
        return false;

    return true;
}

int DataLoader::state()
{
    return state_;
}

void DataLoader::init(shared_ptr<WorldMap> worldmap, string filename)
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
    pthread_ = make_unique<thread>(&DataLoader::core, this);
}

void DataLoader::core()
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
    }
    else
    {
        state_ = STATE_FAILED;
    }

    LOG_D(tag_) << "core, leave with state: " << state_;

    return;
}

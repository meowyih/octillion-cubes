#include "BaseLoader.hpp"

BaseLoader::BaseLoader()
{
    state_ = STATE_IDLE;
}

BaseLoader::~BaseLoader()
{
    if (pthread_ != nullptr)
    {
        pthread_->detach();
        pthread_ = nullptr;
    }
}

bool BaseLoader::is_idle()
{
    if (state_ == STATE_LOADING)
        return false;

    return true;
}

int BaseLoader::state()
{
    return state_;
}
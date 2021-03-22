#include "CubeContainer.hpp"

CubeContainer::CubeContainer(std::shared_ptr<octillion::Cube> cube)
{
    data_ = cube;
}

CubeContainer::~CubeContainer()
{
    objects_.clear();
    listeners_.clear();
}

void CubeContainer::dispatch(int type, std::shared_ptr<InteractiveContainer> source)
{
    for (auto it = listeners_.begin(); it != listeners_.end(); it++)
    {
        if (*it != source)
        {
            (*it)->recv(type, source);
        }
    }

    for (auto it = objects_.begin(); it != objects_.end(); it++)
    {
        if (*it != source)
        {
            (*it)->recv(type, source);
        }
    }
}

void CubeContainer::add(std::shared_ptr<InteractiveContainer> object)
{
    objects_.insert(object);
}

void CubeContainer::remove_all()
{
    objects_.clear();
}

void CubeContainer::remove(std::shared_ptr<InteractiveContainer> object)
{
    objects_.erase(object);
    return;
}

void CubeContainer::reg(std::shared_ptr<InteractiveContainer> object)
{
    listeners_.insert(object);
}

void CubeContainer::unreg_all()
{
    listeners_.clear();
}

void CubeContainer::unreg(std::shared_ptr<InteractiveContainer> object)
{
    listeners_.erase(object);
}

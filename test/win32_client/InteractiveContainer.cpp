#include "InteractiveContainer.hpp"

InteractiveContainer::InteractiveContainer(std::shared_ptr<octillion::Interactive> interactive)
{
    data_ = interactive;
}

InteractiveContainer::~InteractiveContainer()
{
}

void InteractiveContainer::recv(int type, std::shared_ptr<InteractiveContainer> source)
{
}

#pragma once

#include <memory>

#include "world/interactive.hpp"

class InteractiveContainer
{
public:
    InteractiveContainer(std::shared_ptr<octillion::Interactive> interactive);
    ~InteractiveContainer();

    void recv( int type, std::shared_ptr<InteractiveContainer> source);

public:
    std::shared_ptr<octillion::Interactive> data_;
};


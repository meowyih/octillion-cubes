#pragma once

#include <memory>
#include <set>

#include "world/cube.hpp"

#include "InteractiveContainer.hpp"

class CubeContainer
{
public:
    const static int EVENT_DELETE = 1;

public:
    CubeContainer(std::shared_ptr<octillion::Cube> cube);
    ~CubeContainer();

    // event
    void dispatch(int type, std::shared_ptr<InteractiveContainer> source);

    // add/remove interactive 
    void add(std::shared_ptr<InteractiveContainer> object);
    void remove_all();
    void remove(std::shared_ptr<InteractiveContainer> object);

    void reg(std::shared_ptr<InteractiveContainer> object);
    void unreg_all();
    void unreg(std::shared_ptr<InteractiveContainer> object);

public:
    std::shared_ptr<octillion::Cube> data_;
    std::set<std::shared_ptr<InteractiveContainer>> objects_;
    std::set<std::shared_ptr<InteractiveContainer>> listeners_;
};


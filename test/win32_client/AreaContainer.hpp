#pragma once

#include <map>

#include "world/cube.hpp"
#include "error/macrolog.hpp"

#include "CubeContainer.hpp"
#include "InteractiveContainer.hpp"

class AreaContainer
{
private:
    const std::string tag_ = "AreaContainer";

public:
    AreaContainer();
    ~AreaContainer();

    void set(std::vector<std::shared_ptr<octillion::Area>> areas);
    void insert(std::shared_ptr<octillion::Area> area);

    // we assume interactive object won't go to other area
    void delete_by_areaid(int areaid);

private:
    void clear_cube(std::shared_ptr<CubeContainer> cube);

public:
    std::map<
        int, 
        std::map<octillion::CubePosition, std::shared_ptr<CubeContainer>>
    > cubes_;

    std::map<
        int,
        std::set<std::shared_ptr<InteractiveContainer>>
    > interactives_;
};


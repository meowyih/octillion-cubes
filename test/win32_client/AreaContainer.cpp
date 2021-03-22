#include "AreaContainer.hpp"

AreaContainer::AreaContainer()
{
}

AreaContainer::~AreaContainer()
{
}

void AreaContainer::set(std::vector<std::shared_ptr<octillion::Area>> areas)
{
    std::vector<int> unused_area;

    // remove the cube that does not need to exist anymore
    for (auto it = cubes_.begin(); it != cubes_.end(); it++)
    {
        bool exist = false;
        int areaid_in_cubes = (*it).first;
        for (size_t i = 0; i < areas.size(); i++)
        {
            if (areaid_in_cubes == areas.at(i)->id())
            {
                exist = true;
                break;
            }
        }

        if (!exist)
        {
            unused_area.push_back(areaid_in_cubes);
        }
    }

    if (unused_area.size() == 0)
    {
        LOG_D(tag_) << "set(), no need to delete old area data";
    }

    for (auto it = unused_area.begin(); it != unused_area.end(); it++)
    {
        LOG_D(tag_) << "set(), delete old area data:" << *it;
        delete_by_areaid(*it);
    }

    // insert area data into cubes_ if needed
    for (auto it = areas.begin(); it != areas.end(); it++)
    {
        int areaid = (*it)->id();

        if (cubes_.find(areaid) != cubes_.end())
        {
            // already there
            LOG_D(tag_) << "set(), area:" << areaid << " already exist, skip it";
            continue;
        }

        // insert data 
        LOG_D(tag_) << "set(), insert area:" << areaid;
        insert((*it));
    }
}

void AreaContainer::insert(std::shared_ptr<octillion::Area> area)
{
    // create a new space in cubes_ and interactives_ based on area id
    cubes_.insert
        (std::pair<int, std::map<octillion::CubePosition, std::shared_ptr<CubeContainer>>>
            (
            area->id(), 
            std::map<octillion::CubePosition, std::shared_ptr<CubeContainer>>()
            )
        );

    interactives_.insert
        (std::pair<int, std::set<std::shared_ptr<InteractiveContainer>>>
            (
            area->id(),
            std::set<std::shared_ptr<InteractiveContainer>>()
            )
        );

    auto cube_list = cubes_.find(area->id());
    auto interactive_list = interactives_.find(area->id());

    if (cube_list == cubes_.end() || interactive_list == interactives_.end())
    {
        LOG_E(tag_) << "Fatal error in insert(), failed to create cube list or interactive list";
        return;
    }

    // create and insert cube container into that new space
    for (auto it = area->cubes_.begin(); it != area->cubes_.end(); it++)
    {
        std::shared_ptr<CubeContainer> cube_container =
            std::make_shared<CubeContainer>(it->second);

        cube_list->second.insert(
            std::pair<octillion::CubePosition, std::shared_ptr<CubeContainer>>
            (
                it->second->loc(),
                cube_container
            )
        );
    }

    // create interactive and put into cube container
    for (auto it = area->interactives_.begin(); it != area->interactives_.end(); it++)
    {
        octillion::CubePosition pos = (*it)->loc();
        std::shared_ptr<InteractiveContainer> inter_container =
            std::make_shared<InteractiveContainer>(*it);

        auto cube_for_pos = cube_list->second.find(pos);

        if (cube_for_pos != cube_list->second.end())
        {
            cube_for_pos->second->add(inter_container);
            interactive_list->second.insert(inter_container);
        }
    }
}

void AreaContainer::delete_by_areaid(int areaid)
{
    auto map_it = cubes_.find(areaid);

    if (map_it == cubes_.end())
        return;

    for (auto it = map_it->second.begin(); it != map_it->second.end(); it++)
    {
        clear_cube(it->second);
    }

    cubes_.erase(areaid);
    interactives_.erase(areaid);
}

void AreaContainer::clear_cube(std::shared_ptr<CubeContainer> cube)
{
    // let all the monitorer knows all interactives in cube are disappear
    for (auto it = cube->objects_.begin(); it != cube->objects_.end(); it++)
    {
        cube->dispatch( CubeContainer::EVENT_DELETE, *it );
    }
}

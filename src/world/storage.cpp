#include "world/storage.hpp"

octillion::Storage::Storage()
{
    areaid_ = 0;
    integers_.insert(std::pair<unsigned int, uint_fast32_t>(1, 0));
}

bool octillion::Storage::boolean(unsigned int area_id, unsigned int index)
{
    return false;
}

void octillion::Storage::boolean_set(unsigned int area_id, unsigned int index, bool value)
{
}

int octillion::Storage::integer(unsigned int area_id, unsigned int index)
{
    if (index == 1)
    {
        return integers_.at(index);
    }
    return 0;
}

void octillion::Storage::integer_set(unsigned int area_id, unsigned int index, int value)
{
    if (index == 1)
    {
        integers_[index] = value;
    }
}

unsigned int octillion::Storage::move_count_inc(unsigned int area_id)
{
    auto it = move_count_.find(area_id);
    if (it == move_count_.end())
    {
        move_count_.insert(std::pair<unsigned int, unsigned int>(area_id, 1));
        return 1;
    }

    (*it).second++;
    return (*it).second;
}

unsigned int octillion::Storage::move_count(unsigned int area_id)
{
    auto it = move_count_.find(area_id);
    if (it == move_count_.end())
    {       
        return 0;
    }
;
    return (*it).second;
}

void octillion::Storage::reset_timer()
{
    timer_ = std::chrono::high_resolution_clock::now();
}

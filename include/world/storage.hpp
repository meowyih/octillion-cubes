#ifndef OCTILLION_STORAGE_HEADER
#define OCTILLION_STORAGE_HEADER

#include <cstdint>
#include <chrono>
#include <unordered_map>

#include "world/cubeposition.hpp"

namespace octillion
{
    class Storage;
}

class octillion::Storage
{
public:
    Storage();

    bool boolean(unsigned int area_id, unsigned int index);
    void boolean_set(unsigned int area_id, unsigned int index, bool value);

    int integer(unsigned int area_id, unsigned int index);
    void integer_set(unsigned int area_id, unsigned int index, int value);

    unsigned int move_count_inc(unsigned int area_id);
    unsigned int move_count(unsigned int area_id);

    void reset_timer();

public:
    unsigned int areaid_;
    CubePosition loc_;
    std::unordered_map<unsigned int, uint_fast32_t> booleans_;
    std::unordered_map<unsigned int, uint_fast32_t> integers_;
    std::unordered_map<unsigned int, uint_fast32_t> move_count_;

    std::chrono::time_point<std::chrono::high_resolution_clock> timer_;
};

#endif
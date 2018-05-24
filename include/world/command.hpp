#ifndef OCTILLION_COMMAND_HEADER
#define OCTILLION_COMMAND_HEADER

#include "world/cube.hpp"

namespace octillion
{
    class Command;
}

class octillion::Command
{
public:
    uint32_t pcid_;
    uint32_t cmd_;
    CubePosition loc_;
};

#endif

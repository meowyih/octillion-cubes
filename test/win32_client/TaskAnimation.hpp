#ifndef HEADER_TASK_ANIMATION
#define HEADER_TASK_ANIMATION

#include <chrono>
#include <memory>

#include "world/cube.hpp"
#include "Cube3d.hpp"

class TaskAnimation
{
public:
    const static int MOVE_2D = 1;
    const static int ROTATE = 2;
    const static int ROTATE_RESET = 3;

    const static int ANI_WAITING = 0;
    const static int ANI_RUNNING = 1;
public:
    TaskAnimation()
    {
        ms_start_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }

public:
    int type_ = 0;
    int status_ = ANI_WAITING;

    int interval_x_ = 0, interval_y_ = 0;
    int dest_degree_x_ = 0, dest_degree_y_ = 0, dest_degree_z_ = 0;
    int start_degree_x_ = 0, start_degree_y_ = 0, start_degree_z_ = 0;
    double start_scale_ = 0, dest_scale_ = 0;
    std::chrono::milliseconds ms_start_;
    long long duration_ = 0;

    int start_x = 0, start_y = 0, start_z = 0;

    std::shared_ptr<octillion::Cube> cube_dest_ = nullptr;
};

#endif
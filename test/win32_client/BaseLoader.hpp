#pragma once

#include <memory>
#include <thread>

using std::unique_ptr;
using std::thread;

class BaseLoader
{
public:
    const static int STATE_IDLE = 1;
    const static int STATE_LOADING = 2;
    const static int STATE_SUCCEEDED = 3;
    const static int STATE_FAILED = 4;

public:
    BaseLoader();
    ~BaseLoader();

    bool is_idle();
    int state();

protected:
    unique_ptr<thread> pthread_;
    int state_;
};


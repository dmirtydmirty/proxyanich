#pragma once
#include <pthread.h>
#include <atomic>

struct thread
{
    int start();
    int stop();

    virtual void function() = 0;

    static void* function_wrapper(void* arg);

protected:
    std::atomic_bool stop_flag = true;
    pthread_t th = 0;
};

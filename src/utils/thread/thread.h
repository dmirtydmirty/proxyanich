#pragma once
#include <pthread.h>
#include <atomic>

struct thread
{
    int start();
    int stop();

    static void* function(void*);

private:
    std::atomic_bool stop_flag = true;
    pthread_t th = 0;
};

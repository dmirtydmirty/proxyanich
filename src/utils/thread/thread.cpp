#include "thread.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


int thread::start()
{
    if (int ret = pthread_create(&th, nullptr, function_wrapper, this); ret != 0) {
        spdlog::critical("thread::start(): pthread_create error: {}!", strerror(ret));
        return -1;
    }
    stop_flag = false;
    spdlog::debug("thread::start(): thread successfully started");
    return 0; 
}

int thread::stop()
{
    stop_flag = true;
    pthread_join(th, nullptr);
    return 0;
}

void* thread::function_wrapper(void* arg) 
{
    auto* self = (thread*)arg;
    self->function();
    return nullptr;
}

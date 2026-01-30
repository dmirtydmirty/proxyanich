#pragma once
#include <boost/lockfree/spsc_queue.hpp>

#include "itc/itc.h"
#include "thread/thread.h"

struct dp_thread : thread
{
    static void* function(void*);
};


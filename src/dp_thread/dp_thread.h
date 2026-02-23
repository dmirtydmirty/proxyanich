#pragma once

#include "itc/itc.h"
#include "thread/thread.h"
#include "event_loop/event_loop.h"

struct dp_thread : thread
{
    void function(void);
    event_loop el;
};


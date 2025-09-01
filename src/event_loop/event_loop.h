#pragma once
#include <liburing.h>


class event_loop {

    io_uring m_ring;

    int init(size_t max_client_cnt);

    int start();

    int stop();


};
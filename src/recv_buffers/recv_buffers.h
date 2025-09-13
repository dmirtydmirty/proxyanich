#pragma once
#include <cstddef>
#include <cstdint>

struct recv_buffers_t {
    
    int alloc(size_t bufs_len, size_t bufs_cnt);
    uint8_t* operator[](size_t buf_idx);
    ~recv_buffers_t();

    uint8_t* m_bufs{nullptr};
    size_t m_bufs_len{0};
    size_t m_bufs_cnt{0};
};
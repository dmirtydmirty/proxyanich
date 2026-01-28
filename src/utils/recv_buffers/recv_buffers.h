#pragma once
#include <cstddef>
#include <cstdint>

struct recv_buffers_t {
    
    int alloc(size_t bufs_len, size_t bufs_cnt);
    char* operator[](size_t buf_idx);
    ~recv_buffers_t();

    char* m_bufs{nullptr};
    size_t m_bufs_len{0};
    size_t m_bufs_cnt{0};
};
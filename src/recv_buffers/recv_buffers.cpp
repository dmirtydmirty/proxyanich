#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "recv_buffers.h"

int recv_buffers_t::alloc(size_t bufs_len, size_t bufs_cnt)
{    
    if ( int ret = posix_memalign((void**)&m_bufs, 64, bufs_len * bufs_cnt); ret != 0) {
        fprintf(stderr, "recv_buffers_t::alloc posix_memalign m_buffs error, %s\n", strerror(ret));
        return -1;
    }

    m_bufs_len = bufs_len;
    m_bufs_cnt = bufs_cnt;

    return 0;
}

uint8_t* recv_buffers_t::operator[](size_t buf_idx)
{
    return m_bufs + buf_idx * m_bufs_len;
}

recv_buffers_t::~recv_buffers_t()
{
    free(m_bufs);
}

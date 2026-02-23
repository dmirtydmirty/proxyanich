#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>

#include "dp_thread.h"

extern boost::lockfree::spsc_queue<itc_message, boost::lockfree::capacity<10>> DP_thread_queue;

enum class handle_tls_stage {
    handle_header,
    handle_payload,
};

std::unordered_map<int, int> fwd_rules;

void print_msg_hex(event_object& ev) {

    size_t bid = ev.flags >> IORING_CQE_BUFFER_SHIFT;
    if (ev.rv > 0) {
        size_t bytes_recv = ev.rv;
        auto bytes = std::string_view(ev.el->buffs[bid], bytes_recv);  
    
        std::cout << "Hex: ";
        for (ssize_t i = 0; i < bytes_recv; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<uint16_t>(bytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    }

    ev.el->free_buf(bid);
} 

void handle_tls(event_object& ev) {
    size_t bid = ev.flags >> IORING_CQE_BUFFER_SHIFT;
    auto stage = (handle_tls_stage)ev.u64;
    switch (stage)
    {
    case handle_tls_stage::handle_header:
        {

            break;
        }
    case handle_tls_stage::handle_payload:
        {

            break;
        }
    default:
        break;
    }

}

void free_buff(event_object& ev) {
    spdlog::debug("send {} bytes of date complete, free bufer", ev.rv);
    ev.el->free_buf(ev.bid);
}

void handle_data(event_object& ev) {
    size_t bid = ev.flags >> IORING_CQE_BUFFER_SHIFT;
    auto bytes_recv = ev.rv;
    if (bytes_recv > 0) {
        int from_fd = ev.fd;
        int to_fd = fwd_rules[from_fd];
        spdlog::debug("forwarding {} data bytes from {} to {}", bytes_recv, from_fd, to_fd);
        ev.el->perp_recv(from_fd, 0, handle_data, (uint64_t)handle_tls_stage::handle_header);
        ev.el->prep_send(to_fd, bid, (size_t)bytes_recv, free_buff);

    } else {
        ev.el->free_buf(bid);
    }
}


void dp_thread::function()
{
    spdlog::info("starting dp thread");

    el.init(1 < 14, 1024);
    while (not stop_flag)
    {   
        el.process();
        itc_message msg;
        if (DP_thread_queue.pop(msg)) {
            switch (msg.type)
            {
            case itc_message_type::NEW_CONNECTION:
                spdlog::debug("registed new forwarding rule {} <-> {}", msg.sock_pair.client_fd, msg.sock_pair.server_fd);

                    el.perp_recv(msg.sock_pair.client_fd, 0, handle_data, (uint64_t)handle_tls_stage::handle_header);
                    el.perp_recv(msg.sock_pair.server_fd, 0, handle_data, (uint64_t)handle_tls_stage::handle_header);

                    fwd_rules.insert({msg.sock_pair.client_fd, msg.sock_pair.server_fd});
                    fwd_rules.insert({msg.sock_pair.server_fd, msg.sock_pair.client_fd});
                break;
            
            default:
                spdlog::warn("unsupported msg type");
                break;
            }
        }
    }
    
}
#pragma once

#include <memory>
#include <vector>
#include <order.h>
#include <spsc_queue.h>
#include <unordered_map>
#include <order_routing_system.h>
#include <core.h>

class alignas(CACHE_LINE_SIZE) Producer {
public:
    using SPSCQueues = std::vector<SPSCQueue<OrderCommand>>;

    Producer(OrderRoutingSystem& order_routing_system):
        _order_routing_system(order_routing_system)
    {}
    
    bool submit(const OrderCommand& cmd) {
        auto it = _order_routing_system.instrument_router.find(cmd.order.get_instrument());
        
        if (it == _order_routing_system.instrument_router.end())
            return false;
            
        size_t q_idx = it -> second;
        return (_order_routing_system.worker_contexts[q_idx] -> queue).push(cmd);
    }

    bool submit(OrderCommand&& cmd) {
        auto it = _order_routing_system.instrument_router.find(cmd.order.get_instrument());
        
        if (it == _order_routing_system.instrument_router.end())
            return false;
            
        size_t q_idx = it -> second;
        return (_order_routing_system.worker_contexts[q_idx] -> queue).push(std::move(cmd));
    }
    
    // this is a blocking call
    void submit_all_shutdown_commands() {
        size_t num_workers = _order_routing_system.worker_contexts.size();
        std::vector<bool> shutdown_commands_submitted(num_workers, false);
        size_t shutdown_count = 0;
        
        while (shutdown_count != num_workers) {
            for (size_t i = 0; i < num_workers; ++i) {
                if (shutdown_commands_submitted[i])
                    continue;
                    
                OrderCommand shutdown_cmd;
                shutdown_cmd.type = OrderCommand::Type::SHUTDOWN;
                bool shutdown_submitted = (_order_routing_system.worker_contexts[i] -> queue).push(shutdown_cmd);
                
                if (shutdown_submitted) {
                    shutdown_commands_submitted[i] = true;
                    ++shutdown_count;   
                }
            }
        }
    }
    
private:
    OrderRoutingSystem& _order_routing_system;
    char pad[CACHE_LINE_SIZE];
};

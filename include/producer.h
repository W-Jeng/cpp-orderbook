#pragma once

#include <memory>
#include <vector>
#include <order.h>
#include <spsc_queue.h>
#include <unordered_map>
#include <core.h>

class alignas(CACHE_LINE_SIZE) Producer {
public:
    using SPSCQueues = std::vector<SPSCQueue<OrderCommand>>;

    Producer(SPSCQueues& queues, const std::unordered_map<Instrument, size_t>& route_map):
        _queues(queues),
        _route_map(route_map)
    {}
    
    bool submit(const OrderCommand& cmd) {
        auto it = _route_map.find(cmd.order.get_instrument());
        
        if (it == _route_map.end() && cmd.type != OrderCommand::Type::SHUTDOWN)
            return false;
            
        size_t q_idx = it -> second;
        return _queues[q_idx].push(cmd);
    }

    bool submit(OrderCommand&& cmd) {
        auto it = _route_map.find(cmd.order.get_instrument());
        
        if (it == _route_map.end() && cmd.type != OrderCommand::Type::SHUTDOWN) {
            return false;
        }

        size_t q_idx = it -> second;
        return _queues[q_idx].push(std::move(cmd));
    }
    
private:
    SPSCQueues& _queues;
    // read only routing on map
    const std::unordered_map<Instrument, size_t>& _route_map;
};

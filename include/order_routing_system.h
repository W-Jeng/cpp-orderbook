#pragma once

#include <vector>
#include <unordered_map>
#include <producer.h>
#include <order.h>
#include <worker.h>
#include <spsc_queue.h>
#include <id_allocator.h>
#include <core.h>

// the key idea is to keep track of [worker_index1] -> {instrument1, instrument2,... }
// then the router is to route to the right worker_index based on the instrument
// finally the queue is shared by consumer (given raw ptr from unique ptr) and producer (where this gets the unique ptr)

struct alignas(CACHE_LINE_SIZE) OrderRoutingSystem {
    std::vector<std::vector<OrderBook>> worker_orderbooks;
    std::unordered_map<Instrument, size_t> instrument_to_worker;
    std::vector<std::unique_ptr<SPSCQueue<OrderCommand>>> queues;
};

OrderRoutingSystem build_order_routing_system(
    const std::vector<Instrument>& instruments,
    IdAllocator& id_allocator,
    const size_t num_workers,
    const size_t queue_cap
) {
    OrderRoutingSystem order_routing_sys;
    order_routing_sys.queues.reserve(num_workers);
    order_routing_sys.worker_orderbooks.resize(num_workers);

    
    // let's do round-robin to distribute the workload to each worker and map the router accordingly
    for (size_t i = 0; i < instruments.size(); ++i) {
        size_t allocated_worker_idx = i % num_workers;
        const Instrument& instrument = instruments[i];
        order_routing_sys.worker_orderbooks[allocated_worker_idx].emplace_back(instrument, id_allocator);
        order_routing_sys.instrument_to_worker.emplace(instrument, allocated_worker_idx);
    }
    
    for (size_t i = 0; i < num_workers; ++i) {
        order_routing_sys.queues.emplace_back(std::make_unique<SPSCQueue<OrderCommand>>(queue_cap));
    }
    
    return order_routing_sys;
}
    
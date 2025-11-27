#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <order.h>
#include <spsc_queue.h>
#include <id_allocator.h>
#include <core.h>

// i think false sharing is happening between threads
struct alignas(CACHE_LINE_SIZE) WorkerContext {
    std::vector<OrderBook> orderbooks;
    SPSCQueue<OrderCommand> queue;
    size_t reserve_space_per_instrument;
    
    WorkerContext(std::vector<Instrument>& instruments, IdAllocator& id_allocator, const size_t queue_cap, const size_t r_space):
        queue(queue_cap),
        reserve_space_per_instrument(r_space)
    {
        orderbooks.reserve(instruments.size());
        
        for (auto& instrument: instruments)
            orderbooks.push_back(OrderBook(instrument, id_allocator, r_space));
    }
};

struct alignas(CACHE_LINE_SIZE) OrderRoutingSystem {
    alignas(CACHE_LINE_SIZE) std::vector<std::unique_ptr<WorkerContext>> worker_contexts;
    char pad0[CACHE_LINE_SIZE];
    alignas(CACHE_LINE_SIZE) std::unordered_map<Instrument, size_t> instrument_router;
    char pad1[CACHE_LINE_SIZE];
};

OrderRoutingSystem build_order_routing_system(
    const std::vector<Instrument>& instruments,
    IdAllocator& id_allocator,
    const size_t num_workers,
    const size_t queue_cap,
    const size_t total_reserve_space
) {
    OrderRoutingSystem order_routing_sys;
    order_routing_sys.worker_contexts.reserve(num_workers);
    std::vector<std::vector<Instrument>> instrument_sets;
    instrument_sets.resize(num_workers);    
    size_t reserve_space_per_instrument = total_reserve_space/instruments.size() + 1;

    for (size_t i = 0; i < instruments.size(); ++i) {
        size_t allocated_worker_idx = i % num_workers;
        Instrument instrument = instruments[i];
        instrument_sets[allocated_worker_idx].push_back(instrument);
        order_routing_sys.instrument_router.emplace(instrument, allocated_worker_idx);
    }
    
    for (size_t w = 0; w < num_workers; ++w) {
        std::unique_ptr<WorkerContext> worker_data = std::make_unique<WorkerContext>(
            instrument_sets[w], 
            id_allocator, 
            queue_cap, 
            reserve_space_per_instrument
        );
        order_routing_sys.worker_contexts.push_back(std::move(worker_data));
    }
    
    return order_routing_sys;
}
    
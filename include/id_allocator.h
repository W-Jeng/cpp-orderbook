#pragma once

#include <cstdint>
#include <order.h>
#include <core.h>
#include <atomic>

struct alignas(CACHE_LINE_SIZE) OrderIdBlock{
    OrderId _start;
    OrderId _end;
    
    OrderIdBlock(OrderId start, OrderId end):
        _start(start),
        _end(end)
    {}
};

// the idea is to allocate the thread id in blocks so we can reduce the fetch add for multithreaded (less contention)
class alignas(CACHE_LINE_SIZE) IdAllocator {
public:
    static constexpr OrderId DEFAULT_BLOCK_SIZE = 1'000'000;

    IdAllocator():
        _block_size(DEFAULT_BLOCK_SIZE)
    {}

    IdAllocator(const OrderId block_size):
        _block_size(block_size)
    {}

    OrderIdBlock get_next_id_block() {
        OrderId local_start = _global_counter.fetch_add(_block_size);
        OrderId local_end = local_start + _block_size;
        return OrderIdBlock(local_start, local_end);                 
    }

private:
    const OrderId _block_size;
    std::atomic<OrderId> _global_counter{FIRST_ORDER_ID};
    char pad[64];
};
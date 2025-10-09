#pragma once

#include <cstdint>
#include <order.h>

struct OrderIdBlock{
    OrderId _start;
    OrderId _end;
    
    OrderIdBlock(OrderId start, OrderId end):
        _start(start),
        _end(end)
    {}
};

// we try with single threaded first
// the idea is to allocate the thread id in blocks so we can reduce the fetch add for multithreaded (less contention)
class ThreadIDAllocator {
public:
    static constexpr OrderId DEFAULT_BLOCK_SIZE = 1'000'000;

    ThreadIDAllocator():
        _block_size(DEFAULT_BLOCK_SIZE)
    {}

    ThreadIDAllocator(const OrderId block_size):
        _block_size(block_size)
    {}

    OrderIdBlock get_next_id_block() {
        OrderId local_start = _global_counter;
        _global_counter += _block_size;
        OrderId local_end = _global_counter;
        // if using atomic
        // _global_counter = _global_counter.fetch_add(_block_size);
        return OrderIdBlock(local_start, local_end);                 
    }

private:
    const OrderId _block_size;
    OrderId _global_counter{0};
};
#pragma once

#include <cstdint>

struct OrderIdBlock{
    uint64_t _start;
    uint64_t _end;
    
    OrderIdBlock(uint64_t start, uint64_t end):
        _start(start),
        _end(end)
    {}
};

// we try with single threaded first
// the idea is to allocate the thread id in blocks so we can reduce the fetch add for multithreaded (less contention)
class ThreadIDAllocator {
public:
    static constexpr uint64_t DEFAULT_BLOCK_SIZE = 1'000'000;

    ThreadIDAllocator():
        _block_size(DEFAULT_BLOCK_SIZE)
    {}

    ThreadIDAllocator(const uint64_t block_size):
        _block_size(block_size)
    {}

    OrderIdBlock get_next_id_block() {
        uint64_t local_start = _global_counter;
        _global_counter += _block_size;
        uint64_t local_end = _global_counter;
        // if using atomic
        // _global_counter = _global_counter.fetch_add(_block_size);
        return OrderIdBlock(local_start, local_end);                 
    }

private:
    const uint64_t _block_size;
    uint64_t _global_counter{0};
};
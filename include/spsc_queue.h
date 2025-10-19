#pragma once

#include <vector>
#include <atomic>
#include <new>
#include <core.h>

// a very simple implementation of ring buffer
template <typename T>
class alignas(CACHE_LINE_SIZE) SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity):
        _buffer(capacity),
        _head(0),
        _tail(0) 
    {}
    
    // Disable copying (since atomics can’t be copied safely)
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Allow moving
    SPSCQueue(SPSCQueue&& other) noexcept
        : _buffer(std::move(other._buffer)),
          _head(other._head.load(std::memory_order_relaxed)),
          _tail(other._tail.load(std::memory_order_relaxed))
    {}

    SPSCQueue& operator=(SPSCQueue&& other) noexcept {
        if (this != &other) {
            _buffer = std::move(other._buffer);
            _head.store(other._head.load(std::memory_order_relaxed), std::memory_order_relaxed);
            _tail.store(other._tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    bool push(const T& item) noexcept {
        size_t next = (_head + 1) % _buffer.size();
        
        if (next == _tail) 
            return false;
            
        _buffer[_head] = item;
        _head = next;
        return true;
    }
    
    bool pop(T& item) {
        if (_tail == _head)
            return false;
            
        item = _buffer[_tail];
        _tail = (_tail + 1) % _buffer.size();
        return true; 
    }

private:
    std::vector<T> _buffer;
    char pad0[64];
    std::atomic<size_t> _head;
    char pad1[64 - sizeof(std::atomic<size_t>)];
    std::atomic<size_t> _tail;
    char pad2[64 - sizeof(std::atomic<size_t>)];
};
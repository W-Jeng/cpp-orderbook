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
        size_t head = _head.load(std::memory_order_relaxed);
        size_t next = (head + 1) % _buffer.size();
        size_t tail = _tail.load(std::memory_order_acquire);

        if (next == tail)
            return false;

        _buffer[head] = item;
        _head.store(next, std::memory_order_release);
        return true;
    }

    bool push(T&& item) noexcept {
        size_t head = _head.load(std::memory_order_relaxed);
        size_t next = (head + 1) % _buffer.size();
        size_t tail = _tail.load(std::memory_order_acquire);

        if (next == tail)
            return false;

        _buffer[head] = std::move(item);
        _head.store(next, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t tail = _tail.load(std::memory_order_relaxed);
        size_t head = _head.load(std::memory_order_acquire);

        if (tail == head)
            return false;

        item = std::move(_buffer[tail]);
        _tail.store((tail+1) % _buffer.size(), std::memory_order_release);
        return true;
    }

private:
    char pad0[64];
    std::vector<T> _buffer;
    char pad1[64];
    std::atomic<size_t> _head;
    char pad2[64];
    std::atomic<size_t> _tail;
    char pad3[64];
};
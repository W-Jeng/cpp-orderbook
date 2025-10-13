#pragma once

#include <vector>
#include <atomic>

// a very simple implementation of ring buffer
template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity):
        _buffer(capacity),
        _head(0),
        _tail(0) 
    {}
    
    bool push(const T& item) noexcept {
        size_t next = (_head + 1) % _buffer.size();
        
        if (next == _tail) 
            return false;
            
        _buffer[_head] = item;
        _head = _next;
        return true;
    }
    
    bool pop(T& item) {
        if (_tail == head)
            return false;
            
        item = _buffer[_tail];
        _tail = (_tail + 1) % _buffer.size();
        return true; 
    }

private:
    std::vector<T> _buffer;
    std::atomic<size_t> _head;
    std::atomic<size_t> _tail;
};
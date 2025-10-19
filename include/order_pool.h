#pragma once

#include <vector>
#include <order.h>
#include <core.h>

class alignas(CACHE_LINE_SIZE) OrderPool {
public:
    explicit OrderPool(size_t reserve_size)
        : _next_free_index(0)
    {
        _orders.reserve(reserve_size);

        for (size_t i = 0; i < reserve_size; ++i)
            _orders.emplace_back(); 
    }

    Order* allocate() {
        if (_next_free_index >= _orders.size())
            reserve_more();

        return &_orders[_next_free_index++];
    }

private:
    size_t _next_free_index;
    std::vector<Order> _orders;

    void reserve_more() {
        size_t old_size = _orders.size();
        _orders.reserve(old_size * 2);
        for (size_t i = 0; i < old_size; ++i)
            _orders.emplace_back();
    }
};
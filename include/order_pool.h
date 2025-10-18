#pragma once

#include <vector>
#include <order.h>

class OrderPool {
public:
    OrderPool(size_t reserve_size):
        _next_free_index(0),
        _reserve_size(reserve_size)
    {
        for (size_t i = 0; i < _reserve_size; ++i)
            _orders.push_back(std::make_unique<Order>());
    }

    Order* allocate() {
        if (_next_free_index >= _orders.size())
            reserve_more();

        return _orders[_next_free_index++].get();
    }

private:
    const size_t _reserve_size;
    size_t _next_free_index;
    std::vector<std::unique_ptr<Order>> _orders;

    void reserve_more() {
        size_t elements_to_add = _orders.size();
        _orders.reserve(_orders.size() * 2);

        for (size_t i = 0; i < elements_to_add; ++i)
            _orders.push_back(std::make_unique<Order>());
    }
};
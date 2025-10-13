#pragma once

#include <id_allocator.h>
#include <order.h>
#include <orderbook.h>
#include <unordered_map>
#include <vector>

// This implementation is likely not correct due to the multithreaded nature we are aiming for
// A wrapper to distribute the orders to different OrderBook
class BookMap {
public:
    inline static constexpr size_t BOOK_RESERVE = 128;
    inline static constexpr size_t ORDER_REGISTRY_RESERVE = 1'000'000;

    BookMap(const std::vector<Instrument>& instruments):
        _id_allocator(IdAllocator{})
    {
        _book.reserve(BOOK_RESERVE);
        _registry.reserve(ORDER_REGISTRY_RESERVE);

        for (const auto& instrument: instruments) 
            _book.try_emplace(instrument, instrument, _id_allocator);
    }

    OrderId add_order(const Order& order) {
        auto orderbook_it = _book.find(order.get_instrument());

        if (orderbook_it == _book.end()) 
            return INVALID_ORDER_ID;

        OrderId id = (orderbook_it -> second).add_order(order);
        _registry[id] = &(orderbook_it -> second);
        return id;
    }

    bool cancel_order(OrderId order_id) {
        auto orderbook_it = _registry.find(order_id);

        if (orderbook_it == _registry.end())
            return false;

        OrderBook* orderbook = orderbook_it -> second;
        
        if (orderbook -> cancel_order(order_id)) {
            _registry.erase(orderbook_it);
            return true;
        }

        return false;
    }

    bool modify_order(OrderId order_id, Price price, Quantity quantity) {
        auto orderbook_it = _registry.find(order_id);

        if (orderbook_it == _registry.end())
            return false;

        OrderBook* orderbook = orderbook_it -> second;
        return orderbook -> modify_order(order_id, price, quantity);
    }

private:
    IdAllocator _id_allocator;
    std::unordered_map<Instrument, OrderBook> _book;
    std::unordered_map<OrderId, OrderBook*> _registry;
};
#pragma once

#include <list>
#include <order.h>
#include <unordered_map>
#include <cstdint>
#include <memory>

class PriceLevel {
public:
    using OrderPtr = std::unique_ptr<ExchangeOrder>;
    using OrderList = std::list<OrderPtr>;

    PriceLevel(int price):
        _price(price)
    {}

    void add_order(OrderPtr eorder_ptr) {
        const uint64_t order_id = eorder_ptr -> _id;
        _orders_list.push_back(std::move(eorder_ptr));
        auto it = --_orders_list.end();
        _orders_map[order_id] = it;
    }

    void remove_order(uint64_t order_id) {
        auto it_map = _orders_map.find(order_id);
        if (it_map == _orders_map.end()) {
            return;
        }
        auto it_list = it_map->second;
        _orders_list.erase(it_list);
        _orders_map.erase(it_map);
    }

    void modify_qty(OrderPtr eorder_ptr) {
        
    }

private:
    const double _price;
    OrderList _orders_list;
    std::unordered_map<uint64_t, OrderList::iterator> _orders_map;
};
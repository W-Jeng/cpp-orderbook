#pragma once

#include <list>
#include <order.h>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

class PriceLevel {
public:
    using OrderPtr = std::unique_ptr<ExchangeOrder>;
    using OrderList = std::list<OrderPtr>;

    PriceLevel(double price):
        _price(price)
    {}

    void add_order(OrderPtr eorder_ptr) {
        const uint64_t order_id = eorder_ptr -> _id;
        _orders_list.push_back(std::move(eorder_ptr));
        auto it = --_orders_list.end();
        _orders_map[order_id] = it;
    }

    bool remove_order(uint64_t order_id) {
        auto it_map = _orders_map.find(order_id);

        if (it_map == _orders_map.end()) {
            return false;
        }

        auto it_list = it_map->second;
        _orders_list.erase(it_list);
        _orders_map.erase(it_map);
        return true;
    }

    bool modify_qty(OrderPtr eorder_ptr) {
        // if quantity increase, then lose queue priority
        // if quantity decrease, then just modify internal pointer info
        auto it_map = _orders_map.find(eorder_ptr -> _id);

        if (it_map == _orders_map.end()) {
            return false;
        }
        
        auto it_list = it_map -> second;
        auto& target_order_ptr = *it_list;
        const uint64_t new_qty = eorder_ptr -> _quantity;
        const uint64_t filled_qty  = target_order_ptr -> _quantity_filled;
        
        if (new_qty > target_order_ptr -> _quantity) {
            // lose queue priority
            OrderPtr temp = std::move(target_order_ptr);
            _orders_list.erase(it_list);
            _orders_map.erase(it_map);
            temp -> modify_quantity(new_qty);
            add_order(std::move(temp));
        } else {
            // just change internal quantity information
            target_order_ptr -> modify_quantity(new_qty);
        }

        return true;
    }
    
    size_t size() {
        return _orders_list.size();   
    }
    
    bool empty() {
        return _orders_list.empty();   
    }
    
private:
    const double _price;
    OrderList _orders_list;
    std::unordered_map<uint64_t, OrderList::iterator> _orders_map;
};
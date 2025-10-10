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
    using OrderList = std::list<ExchangeOrder*>;

    PriceLevel(Price price):
        _price(price)
    {}

    void add_order(ExchangeOrder* order) {
        const OrderId order_id = order -> _id;
        _orders_list.push_back(order);
        auto it = --_orders_list.end();
        _orders_map[order_id] = it;
    }

    bool remove_order(OrderId order_id) {
        auto it_map = _orders_map.find(order_id);

        if (it_map == _orders_map.end())
            return false;

        auto it_list = it_map->second;
        _orders_list.erase(it_list);
        _orders_map.erase(it_map);
        return true;
    }
    
    bool remove_order_on_it(OrderList::iterator order_it) {
        OrderId order_id = (*order_it) -> _id;
        _orders_list.erase(order_it);
        _orders_map.erase(order_id);
    }

    size_t size() const noexcept {
        return _orders_list.size();   
    }
    
    bool empty() const noexcept {
        return _orders_list.empty();   
    }
    
    Quantity total_qty() const {
        Quantity qty = 0;
        
        for (auto order: _orders_list) {
            qty += (order -> _quantity - order -> _quantity_filled);
        }
        
        return qty;
    }
    
    ExchangeOrder* front() {
        return *(_orders_list.begin());
    }
    
    void pop_front_order_if_filled() {
        if (empty()) {
            return;   
        }
        
        ExchangeOrder* order = front();

        if (order -> _status == OrderStatus::FULLY_FILLED) {
            _orders_list.pop_front();
        }
    }
    
private:
    const Price _price;
    OrderList _orders_list;
    std::unordered_map<OrderId, OrderList::iterator> _orders_map;
};
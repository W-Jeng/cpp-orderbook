#pragma once

#include <list>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <order.h>
#include <core.h>

class PriceLevel {
public:
    using OrderList = std::list<Order*>;

    PriceLevel(Price price):
        _price(price)
    {}

    void add_order(Order* order) {
        const OrderId order_id = order -> get_id();
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
    
    void remove_order_on_it(OrderList::iterator order_it) {
        OrderId order_id = (*order_it) -> get_id();
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
        
        for (auto order: _orders_list) 
            qty += (order -> get_quantity() - order -> get_quantity_filled());
        
        return qty;
    }
    
    Order* front() {
        return *(_orders_list.begin());
    }
    
    const Order* front() const {
        return *(_orders_list.begin());
    }

    void pop_front_order_if_filled() {
        if (empty()) 
            return;   
        
        Order* order = front();

        if (order -> get_status() == OrderStatus::FULLY_FILLED)
            _orders_list.pop_front();
    }
    
    const OrderList& get_order_list() const {
        return _orders_list;
    }

private:
    const Price _price;
    OrderList _orders_list;
    std::unordered_map<OrderId, OrderList::iterator> _orders_map;
};

inline std::ostream& operator<<(std::ostream& os, const PriceLevel& price_level) {
    os << "PriceLevel(";
    const std::list<Order*> orders_list = price_level.get_order_list();
    auto it = orders_list.begin();
    
    while (it != orders_list.end()) {
        if (it != orders_list.begin()) {
            os << "->";
        }

        Order* order = *it;
        os << *order;
        ++it;
    }

    os << ")";
    return os;
}
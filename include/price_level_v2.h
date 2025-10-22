#pragma once

#include <list>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <order.h>
#include <core.h>
#include <boost/unordered/unordered_flat_map.hpp>

struct OrderEntry{
    bool is_valid;
    Order* order;

    OrderEntry(Order* o):
        is_valid(true),
        order(o)
    {}
};

class alignas(CACHE_LINE_SIZE) PriceLevel {
public:
    using OrderEntries = std::vector<OrderEntry>;
    static constexpr size_t ORDER_ENTRY_RESERVE = 1'048'576;
    std::chrono::duration<double> elapsed;
    using clock = std::chrono::high_resolution_clock;

    PriceLevel(Price price):
        _price(price),
        _queue_head_index(0)
    {
        _order_entries.reserve(ORDER_ENTRY_RESERVE);
        _orders_map.reserve(ORDER_ENTRY_RESERVE * 1.5);
        // boost::unordered_flat_map<int, std::string> id_to_name;
    }

    void add_order(Order* order) {
        const OrderId order_id = order -> get_id();
        _order_entries.push_back(OrderEntry(order));
        // auto start = clock::now();
        _orders_map[order_id] = _order_entries.size()-1;
        // auto end = clock::now();
        // elapsed += end-start;
    }

    void print_time() {
        std::cout << "Elapsed time (count) in Price Level Add: " << elapsed.count() << "\n";
    }

    bool remove_order(OrderId order_id) {
        auto it_map = _orders_map.find(order_id);

        if (it_map == _orders_map.end())
            return false;

        size_t order_entry_index = it_map->second;
        _order_entries[order_entry_index].is_valid = false;
        _orders_map.erase(it_map);
        increment_to_valid();
        return true;
    }
    
    void increment_to_valid() {
        if (empty()) {
            return;
        }
        
        // strictly increasing
        while (!_order_entries[_queue_head_index].is_valid) {
            ++_queue_head_index;
        }
    }

    size_t size() const noexcept {
        return _orders_map.size();   
    }
    
    bool empty() const noexcept {
        return _orders_map.empty();   
    }
    
    Quantity total_qty() const {
        Quantity qty = 0;
        
        for (auto& order_entry: _order_entries) {
            if (!order_entry.is_valid) 
                continue;

            qty += (order_entry.order -> get_quantity() - order_entry.order -> get_quantity_filled());
        }
        
        return qty;
    }
    
    Order* front() {
        return _order_entries[_queue_head_index].order;
    }
    
    const Order* front() const {
        return _order_entries[_queue_head_index].order;
    }

    void pop_front_order_if_filled() {
        if (empty()) 
            return;   
        
        OrderEntry& order_entry = _order_entries[_queue_head_index];

        if (order_entry.order -> get_status() == OrderStatus::FULLY_FILLED) {
            _orders_map.erase(_orders_map.find(order_entry.order -> get_id()));
            order_entry.is_valid = false;
            increment_to_valid();
        }
    }

    const OrderEntries& get_order_entries() const {
        return _order_entries;
    }

    const size_t get_queue_head_index() const {
        return _queue_head_index;
    }
    
private:
    const Price _price;
    OrderEntries _order_entries;
    size_t _queue_head_index;
    boost::unordered_flat_map<OrderId, size_t> _orders_map;
};

inline std::ostream& operator<<(std::ostream& os, const PriceLevel& price_level) {
    os << "PriceLevel(";
    const std::vector<OrderEntry>& order_entries = price_level.get_order_entries();
    bool first_order_entry = true;

    for (const auto& order_entry: order_entries) {
        if (!order_entry.is_valid)
            continue;
        
        if (!first_order_entry)
            os << "->";

        first_order_entry = false;
        os << *order_entry.order;
    }

    os << ")";
    return os;
}
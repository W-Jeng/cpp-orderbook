#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <price_level.h>
#include <order.h>
#include <thread_id_allocator.h>
#include <new>

class alignas(std::hardware_destructive_interference_size) OrderBook {
public:
    explicit OrderBook(const Instrument& instrument, ThreadIDAllocator& thread_id_allocator): 
        _instrument(instrument),
        _thread_id_allocator(thread_id_allocator),
        _order_id_block(thread_id_allocator.get_next_id_block())
    {
        _next_available_order_id = _order_id_block._start;
    }
    
    // returns order id
    uint64_t add_order(const Order& order) {
        OrderPtr eorder = std::make_unique<ExchangeOrder>(order, get_order_id());

        switch (order._side) {
            case OrderSide::BUY:
                add_order_dispatcher(_bids, std::move(eorder));           
                break;
                
            case OrderSide::SELL:
                add_order_dispatcher(_asks, std::move(eorder));
                break;
        }
        
        return eorder._id;
    }
    
    void modify_order(OrderId order_id, Price price, Quantity quantity) {
        bool price_changed = false;
        bool quantity_changed = false;

    }
    
    template <typename MapType>
    void add_order_dispatcher(MapType& book, OrderPtr order) {
        auto [it, inserted] = book.try_emplace(order._price, order._price);
        PriceLevel& p_level = it -> second;
        // remove below
        p_level.add_order(order);
        _order_id_to_level[order._id] = std::move(order);
    }
    
    void try_match() {
        
    }
    
    OrderId get_order_id() {
        if (_next_available_order_id == _order_id_block._end) {
            _order_id_block = _thread_id_allocator.get_next_id_block();
            _next_available_order_id = _order_id_block._start;
        }
        return _next_available_order_id++;
    }

private:
    const Instrument _instrument;
    std::map<Price, PriceLevel, std::greater<double>> _bids;
    std::map<Price, PriceLevel, std::less<double>> _asks;
    std::unordered_map<OrderId, OrderPtr> _order_id_to_order;
    ThreadIDAllocator& _thread_id_allocator;
    OrderIdBlock _order_id_block;
    OrderId _next_available_order_id;
};
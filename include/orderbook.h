#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <price_level.h>
#include <order.h>
#include <thread_id_allocator.h>

class OrderBook {
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
        ExchangeOrder eorder(order, get_order_id());

        switch (order._side) {
            case OrderSide::BUY:
                add_order_dispatcher(_bids, eorder);           
                break;
                
            case OrderSide::SELL:
                add_order_dispatcher(_asks, eorder);
                break;
        }
        
        return eorder._id;
    }
    
    void modify_order(OrderId order_id, Price price) {
        // auto it = _order_id_to_level.find(order -> _id);
    }
    
    template <typename MapType>
    void add_order_dispatcher(MapType& book, ExchangeOrder& order) {
        auto [it, inserted] = book.try_emplace(order._price, order._price);
        PriceLevel& p_level = it -> second;
        p_level.add_order(order);
        _order_id_to_level[order._id] = &p_level;
    }
    
    void try_match() {
        
    }
    
    uint64_t get_order_id() {
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
    std::unordered_map<OrderId, PriceLevel*> _order_id_to_level;
    ThreadIDAllocator& _thread_id_allocator;
    OrderIdBlock _order_id_block;
    uint64_t _next_available_order_id;
};
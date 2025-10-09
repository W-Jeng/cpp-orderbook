#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <price_level.h>
#include <order.h>
#include <thread_id_allocator.h>

class OrderBook {
public:
    explicit OrderBook(const std::string& instrument, ThreadIDAllocator& thread_id_allocator): 
        _instrument(instrument),
        _thread_id_allocator(thread_id_allocator),
        _order_id_block(thread_id_allocator.get_next_id_block())
    {
        _active_order_id = _order_id_block._start;
    }

    void add_order(const Order& order, uint64_t assigned_order_id) {
        ExchangeOrder eorder(order, assigned_order_id);

        switch (order._side) {
            case OrderSide::BUY:
                add_order_dispatcher(_bids, eorder);           
                break;
                
            case OrderSide::SELL:
                add_order_dispatcher(_asks, eorder);
                break;
        }
    }
    
    template <typename MapType>
    void add_order_dispatcher(MapType& book, ExchangeOrder& order) {
        
    }
    
    void try_match() {
        
    }
    
    uint64_t get_order_id() {
        if (_active_order_id == _order_id_block._end) {
            _order_id_block = _thread_id_allocator.get_next_id_block();
            _active_order_id = _order_id_block._start;
        }
        return _active_order_id++;
    }

private:
    const std::string _instrument;
    std::map<double, PriceLevel, std::greater<double>> _bids;
    std::map<double, PriceLevel, std::less<double>> _asks;
    std::unordered_map<uint64_t, PriceLevel*> _order_price_map;
    ThreadIDAllocator& _thread_id_allocator;
    OrderIdBlock _order_id_block;
    uint64_t _active_order_id;
};
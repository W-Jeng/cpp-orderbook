#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <price_level.h>
#include <order.h>
#include <id_allocator.h>
#include <new>

class alignas(std::hardware_destructive_interference_size) OrderBook {
public:
    explicit OrderBook(const Instrument& instrument, IdAllocator& id_allocator): 
        _instrument(instrument),
        _id_allocator(id_allocator),
        _order_id_block(id_allocator.get_next_id_block())
    {
        _next_available_order_id = _order_id_block._start;
    }
    
    // returns order id
    OrderId add_order(const Order& order) {
        OrderPtr eorder = std::make_unique<ExchangeOrder>(order, get_order_id());

        switch (order._side) {
            case OrderSide::BUY:
                add_order_dispatcher(_bids, std::move(eorder));           
                break;
                
            case OrderSide::SELL:
                add_order_dispatcher(_asks, std::move(eorder));
                break;
        }
        
        return eorder -> _id;
    }
    
    bool modify_order(OrderId order_id, Price price, Quantity quantity) {
        auto it = _order_registry.find(order_id);
        
        if (it == _order_registry.end()) 
            return false;
        
        ExchangeOrder* order = (it->second).get();
        OrderStatus status = order -> _status;
        
        if (status == OrderStatus::CANCELLED || status == OrderStatus::FULLY_FILLED) 
            return false;   
        
        OrderSide side = order -> _side;
        
        try {
            switch (order -> _side) {
                case OrderSide::BUY:
                    return modify_order_dispatcher(_bids, order, price, quantity);           
                case OrderSide::SELL:
                    return modify_order_dispatcher(_asks, order, price, quantity);           
            }
        } catch (const std::exception& e) {
            std::cout << "Unknown exception: " << e.what() << std::endl;   
        } catch (const std::runtime_error& e) {
            std::cout << "Unknown runtime error: " << e.what() << std::endl;   
        } catch (...) {
            std::cerr << "Unknown exception caught in modify order dispatcher!" << std::endl;   
        }
        return false;
    }
    
    template<typename MapType>
    bool modify_order_dispatcher(MapType& book, ExchangeOrder* order, Price new_price, Quantity new_qty) {
        if (!validate_modify_order(order, new_price, new_qty))
            return false;   
        
        auto it = book.find(order -> _price);
        PriceLevel& old_price_level = it -> second;
        bool price_changed = (order -> _price != new_price);
        bool quantity_changed = (order -> _quantity != new_qty);
        
        if (price_changed) {
            old_price_level.remove_order(order -> _id);   
            order -> _price = new_price;
            auto [it, inserted] = book.try_emplace(new_price, new_price);
            PriceLevel& new_price_level = it -> second;
            new_price_level.add_order(order);
        }
        
        if (quantity_changed) {
            bool quantity_increased = (new_qty > order -> _quantity);
            order -> _quantity = new_qty;
            
            if (quantity_increased && !price_changed) {
                old_price_level.remove_order(order -> _id);
                old_price_level.add_order(order);
            }
        }
        
        return true;
    }
    
    bool validate_modify_order(ExchangeOrder* order, Price new_price, Quantity new_qty) {
        if (new_price <= 0.0) 
            return false;   
        
        if (new_qty <= order -> _quantity_filled)
            return false;   
        
        return true;
    }
    
    template <typename MapType>
    void add_order_dispatcher(MapType& book, OrderPtr order) {
        auto [it, inserted] = book.try_emplace(order -> _price, order -> _price);
        PriceLevel& p_level = it -> second;
        p_level.add_order(order.get());
        _order_registry[order -> _id] = std::move(order);
    }
    
    void try_match() {
        
    }
    
    OrderId get_order_id() {
        if (_next_available_order_id == _order_id_block._end) {
            _order_id_block = _id_allocator.get_next_id_block();
            _next_available_order_id = _order_id_block._start;
        }
        return _next_available_order_id++;
    }

private:
    const Instrument _instrument;
    std::map<Price, PriceLevel, std::greater<double>> _bids;
    std::map<Price, PriceLevel, std::less<double>> _asks;
    std::unordered_map<OrderId, OrderPtr> _order_registry;
    IdAllocator& _id_allocator;
    OrderIdBlock _order_id_block;
    OrderId _next_available_order_id;
};
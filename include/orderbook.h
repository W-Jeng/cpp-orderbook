#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <price_level.h>
#include <order.h>
#include <id_allocator.h>
#include <new>
#include <iostream>

constexpr std::size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;

class alignas(CACHE_LINE_SIZE) OrderBook {
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
        OrderId order_id = eorder -> _id;

        switch (order._side) {
            case OrderSide::BUY:
                add_order_dispatcher(_bids, std::move(eorder));           
                break;
                
            case OrderSide::SELL:
                add_order_dispatcher(_asks, std::move(eorder));
                break;
        }
        
        return order_id;
    }
    
    template <typename MapType>
    void add_order_dispatcher(MapType& side_levels, OrderPtr order) {
        auto [it, inserted] = side_levels.try_emplace(order -> _price, order -> _price);
        PriceLevel& p_level = it -> second;
        p_level.add_order(order.get());
        _order_registry[order -> _id] = std::move(order);
    }

    bool cancel_order(OrderId order_id) {
        auto it = _order_registry.find(order_id);
        
        if (it == _order_registry.end()) 
            return false;
        
        ExchangeOrder* order = (it->second).get();
        bool cancel_sucess = order->cancel();
        
        if (!cancel_sucess) {
            return false;   
        }
        
        try {
            switch (order -> _side) {
                case OrderSide::BUY:
                    cancel_order_dispatcher(_bids, order);           
                    break;

                case OrderSide::SELL:
                    cancel_order_dispatcher(_asks, order);           
                    break;
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Unknown runtime error caught in cancel order dispatcher: " << e.what() << std::endl;   
        } catch (const std::exception& e) {
            std::cout << "Unknown exception caught in cancel order dispatcher: " << e.what() << std::endl;   
        } catch (...) {
            std::cerr << "Unknown exception caught in cancel order dispatcher!" << std::endl;   
        }
        return true;
    }
    
    template<typename MapType>
    void cancel_order_dispatcher(MapType& side_levels, ExchangeOrder* order) {
        auto it = side_levels.find(order -> _price);
        PriceLevel& price_level = it -> second;
        price_level.remove_order(order -> _id);
        remove_price_level_if_empty(side_levels, it);
    }
    
    template<typename MapType>
    void remove_price_level_if_empty(MapType& side_levels, typename MapType::iterator price_level_it) {
        if (!(price_level_it -> second).empty())
            return;
            
        // empty, prune the price level!
        side_levels.erase(price_level_it);
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
        } catch (const std::runtime_error& e) {
            std::cout << "Unknown runtime error caught in modify order dispatcher: " << e.what() << std::endl;   
        } catch (const std::exception& e) {
            std::cout << "Unknown exception caught in modify order dispatcher: " << e.what() << std::endl;   
        } catch (...) {
            std::cerr << "Unknown exception caught in modify order dispatcher!" << std::endl;   
        }
        return false;
    }
    
    template<typename MapType>
    bool modify_order_dispatcher(MapType& side_levels, ExchangeOrder* order, Price new_price, Quantity new_qty) {
        if (!validate_modify_order(order, new_price, new_qty))
            return false;   
        
        auto it = side_levels.find(order -> _price);
        PriceLevel& old_price_level = it -> second;
        bool price_changed = (order -> _price != new_price);
        bool quantity_changed = (order -> _quantity != new_qty);
        
        if (price_changed) {
            old_price_level.remove_order(order -> _id);   
            order -> set_price(new_price);
            auto [it, inserted] = side_levels.try_emplace(new_price, new_price);
            PriceLevel& new_price_level = it -> second;
            new_price_level.add_order(order);
        }
        
        if (quantity_changed) {
            bool quantity_increased = (new_qty > order -> _quantity);
            order -> set_quantity(new_qty);
            
            if (quantity_increased && !price_changed) {
                old_price_level.remove_order(order -> _id);
                old_price_level.add_order(order);
            }
        }
        
        remove_price_level_if_empty(side_levels, it);
        return (price_changed || quantity_changed);
    }
    
    bool validate_modify_order(ExchangeOrder* order, Price new_price, Quantity new_qty) {
        if (new_price <= 0.0 || new_qty <= order -> _quantity_filled) 
            return false;   
        
        return true;
    }
    
    bool try_match() {
        bool order_matched = false;
        
        while (true) {
            auto bid_it = _bids.begin();
            auto ask_it = _asks.begin();
            
            if (bid_it == _bids.end() || ask_it == _asks.end()) {
                return order_matched;   
            }

            bool match = (bid_it -> first) >= (ask_it -> first);
            
            if (!match) {
                return order_matched;
            }
            
            order_matched = true;
            
            // implement order match
            PriceLevel& best_bid_price_level = bid_it -> second;
            PriceLevel& best_ask_price_level = ask_it -> second;
            ExchangeOrder* bid_order = best_bid_price_level.front();
            ExchangeOrder* ask_order = best_ask_price_level.front();
            match_orders(bid_order, ask_order);
            best_bid_price_level.pop_front_order_if_filled();
            best_ask_price_level.pop_front_order_if_filled();
        }
        
        return order_matched;
    }
    
    void match_orders(ExchangeOrder* bid_order, ExchangeOrder* ask_order) {
        Quantity bid_available = bid_order -> _quantity - bid_order -> _quantity_filled;
        Quantity ask_available = ask_order -> _quantity - ask_order -> _quantity_filled;
        Quantity match_qty = std::min(bid_available, ask_available);
        bid_order -> match(match_qty);
        ask_order -> match(match_qty);
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
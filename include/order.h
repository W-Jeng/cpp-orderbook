#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FULLY_FILLED,
    CANCELLED
};

// for client submission
struct Order{
    std::string _instrument;
    OrderSide _side;
    double _price;
    uint64_t _quantity;

    Order(std::string instrument, OrderSide side, double price, uint64_t quantity):
        _instrument(instrument),
        _side(side),
        _price(price),
        _quantity(quantity)
    {}
};

// for exchange submission
struct ExchangeOrder{
    using Clock = std::chrono::system_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    uint64_t _id;
    OrderSide _side;
    double _price;
    uint64_t _quantity;
    uint64_t _quantity_filled;
    OrderStatus _status;
    TimePoint _time_submitted;
    TimePoint _updated_time;

    ExchangeOrder(const Order& order, uint64_t id): 
        _side(order._side),
        _price(order._price),
        _quantity(order._quantity),
        _quantity_filled(order._quantity),
        _id(id),
        _status(OrderStatus::NEW)
    {
        TimePoint now = std::chrono::system_clock::now();
        _time_submitted = now;
        _updated_time = now; 
    }

    void set_updated_time(TimePoint t) {
        _updated_time = t;
    }

    void set_order_status(const OrderStatus new_status) {
        _status = new_status;
    }
    
    void add_fill_quantity(uint64_t fill_qty) {
        if (_quantity_filled + fill_qty > _quantity) {
            throw std::runtime_error("Unable to carry add fill quantity due to it exceeds the order quantity amount");
        }
        
        _quantity_filled += fill_qty;
        
        if (_quantity_filled == _quantity) {
            _status = OrderStatus::FULLY_FILLED;  
            return;
        }
        
        _status = OrderStatus::PARTIALLY_FILLED;
    }
    
    bool modify_price(double new_price) {
        if (new_price <= 0.0) {
            return false;
        }
        
        _price = new_price;
        return true;
    }
    
    bool modify_quantity(uint64_t new_quantity) {
        if (new_quantity <= _quantity) {
            return false;
        }
        
        _quantity = new_quantity;
        return true;
    }
};

std::ostream& operator<<(std::ostream& os, const ExchangeOrder& eorder) {
    const char* side_str = (eorder._side == OrderSide::BUY ? "BUY" : "SELL");
    std::time_t updated_time_t = std::chrono::system_clock::to_time_t(eorder._updated_time);
    return os << "ExchangeOrder(id=" << eorder._id << ", side=" << side_str <<
        ", price=" << eorder._price << ", quantity=" << eorder._quantity << 
        ", updated_time=" << updated_time_t << ")";
}

std::ostream& operator<<(std::ostream& os, const Order& order) {
    const char* side_str = (order.side == OrderSide::BUY ? "BUY" : "SELL");
    return os << "Order(side=" << side_str <<
        ", price=" << order.price << ", quantity=" << order.quantity << ")";
}
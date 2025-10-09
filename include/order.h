#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <memory>

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

using Instrument = std::string;
using OrderId = uint64_t;
using Price = double;
using Quantity = uint64_t;

// for client submission
struct Order{
    Instrument _instrument;
    OrderSide _side;
    Price _price;
    Quantity _quantity;

    Order(Instrument instrument, OrderSide side, Price price, Quantity quantity):
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

    OrderId _id;
    OrderSide _side;
    Price _price;
    Quantity _quantity;
    Quantity _quantity_filled;
    OrderStatus _status;
    TimePoint _time_submitted;
    TimePoint _updated_time;

    ExchangeOrder(const Order& order, OrderId id): 
        _side(order._side),
        _price(order._price),
        _quantity(order._quantity),
        _quantity_filled(0),
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
    
    void add_fill_quantity(Quantity fill_qty) {
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
    
    bool modify_price(Price new_price) {
        if (new_price <= 0.0) {
            return false;
        }
        
        _price = new_price;
        return true;
    }
    
    bool modify_quantity(Quantity new_quantity) {
        if (new_quantity <= _quantity) {
            return false;
        }
        
        _quantity = new_quantity;
        return true;
    }
};

using OrderPtr = std::unique_ptr<ExchangeOrder>;

inline std::ostream& operator<<(std::ostream& os, const ExchangeOrder& eorder) {
    const char* side_str = (eorder._side == OrderSide::BUY ? "BUY" : "SELL");
    std::time_t updated_time_t = std::chrono::system_clock::to_time_t(eorder._updated_time);
    return os << "ExchangeOrder(id=" << eorder._id << ", side=" << side_str <<
        ", price=" << eorder._price << ", quantity=" << eorder._quantity << 
        ", updated_time=" << updated_time_t << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Order& order) {
    const char* side_str = (order._side == OrderSide::BUY ? "BUY" : "SELL");
    return os << "Order(side=" << side_str <<
        ", price=" << order._price << ", quantity=" << order._quantity << ")";
}
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
constexpr OrderId INVALID_ORDER_ID = UINT64_MAX; 

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

    void match(Quantity fill_qty) {
        if (_quantity_filled + fill_qty > _quantity) {
            throw std::runtime_error("Unable to carry add fill quantity due to it exceeds the order quantity amount");
        }
        
        _quantity_filled += fill_qty;
        _updated_time = std::chrono::system_clock::now();
        
        if (_quantity_filled == _quantity) {
            _status = OrderStatus::FULLY_FILLED;  
            return;
        }
        
        _status = OrderStatus::PARTIALLY_FILLED;
    }
    
    bool cancel() {
        if (_status == OrderStatus::FULLY_FILLED || _status == OrderStatus::CANCELLED) {
            return false;   
        }
        
        _updated_time = std::chrono::system_clock::now();
        _status = OrderStatus::CANCELLED;
        return true;
    }
    
    void set_price(Price new_price) {
        _updated_time = std::chrono::system_clock::now();
        _price = new_price;
    }
    
    void set_quantity(Quantity new_quantity) {
        _updated_time = std::chrono::system_clock::now();
        _quantity = new_quantity;
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
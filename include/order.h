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
    OrderSide side;
    double price;
    uint64_t quantity;

    Order(OrderSide side, double p, uint64_t qty):
        side(side),
        price(p),
        quantity(qty)
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
    OrderStatus _status;
    TimePoint _time_submitted;
    TimePoint _updated_time;

    ExchangeOrder(const Order& order, uint64_t id): 
        _side(order.side),
        _price(order.price),
        _quantity(order.quantity),
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
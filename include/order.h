#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <chrono>
#include <memory>
#include <core.h>

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

// for exchange submission
class Order{
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    // default
    Order():
        _instrument(""),
        _side(OrderSide::BUY),
        _price(0.0),
        _quantity(0),
        _id(DEFAULT_ORDER_ID),
        _quantity_filled(0),
        _status(OrderStatus::NEW)
    {
        TimePoint now = std::chrono::system_clock::now();
        _time_submitted = now;
        _updated_time = now; 
    }
    
    // information received from client
    Order(Instrument instrument, OrderSide side, Price price, Quantity quantity):
        _instrument(std::move(instrument)),
        _side(side),
        _price(price),
        _quantity(quantity),
        _id(DEFAULT_ORDER_ID),
        _quantity_filled(0),
        _status(OrderStatus::NEW)
    {
        TimePoint now = std::chrono::system_clock::now();
        _time_submitted = now;
        _updated_time = now; 
    }
    
    // information constructed for Orderbook itself
    Order(Order&& client_order, OrderId allocated_id): 
        _instrument(std::move(client_order._instrument)),
        _side(client_order._side),
        _price(client_order._price),
        _quantity(client_order._quantity),
        _id(allocated_id),
        _quantity_filled(0),
        _status(OrderStatus::NEW)
    {
        TimePoint now = std::chrono::system_clock::now();
        _time_submitted = now;
        _updated_time = now; 
    }

    void match(Quantity fill_qty) {
        if (_quantity_filled + fill_qty > _quantity) 
            throw std::runtime_error("Unable to carry add fill quantity due to it exceeds the order quantity amount");
        
        _quantity_filled += fill_qty;
        _updated_time = std::chrono::system_clock::now();
        
        if (_quantity_filled == _quantity) {
            _status = OrderStatus::FULLY_FILLED;  
            return;
        }
        
        _status = OrderStatus::PARTIALLY_FILLED;
    }
    
    bool cancel() {
        if (_status == OrderStatus::FULLY_FILLED || _status == OrderStatus::CANCELLED) 
            return false;   
        
        _updated_time = std::chrono::system_clock::now();
        _status = OrderStatus::CANCELLED;
        return true;
    }
    
    Price get_price() const {
        return _price;
    }
    
    void set_price(Price new_price) {
        _updated_time = std::chrono::system_clock::now();
        _price = new_price;
    }
    
    Quantity get_quantity() const {
        return _quantity;   
    }

    void set_quantity(Quantity qty) {
        _quantity = qty;
    }
    
    Quantity get_quantity_filled() const {
        return _quantity_filled;  
    }
    
    Instrument get_instrument() const {
        return _instrument;
    }

    OrderId get_id() const {
        return _id;
    }
    
    OrderStatus get_status() const {
        return _status;   
    }

    OrderSide get_side() const {
        return _side;   
    }

    TimePoint get_time_submitted() const {
        return _time_submitted;   
    }

    TimePoint get_updated_time() const {
        return _time_submitted;   
    }

private:
    Instrument _instrument;
    OrderSide _side;
    Price _price;
    Quantity _quantity;
    OrderId _id;
    Quantity _quantity_filled;
    OrderStatus _status;
    TimePoint _time_submitted;
    TimePoint _updated_time;
};

struct OrderCommand {
    enum class Type {
        ADD,
        MODIFY,
        CANCEL,
        SHUTDOWN // poison pill to stop thread
    };

    Type type;
    Order order;
    
    OrderCommand():
        type(Type::ADD),
        order(Order{})
    {}

    OrderCommand(Type t, const Order& o):
        type(t),
        order(o)
    {}
};

inline std::ostream& operator<<(std::ostream& os, const Order& order) {
    const char* side_str = (order.get_side() == OrderSide::BUY ? "BUY" : "SELL");
    std::time_t updated_time_t = std::chrono::system_clock::to_time_t(order.get_updated_time());
    return os << "Order(instrument=" << order.get_instrument() << ", id=" << order.get_id() << 
        ", side=" << side_str << ", price=" << order.get_price() << ", quantity=" << order.get_quantity() << 
        ", updated_time=" << updated_time_t << ")";
}

inline std::ostream& operator<<(std::ostream& os, const OrderCommand& order_cmd) {
    std::string command_type;
    
    switch (order_cmd.type) {
        case OrderCommand::Type::ADD:
            command_type = "ADD";   
            break;

        case OrderCommand::Type::MODIFY:
            command_type = "MODIFY";   
            break;

        case OrderCommand::Type::CANCEL:
            command_type = "CANCEL";   
            break;

        case OrderCommand::Type::SHUTDOWN:
            command_type = "SHUTDOWN";   
            break;
    }
    
    return os << "OrderCommand(type=" << command_type << ", order=" << order_cmd.order << ")";
}

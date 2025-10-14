#pragma once

#include <unordered_map>
#include <spsc_queue.h>
#include <vector>
#include <order.h>
#include <orderbook.h>
#include <core.h>

class alignas(CACHE_LINE_SIZE) Worker {
public:
    Worker(SPSCQueue<OrderCommand>* q, std::vector<OrderBook>&& books):
        _queue(q)
    {
        for (auto& book: books) {
            const Instrument instrument = book.get_instrument();
            _books.emplace(instrument, std::move(book));
        }
    }
    
    void run() {
        OrderCommand cmd{};
        
        while (true) {
            if (_queue -> pop(cmd)) {
                // currently, we only hand those that are successful
                process_command(cmd);
            }
        }
    }
    
    bool process_command(OrderCommand& cmd) {
        auto it = _books.find(cmd.order.get_instrument());
        
        if (it == _books.end()) 
            return false;   
        
        auto& book = it -> second;
        
        switch (cmd.type) {
            case OrderCommand::Type::ADD:
                book.add_order(cmd.order);
                break;

            case OrderCommand::Type::MODIFY:
                book.modify_order(cmd.order.get_id(), cmd.order.get_price(), cmd.order.get_quantity());
                break;

            case OrderCommand::Type::CANCEL:
                book.cancel_order(cmd.order.get_id());
                break;
        }
        
        return true;   
    }
    
private:
    SPSCQueue<OrderCommand>* _queue;
    std::unordered_map<Instrument, OrderBook> _books;
};
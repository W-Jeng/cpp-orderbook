#pragma once

#include <unordered_map>
#include <vector>
#include <spsc_queue.h>
#include <order.h>
#include <orderbook.h>
#include <core.h>
#include <order_routing_system.h>

class alignas(CACHE_LINE_SIZE) Worker {
public:
    Worker(WorkerContext* worker_context):
        _queue(&(worker_context -> queue))
    {
        for (auto& book: worker_context -> orderbooks) {
            const Instrument instrument = book.get_instrument();
            _books.try_emplace(instrument, std::move(book));
        }
    }
    
    void run() {
        OrderCommand cmd{};
        using clock = std::chrono::high_resolution_clock;
        std::chrono::duration<double> elapsed;
        
        while (true) {
            if (_queue -> pop(cmd)) {
                // poison pill to break
                if (cmd.type == OrderCommand::Type::SHUTDOWN)
                    break;
                
                // currently, we only hand those that are successful
                process_command(cmd);
            } 
        }

        for (const auto& [instrument, ob]: _books) {
            std::cout << "time to remove: " << ob.elapsed.count() << "\n";
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
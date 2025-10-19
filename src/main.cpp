#include <iostream>
#include <print>
#include <thread>
#include <csignal>
#include <atomic>
#include <unordered_set>
#include <order.h>
#include <orderbook.h>
#include <id_allocator.h>
#include <book_map.h>
#include <worker.h>
#include <core.h>
#include <order_routing_system.h>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        shutdown_requested.store(true);
        std::cout << "\nReceived Ctrl+C, shutting down....\n";
    }
}

int main() {
    const size_t NUM_ORDERS = 1'000'000;
    const size_t NUM_WORKERS = 4;
    const size_t NUM_INSTRUMENTS = 4;
    
    // ensure that the consumer loads all orders
    const size_t QUEUE_CAP = NUM_ORDERS + 1; 

    std::vector<std::thread> worker_threads;
    worker_threads.reserve(NUM_WORKERS);
    std::vector<Instrument> instruments;
    
    for (size_t i = 0; i < NUM_INSTRUMENTS; ++i) {
        instruments.push_back("S" + std::to_string(i));
    }
    
    IdAllocator id_allocator;
    OrderRoutingSystem order_routing_sys = build_order_routing_system(
        instruments,
        id_allocator,
        NUM_WORKERS,
        QUEUE_CAP
    );
    
    for (size_t i = 0; i < NUM_WORKERS; ++i) {
        worker_threads.emplace_back([&, i]() mutable {
            Worker worker(
                &order_routing_sys.queues[i], 
                std::move(order_routing_sys.worker_orderbooks[i])
            );
            worker.run();
        });
    }
    
    std::signal(SIGINT, signal_handler);
    Producer producer(order_routing_sys.queues, order_routing_sys.instrument_to_worker);
    std::vector<OrderCommand> order_commands;
    order_commands.reserve(QUEUE_CAP);
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        Order order{instruments[i % NUM_INSTRUMENTS], OrderSide::BUY, 100.01, 200};
        order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
    }
    
    // wait for threads to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // start the basic benchmark now
    using clock = std::chrono::high_resolution_clock;
    auto start = clock::now();

    for (auto& order_cmd: order_commands) {
        producer.submit(order_cmd);   
    }
    
    // std::cout << "Shutdown noted. Stopping workers by sending a poison pill...\n";
    std::unordered_set<int> shutdown_message_sent;
    
    while (shutdown_message_sent.size() != NUM_WORKERS) {
        for (int i = 0; i < order_routing_sys.queues.size(); ++i) {
            if (shutdown_message_sent.find(i) != shutdown_message_sent.end()) {
                // we have sent the poison pill
                continue;   
            }

            OrderCommand shutdown_cmd;
            shutdown_cmd.type = OrderCommand::Type::SHUTDOWN;
            auto& queue = order_routing_sys.queues[i];

            if (queue.push(shutdown_cmd)) 
                shutdown_message_sent.insert(i);
        }
    }

    for (auto& t: worker_threads) {
        if (t.joinable())
            t.join();
    }

    auto end = clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";
    std::cout << "All workers stopped cleanly.\n";
    return 0;
}
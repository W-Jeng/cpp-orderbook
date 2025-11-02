#include <iostream>
#include <print>
#include <thread>
#include <csignal>
#include <atomic>
#include <unordered_set>
#include <order.h>
#include <orderbook.h>
#include <id_allocator.h>
#include <worker.h>
#include <core.h>
#include <order_routing_system.h>
#include <producer.h>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        shutdown_requested.store(true);
        std::cout << "\nReceived Ctrl+C, shutting down....\n";
    }
}

int main() {
    const size_t NUM_ORDERS = 20;
    const size_t NUM_WORKERS = 1;
    const size_t NUM_INSTRUMENTS = 1;
    
    // ensure that the consumer loads all orders
    const size_t QUEUE_CAP = NUM_ORDERS + 2; 

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
        QUEUE_CAP,
        NUM_ORDERS
    );
    
    Producer producer(order_routing_sys);
    std::vector<OrderCommand> order_commands;
    order_commands.reserve(QUEUE_CAP);
    
    // we want trade match of around 20% of the orders sent
    for (int i = 0; i < NUM_ORDERS; ++i) {
        if (i % 2 == 0) {
            double price = static_cast<double>((i/2 % 5) + 6);
            Order order{instruments[i % NUM_INSTRUMENTS], OrderSide::BUY, price, 200};
            order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
        } else {
            double price = static_cast<double>(((i-1)/2 % 5) + 10);
            Order order{instruments[i % NUM_INSTRUMENTS], OrderSide::SELL, price, 200};
            order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
        }
    }
    
    // submit and populate the queues first
    for (auto& order_cmd: order_commands) {
        producer.submit(std::move(order_cmd));   
    }

    producer.submit_all_shutdown_commands();
    Worker worker(order_routing_sys.worker_contexts[0].get());
    using clock = std::chrono::high_resolution_clock;

    auto start = clock::now();
    worker.run();
    auto end = clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Total Elapsed time: " << elapsed.count() << " seconds\n";
    std::cout << "All workers stopped cleanly.\n";
    return 0;
}

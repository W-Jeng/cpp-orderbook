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
    using clock = std::chrono::high_resolution_clock;
    const size_t NUM_ORDERS = 1'000'000;
    const size_t NUM_WORKERS = 1;
    const size_t NUM_INSTRUMENTS = 5;
    
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
    
    auto start_add_commands = clock::now();
    int instrument_idx = 0;
    // we want trade match of around 20% of the orders sent
    for (int i = 0; i < NUM_ORDERS; ++i) {
        int j = i % 10;

        if (j == 0) 
            ++instrument_idx;
        
        if (instrument_idx == NUM_INSTRUMENTS) 
            instrument_idx = 0;

        if (j < 5) {
            double price = 6.0 + j; 
            // std::cout << "BUY i: " << i << ", j: " << j << ", price: " << price << std::endl;;
            Order order{instruments[instrument_idx], OrderSide::BUY, price, 1000};
            order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
        } else {
            double price = 5.0 + j;
            // std::cout << "SELL i: " << i << ", j: " << j << ", price: " << price << std::endl;
            Order order{instruments[instrument_idx], OrderSide::SELL, price, 1000};
            order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
        }
    }
    
    // submit and populate the queues first
    for (auto& order_cmd: order_commands) {
        producer.submit(std::move(order_cmd));   
    }

    producer.submit_all_shutdown_commands();
    Worker worker(order_routing_sys.worker_contexts[0].get());

    auto end_add_commands = clock::now();
    std::chrono::duration<double> elapsed_add_commands = end_add_commands - start_add_commands;
    std::cout << "Elapsed seconds Add commands: " << elapsed_add_commands.count() << " seconds\n";
    auto start = clock::now();
    worker.run();
    auto end = clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Total Elapsed time: " << elapsed.count() << " seconds\n";
    std::cout << "All workers stopped cleanly.\n";
    return 0;
}

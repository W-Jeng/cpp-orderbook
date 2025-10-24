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
#include <pthread.h>
#include <sched.h>
#include <producer.h>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        shutdown_requested.store(true);
        std::cout << "\nReceived Ctrl+C, shutting down....\n";
    }
}

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

int main() {
    const size_t NUM_ORDERS = 1'000'000;
    const size_t NUM_WORKERS = 4;
    const size_t NUM_INSTRUMENTS = 4;
    
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
        QUEUE_CAP
    );
    
    
    std::signal(SIGINT, signal_handler);
    Producer producer(order_routing_sys);
    std::vector<OrderCommand> order_commands;
    order_commands.reserve(QUEUE_CAP);
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        Order order{instruments[i % NUM_INSTRUMENTS], OrderSide::BUY, 100.01, 200};
        order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
    }
    
    // wait for threads to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // start the basic benchmark now
    using clock = std::chrono::high_resolution_clock;

    for (auto& order_cmd: order_commands) {
        producer.submit(std::move(order_cmd));   
    }

    auto start = clock::now();

    for (size_t i = 0; i < NUM_WORKERS; ++i) {
        worker_threads.emplace_back([&, i]() mutable {
            pin_to_core(i);
            Worker worker(order_routing_sys.worker_contexts[i].get());
            worker.run();
        });
    }

    auto prod_end = clock::now();
    std::chrono::duration<double> elapsed_prod_submit = prod_end - start;
    
    auto start_submit_shutdown = clock::now();
    producer.submit_all_shutdown_commands();
    auto end_submit_shutdown = clock::now();

    std::chrono::duration<double> elapsed_submit_shutdown = end_submit_shutdown - start_submit_shutdown;

    for (auto& t: worker_threads) {
        if (t.joinable())
            t.join();
    }

    auto end = clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time (Producer submit): " << elapsed_prod_submit.count() << " seconds\n";
    std::cout << "Elapsed time (submit shutdown): " << elapsed_submit_shutdown.count() << " seconds\n";
    std::cout << "Total Elapsed time: " << elapsed.count() << " seconds\n";
    std::cout << "All workers stopped cleanly.\n";
    return 0;
}
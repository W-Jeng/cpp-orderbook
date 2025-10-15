#include <iostream>
#include <print>
#include <order.h>
#include <orderbook.h>
#include <id_allocator.h>
#include <book_map.h>
#include <worker.h>
#include <core.h>
#include <order_routing_system.h>
#include <thread>
#include <csignal>
#include <atomic>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        shutdown_requested.store(true);
        std::cout << "\nReceived Ctrl+C, shutting down....\n";
    }
}

int main() {
    constexpr size_t NUM_WORKERS = 2;
    constexpr size_t QUEUE_CAP = 4096;
    std::vector<std::thread> worker_threads;
    worker_threads.reserve(NUM_WORKERS);
    std::vector<Instrument> instruments = {"AAPL", "MSFT", "TSLA", "GOOG"};
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
                order_routing_sys.queues[i].get(), 
                std::move(order_routing_sys.worker_orderbooks[i])
            );
            worker.run();
        });
    }
    
    std::signal(SIGINT, signal_handler);
    Producer producer(order_routing_sys.queues, order_routing_sys.instrument_to_worker);
    
    // while (!shutdown_requested.load(std::memory_order_relaxed)) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(200));   
        // use producer to push work here, since the pushing will be completed before this loops ends, this is fine
    // }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));   
    using clock = std::chrono::high_resolution_clock;
    auto start = clock::now();
    
    for (int i = 0; i < 1'000'000; ++i) {
        Order order{"MSFT", OrderSide::BUY, 100.01, 200};
        producer.submit(OrderCommand(OrderCommand::Type::ADD, order));
    }
    
    std::cout << "Shutdown noted. Stopping workers by sending a poison pill...\n";
    
    for (auto& queue: order_routing_sys.queues) {
        OrderCommand shutdown_cmd;
        shutdown_cmd.type = OrderCommand::Type::SHUTDOWN;
        queue -> push(shutdown_cmd);   
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
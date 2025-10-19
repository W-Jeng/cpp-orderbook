#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>
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

static void BM_VectorSum(benchmark::State& state) {
    std::vector<int> data(state.range(0), 1);
    for (auto _ : state)
    {
        int sum = std::accumulate(data.begin(), data.end(), 0);
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_BasicOrderBookInsert(benchmark::State& state) {
    const size_t NUM_ORDERS = state.range(0);
    const size_t NUM_WORKERS = state.range(1);
    const size_t NUM_INSTRUMENTS = state.range(2);
    const size_t QUEUE_CAP = NUM_ORDERS + 1;

    std::vector<Instrument> instruments;
    instruments.reserve(NUM_INSTRUMENTS);

    for (size_t i = 0; i < NUM_INSTRUMENTS; ++i)
        instruments.push_back("S" + std::to_string(i));


    for (auto _: state) {
        state.PauseTiming();
        IdAllocator id_allocator;
        OrderRoutingSystem order_routing_sys = build_order_routing_system(
            instruments, 
            id_allocator, 
            NUM_WORKERS, 
            QUEUE_CAP
        );
        std::atomic<size_t> workers_ready{0};
        std::vector<std::thread> worker_threads;

        for (size_t i = 0; i < NUM_WORKERS; ++i) {
            worker_threads.emplace_back([&, i]() mutable {
                Worker worker(
                    &order_routing_sys.queues[i],
                    std::move(order_routing_sys.worker_orderbooks[i])
                );
                workers_ready.fetch_add(1, std::memory_order_release);
                worker.run();
            });
        }

        Producer producer(order_routing_sys.queues, order_routing_sys.instrument_to_worker);
        std::vector<OrderCommand> order_commands;
        order_commands.reserve(NUM_ORDERS);
        std::unordered_set<int> shutdown_message_sent;

        for (int i = 0; i < NUM_ORDERS; ++i) {
            Order order{instruments[i % NUM_INSTRUMENTS], OrderSide::BUY, 100.01, 200};
            order_commands.push_back(OrderCommand(OrderCommand::Type::ADD, order));
        }

        // busy wait until all ready
        while (workers_ready.load(std::memory_order_acquire) != NUM_WORKERS) {
            std::this_thread::yield();
        }

        state.ResumeTiming();

        for (auto& order_cmd : order_commands)
            producer.submit(std::move(order_cmd));

        // Send poison pills after benchmarking
        while (shutdown_message_sent.size() != NUM_WORKERS) {
            for (int i = 0; i < order_routing_sys.queues.size(); ++i) {
                if (shutdown_message_sent.find(i) != shutdown_message_sent.end()) 
                    continue;

                OrderCommand shutdown_cmd;
                shutdown_cmd.type = OrderCommand::Type::SHUTDOWN;
                auto& queue = order_routing_sys.queues[i];

                if (queue.push(shutdown_cmd))
                    shutdown_message_sent.insert(i);
            }
        }

        for (auto& t : worker_threads) {
            if (t.joinable()) 
                t.join();
        }
        state.PauseTiming();
    }
}


// BENCHMARK(BM_VectorSum)->Range(8, 8 << 10);
BENCHMARK(BM_BasicOrderBookInsert) 
    ->Args({1'000, 1, 1})
    ->Args({100'000, 1, 1})
    ->Args({1'000'000, 1, 1})
    ->Args({1'000, 2, 2})
    ->Args({100'000, 2, 2})
    ->Args({1'000'000, 2, 2})
    ->Args({1'000, 4, 4})
    ->Args({100'000, 4, 4})
    ->Args({1'000'000, 4, 4});

BENCHMARK_MAIN();


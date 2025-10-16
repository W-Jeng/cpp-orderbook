#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>

static void BM_VectorSum(benchmark::State& state)
{
    std::vector<int> data(state.range(0), 1);
    for (auto _ : state)
    {
        int sum = std::accumulate(data.begin(), data.end(), 0);
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

BENCHMARK(BM_VectorSum)->Range(8, 8 << 10);

BENCHMARK_MAIN();


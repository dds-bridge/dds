#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"

using namespace std::chrono;

class MicroBenchmark {
private:
    static constexpr int ITERATIONS = 10000;
    static constexpr int WARMUP_ITERATIONS = 100;

public:
    struct BenchmarkResult {
        std::string function_name;
        double avg_microseconds;
        double min_microseconds;
        double max_microseconds;
        long total_iterations;
    };

    std::vector<BenchmarkResult> RunAllMicroBenchmarks() {
        std::vector<BenchmarkResult> results;
        
        std::cout << "=== Heuristic Function Micro-Benchmarks ===" << std::endl;
        std::cout << "Iterations per test: " << ITERATIONS << std::endl;
        std::cout << "Warmup iterations: " << WARMUP_ITERATIONS << std::endl << std::endl;

        // Test each individual heuristic function
        results.push_back(BenchmarkFunction("WeightAllocTrump0", [this](HeuristicContext& ctx) { WeightAllocTrump0(ctx); }));
        results.push_back(BenchmarkFunction("WeightAllocNT0", [this](HeuristicContext& ctx) { WeightAllocNT0(ctx); }));
        results.push_back(BenchmarkFunction("WeightAllocTrumpNotvoid1", [this](HeuristicContext& ctx) { WeightAllocTrumpNotvoid1(ctx); }));
        results.push_back(BenchmarkFunction("WeightAllocNTNotvoid1", [this](HeuristicContext& ctx) { WeightAllocNTNotvoid1(ctx); }));
        results.push_back(BenchmarkFunction("WeightAllocTrumpVoid1", [this](HeuristicContext& ctx) { WeightAllocTrumpVoid1(ctx); }));
        results.push_back(BenchmarkFunction("WeightAllocNTVoid1", [this](HeuristicContext& ctx) { WeightAllocNTVoid1(ctx); }));
        
        return results;
    }

private:
    template<typename Func>
    BenchmarkResult BenchmarkFunction(const std::string& name, Func function) {
        // Setup context once
        HeuristicContext context = CreateTestContext();
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            function(context);
        }

        // Actual benchmark
        std::vector<double> timings;
        timings.reserve(ITERATIONS);

        for (int i = 0; i < ITERATIONS; i++) {
            // Reset weights
            ResetContext(context);
            
            auto start = high_resolution_clock::now();
            function(context);
            auto end = high_resolution_clock::now();
            
            auto duration = duration_cast<nanoseconds>(end - start).count();
            timings.push_back(duration / 1000.0); // Convert to microseconds
        }

        // Calculate statistics
        double avg = std::accumulate(timings.begin(), timings.end(), 0.0) / timings.size();
        double min_time = *std::min_element(timings.begin(), timings.end());
        double max_time = *std::max_element(timings.begin(), timings.end());

        BenchmarkResult result = { name, avg, min_time, max_time, ITERATIONS };
        PrintResult(result);
        return result;
    }

    HeuristicContext CreateTestContext() {
        // Create persistent test data
        moves_.resize(7);
        for (int i = 0; i < 7; i++) {
            moves_[i].suit = i % 4;
            moves_[i].rank = 2 + (i % 13);
            moves_[i].weight = 0;
            moves_[i].sequence = i + 1;
        }

        // Create context structures
        tpos_ = {};
        bestMove_ = {};
        bestMoveTT_ = {};
        thrp_rel_[0] = {};
        track_ = {};

        return HeuristicContext {
            tpos_,
            bestMove_,
            bestMoveTT_,
            thrp_rel_,
            moves_.data(),
            7,    // numMoves
            0,    // lastNumMoves
            1,    // trump
            0,    // suit
            &track_,
            1,    // currTrick
            0,    // currHand
            0,    // leadHand
            0     // leadSuit
        };
    }

    void ResetContext(HeuristicContext& context) {
        // Reset move weights
        for (int i = 0; i < context.numMoves; i++) {
            context.mply[i].weight = 0;
        }
    }

    void PrintResult(const BenchmarkResult& result) {
        std::cout << "Function: " << result.function_name << std::endl;
        std::cout << "  Average: " << std::fixed << std::setprecision(3) 
                  << result.avg_microseconds << " μs" << std::endl;
        std::cout << "  Min:     " << result.min_microseconds << " μs" << std::endl;
        std::cout << "  Max:     " << result.max_microseconds << " μs" << std::endl;
        std::cout << "  Iterations: " << result.total_iterations << std::endl << std::endl;
    }

    // Member variables for persistent test data
    std::vector<moveType> moves_;
    pos tpos_;
    moveType bestMove_;
    moveType bestMoveTT_;
    relRanksType thrp_rel_[1];
    trackType track_;
};

int main() {
    MicroBenchmark benchmark;
    auto results = benchmark.RunAllMicroBenchmarks();
    
    std::cout << "=== Micro-Benchmark Summary ===" << std::endl;
    double total_avg = 0.0;
    for (const auto& result : results) {
        total_avg += result.avg_microseconds;
        std::cout << result.function_name << ": " << std::fixed << std::setprecision(3) 
                  << result.avg_microseconds << " μs" << std::endl;
    }
    
    std::cout << "\nOverall Average: " << (total_avg / results.size()) << " μs" << std::endl;
    
    return 0;
}

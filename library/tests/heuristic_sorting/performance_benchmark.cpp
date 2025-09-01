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

class PerformanceBenchmark {
private:
    static constexpr int ITERATIONS = 100000;
    static constexpr int WARMUP_ITERATIONS = 1000;

    // Test data structures
    struct TestScenario {
        int trump_suit;
        int leader;
        int num_moves;
        std::string description;
    };

    std::vector<TestScenario> test_scenarios_ = {
        {0, 0, 3, "Small_Trump_Leading"},      // 3 moves, trump, leading
        {0, 1, 3, "Small_Trump_Following"},    // 3 moves, trump, following  
        {4, 0, 7, "Medium_NT_Leading"},        // 7 moves, no-trump, leading
        {4, 1, 7, "Medium_NT_Following"},      // 7 moves, no-trump, following
        {1, 0, 13, "Large_Trump_Leading"},     // 13 moves, trump, leading
        {1, 1, 13, "Large_Trump_Following"},   // 13 moves, trump, following
    };

public:
    struct BenchmarkResult {
        std::string scenario;
        double avg_microseconds;
        double min_microseconds;
        double max_microseconds;
        double std_dev;
        long total_iterations;
    };

    std::vector<BenchmarkResult> RunAllBenchmarks() {
        std::vector<BenchmarkResult> results;
        
        std::cout << "=== Heuristic Sorting Performance Benchmark ===" << std::endl;
        std::cout << "Iterations per test: " << ITERATIONS << std::endl;
        std::cout << "Warmup iterations: " << WARMUP_ITERATIONS << std::endl << std::endl;

        for (const auto& scenario : test_scenarios_) {
            auto result = BenchmarkScenario(scenario);
            results.push_back(result);
            PrintResult(result);
        }

        return results;
    }

private:
    BenchmarkResult BenchmarkScenario(const TestScenario& scenario) {
        // Warmup
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            RunSingleHeuristicCall(scenario);
        }

        // Actual benchmark
        std::vector<double> timings;
        timings.reserve(ITERATIONS);

        for (int i = 0; i < ITERATIONS; i++) {
            auto start = high_resolution_clock::now();
            RunSingleHeuristicCall(scenario);
            auto end = high_resolution_clock::now();
            
            auto duration = duration_cast<nanoseconds>(end - start).count();
            timings.push_back(duration / 1000.0); // Convert to microseconds
        }

        // Calculate statistics
        double avg = std::accumulate(timings.begin(), timings.end(), 0.0) / timings.size();
        double min_time = *std::min_element(timings.begin(), timings.end());
        double max_time = *std::max_element(timings.begin(), timings.end());
        
        // Standard deviation
        double variance = 0.0;
        for (double time : timings) {
            variance += (time - avg) * (time - avg);
        }
        variance /= timings.size();
        double std_dev = std::sqrt(variance);

        return {
            scenario.description,
            avg,
            min_time,
            max_time,
            std_dev,
            ITERATIONS
        };
    }

    void RunSingleHeuristicCall(const TestScenario& scenario) {
        // Create test data using HeuristicContext
        std::vector<moveType> moves(scenario.num_moves);
        
        // Generate deterministic but varied suit/rank data
        for (int i = 0; i < scenario.num_moves; i++) {
            moves[i].suit = i % 4;  // Cycle through suits
            moves[i].rank = 2 + (i % 13);  // Cycle through ranks 2-14
            moves[i].weight = 0;  // Initialize weight
            moves[i].sequence = i + 1;  // Sequence
        }

        // Create minimal context structures
        pos tpos = {};
        moveType bestMove = {};
        moveType bestMoveTT = {};
        relRanksType thrp_rel[1] = {};
        trackType track = {};

        // Build HeuristicContext
        HeuristicContext context {
            tpos,
            bestMove,
            bestMoveTT,
            thrp_rel,
            moves.data(),
            scenario.num_moves,
            0,   // lastNumMoves
            scenario.trump_suit,
            0,   // suit
            &track,
            1,   // currTrick
            0,   // currHand
            scenario.leader,  // leadHand
            0    // leadSuit
        };

        // Call the heuristic function
        SortMoves(context);
    }

    void PrintResult(const BenchmarkResult& result) {
        std::cout << "Scenario: " << result.scenario << std::endl;
        std::cout << "  Average: " << std::fixed << std::setprecision(3) 
                  << result.avg_microseconds << " μs" << std::endl;
        std::cout << "  Min:     " << result.min_microseconds << " μs" << std::endl;
        std::cout << "  Max:     " << result.max_microseconds << " μs" << std::endl;
        std::cout << "  Std Dev: " << result.std_dev << " μs" << std::endl;
        std::cout << "  Iterations: " << result.total_iterations << std::endl << std::endl;
    }
};

int main() {
    PerformanceBenchmark benchmark;
    auto results = benchmark.RunAllBenchmarks();
    
    std::cout << "=== Summary ===" << std::endl;
    double total_avg = 0.0;
    for (const auto& result : results) {
        total_avg += result.avg_microseconds;
        std::cout << result.scenario << ": " << std::fixed << std::setprecision(3) 
                  << result.avg_microseconds << " μs" << std::endl;
    }
    
    std::cout << "\nOverall Average: " << (total_avg / results.size()) << " μs" << std::endl;
    
    return 0;
}

#include <iostream>
#include <cstdint>
#include <x86intrin.h>
#include <chrono>
#include <vector>
#include <tuple>
#include <thread>
#include <sched.h>

constexpr double CPU_FREQUENCY_GHZ = 2.495947; // Initial CPU frequency in GHz

void set_cpu_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "Failed to set CPU affinity. Measurements may be less accurate.\n";
    }
}

void tick_every(int milli, int num_ticks) {
    double tick_time = milli / 1000.0;
    double calibrated_frequency = CPU_FREQUENCY_GHZ; // Start with the initial CPU frequency
    unsigned aux;

    // Preallocate a vector to store results
    std::vector<std::tuple<int, double, double, double, uint64_t, uint64_t>> results; 
    results.reserve(num_ticks);

    // Record the base start time and cycle count
    uint64_t base_start_cycles = __rdtscp(&aux);
    auto base_start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < num_ticks; ++i) {
        uint64_t cycles_per_tick = static_cast<uint64_t>(calibrated_frequency * 1e9 * tick_time);
        uint64_t target_cycles = base_start_cycles + (i + 1) * cycles_per_tick; // Expected cycles for this tick
        uint64_t current_cycles;

        // Busy-wait until the target cycle count is reached
        do {
            current_cycles = __rdtscp(&aux);
            if (current_cycles < target_cycles - 50) { // Avoid excessive polling
                _mm_pause(); // Pause instruction to reduce power and resource contention
            }
        } while (current_cycles < target_cycles);

        // Record actual time
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> elapsed = current_time - base_start_time;

        double expected_time = (i + 1) * milli; // Expected time in ms
        double jitter = elapsed.count() - expected_time; // Deviation from expected time

        // Recalibrate CPU frequency based on observed timing
        if (i > 0) { // Avoid recalibration for the first tick
            double observed_tick_time = elapsed.count() / (i + 1); // Average time per tick in ms
            double observed_frequency = (observed_tick_time > 0)
                ? (cycles_per_tick / (observed_tick_time * 1e6)) // GHz
                : calibrated_frequency;
            calibrated_frequency = (calibrated_frequency * 0.9) + (observed_frequency * 0.1); // Smooth adjustment
        }

        // Store results in the vector
        results.emplace_back(i + 1, elapsed.count(), expected_time, jitter, target_cycles, current_cycles);
    }

    // Output the results after the loop
    for (const auto& [tick, elapsed, expected, jitter, target_cycles, actual_cycles] : results) {
        std::cout << "Tick " << tick << ": "
                  << "Elapsed: " << elapsed << " ms, "
                  << "Expected: " << expected << " ms, "
                  << "Jitter: " << jitter << " ms"
                  << ", Target Cycles: " << target_cycles
                  << ", Actual Cycles: " << actual_cycles
                  << std::endl;
    }
}

int main() {
    // Bind the process to a single core to minimize context switching
    set_cpu_affinity(0);

    // Run the tick function
    tick_every(10, 5000);

    return 0;
}

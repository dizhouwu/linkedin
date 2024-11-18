#include <iostream>
#include <cstdint>
#include <x86intrin.h>
#include <chrono>

constexpr double CPU_FREQUENCY_GHZ = 2.49595; // Set your CPU frequency in GHz


void tick_every(int milli) {
    const uint64_t cycles_per_tick = static_cast<uint64_t>(CPU_FREQUENCY_GHZ * 1e9 * 0.1); 
    unsigned aux;

    // Record the base start time and cycle count
    uint64_t base_start_cycles = __rdtscp(&aux);
    auto base_start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) { // Limit to 10 ticks for testing
        uint64_t target_cycles = base_start_cycles + (i + 1) * cycles_per_tick; // Expected cycles for this tick
        uint64_t current_cycles;

        // Busy-wait until the target cycle count is reached
        do {
            current_cycles = __rdtscp(&aux);
        } while (current_cycles < target_cycles);

        // Record actual time
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = current_time - base_start_time;

        double expected_time = (i + 1) * milli; // Expected time in ms
        double jitter = elapsed.count() - expected_time; // Deviation from expected time

        std::cout << "Tick " << i + 1 << ": "
                  << "Elapsed: " << elapsed.count() << " ms, "
                  << "Expected: " << expected_time << " ms, "
                  << "Jitter: " << jitter << " ms"
                  << ", Target Cycles: " << target_cycles
                  << ", Actual Cycles: " << current_cycles
                  << std::endl;
    }
}

int main() {
    tick_every(100);
    return 0;
}

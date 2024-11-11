#include <iostream>
#include <variant>
#include <vector>
#include <chrono>
#include <cmath>
#include <numeric>
#include <algorithm>

// Operation classes for virtual polymorphism
struct IOperation {
    virtual void execute() const = 0;
    virtual ~IOperation() = default;
};

struct MoveOperation : public IOperation {
    double x, y;
    MoveOperation(double x, double y) : x(x), y(y) {}
    void execute() const override {
        double result = x + y;  // simulate work
    }
};

struct RotateOperation : public IOperation {
    double angle;
    RotateOperation(double angle) : angle(angle) {}
    void execute() const override {
        double radians = angle * 3.141592653589793 / 180.0;  // simulate work
    }
};

struct ScaleOperation : public IOperation {
    double factor;
    ScaleOperation(double factor) : factor(factor) {}
    void execute() const override {
        double result = factor * factor;  // simulate work
    }
};

// Variant-based polymorphism
struct Move { double x, y; };
struct Rotate { double angle; };
struct Scale { double factor; };

using OperationVariant = std::variant<Move, Rotate, Scale>;

void executeOperation(const OperationVariant& op) {
    std::visit([](const auto& operation) {
        if constexpr (std::is_same_v<decltype(operation), const Move&>) {
            double result = operation.x + operation.y;  // simulate work
        } else if constexpr (std::is_same_v<decltype(operation), const Rotate&>) {
            double radians = operation.angle * 3.141592653589793 / 180.0;  // simulate work
        } else if constexpr (std::is_same_v<decltype(operation), const Scale&>) {
            double result = operation.factor * operation.factor;  // simulate work
        }
    }, op);
}

// Benchmark function with result storage
template <typename Func>
long long benchmark(Func func, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

// Perform a t-test to check if two sets of benchmark results are significantly different
bool t_test(const std::vector<long long>& sample1, const std::vector<long long>& sample2) {
    int n1 = sample1.size();
    int n2 = sample2.size();

    double mean1 = std::accumulate(sample1.begin(), sample1.end(), 0.0) / n1;
    double mean2 = std::accumulate(sample2.begin(), sample2.end(), 0.0) / n2;

    double variance1 = std::accumulate(sample1.begin(), sample1.end(), 0.0, 
                       [mean1](double sum, long long val) { return sum + (val - mean1) * (val - mean1); }) / (n1 - 1);
    double variance2 = std::accumulate(sample2.begin(), sample2.end(), 0.0, 
                       [mean2](double sum, long long val) { return sum + (val - mean2) * (val - mean2); }) / (n2 - 1);

    double t_stat = std::abs(mean1 - mean2) / std::sqrt((variance1 / n1) + (variance2 / n2));
    double critical_value = 2.101;  // Approximate value for a 95% confidence level with large sample size

    return t_stat > critical_value;
}

int main() {
    constexpr int iterations = 1000000;
    constexpr int trials = 100;

    // Store results for each trial
    std::vector<long long> virtual_durations;
    std::vector<long long> visit_durations;

    // Run multiple trials for virtual function-based polymorphism
    for (int trial = 0; trial < trials; ++trial) {
        std::vector<IOperation*> virtualOperations;
        for (int i = 0; i < iterations / 3; ++i) {
            virtualOperations.push_back(new MoveOperation(10.0, 20.0));
            virtualOperations.push_back(new RotateOperation(90.0));
            virtualOperations.push_back(new ScaleOperation(1.5));
        }

        virtual_durations.push_back(benchmark([&]() {
            for (const auto* op : virtualOperations) {
                op->execute();
            }
        }, iterations));

        for (auto* op : virtualOperations) {
            delete op;
        }
    }

    // Run multiple trials for variant-based polymorphism
    for (int trial = 0; trial < trials; ++trial) {
        std::vector<OperationVariant> variantOperations;
        for (int i = 0; i < iterations / 3; ++i) {
            variantOperations.emplace_back(Move{10.0, 20.0});
            variantOperations.emplace_back(Rotate{90.0});
            variantOperations.emplace_back(Scale{1.5});
        }

        visit_durations.push_back(benchmark([&]() {
            for (const auto& op : variantOperations) {
                executeOperation(op);
            }
        }, iterations));
    }

    // Calculate mean durations
    double mean_virtual = std::accumulate(virtual_durations.begin(), virtual_durations.end(), 0.0) / trials;
    double mean_visit = std::accumulate(visit_durations.begin(), visit_durations.end(), 0.0) / trials;

    // Display results
    std::cout << "Average virtual function polymorphism: " << mean_virtual << " microseconds\n";
    std::cout << "Average std::visit-based polymorphism: " << mean_visit << " microseconds\n";

    // Perform t-test to check statistical significance
    if (t_test(virtual_durations, visit_durations)) {
        std::cout << "The performance difference is statistically significant.\n";
    } else {
        std::cout << "The performance difference is NOT statistically significant.\n";
    }

    return 0;
}

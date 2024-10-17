#include <iostream>
#include <vector>

// Forward declaration of the Derived class
template <typename Derived>
class TradingStrategy {
public:
    // Public interface to calculate signals, delegates to the derived class
    void execute_strategy() {
        double signal = static_cast<Derived*>(this)->generate_signal();
        if (signal > 0) {
            std::cout << "Buy signal detected. Signal value: " << signal << std::endl;
            execute_trade("BUY", signal);
        } else if (signal < 0) {
            std::cout << "Sell signal detected. Signal value: " << signal << std::endl;
            execute_trade("SELL", signal);
        } else {
            std::cout << "No trade signal." << std::endl;
        }
    }

protected:
    // A common function in the base class to execute the trade
    void execute_trade(const std::string& direction, double size) {
        std::cout << "Executing " << direction << " order with size: " << size << std::endl;
    }
};


class MomentumStrategy : public TradingStrategy<MomentumStrategy> {
public:
    double generate_signal() {
        // A simple example of momentum calculation
        double price_change = price_history.back() - price_history.front();
        return price_change;  // Buy if positive, sell if negative
    }

    void set_price_history(const std::vector<double>& prices) {
        price_history = prices;
    }

private:
    std::vector<double> price_history;
};

class MeanReversionStrategy : public TradingStrategy<MeanReversionStrategy> {
public:
    double generate_signal() {
        double mean = 0;
        for (double price : price_history) {
            mean += price;
        }
        mean /= price_history.size();

        double current_price = price_history.back();
        double deviation = current_price - mean;

        // Buy if the price is below the mean (expecting it to revert up)
        // Sell if the price is above the mean (expecting it to revert down)
        return -deviation;  
    }

    void set_price_history(const std::vector<double>& prices) {
        price_history = prices;
    }

private:
    std::vector<double> price_history;
};

int main() {
    // Momentum Strategy Example
    MomentumStrategy momentum_strategy;
    std::vector<double> prices = {100, 102, 104, 105, 107};
    momentum_strategy.set_price_history(prices);
    momentum_strategy.execute_strategy();  // Should output a Buy signal

    // Mean Reversion Strategy Example
    MeanReversionStrategy mean_reversion_strategy;
    mean_reversion_strategy.set_price_history(prices);
    mean_reversion_strategy.execute_strategy();  // Should output a Sell signal
}

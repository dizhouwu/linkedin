#include <iostream>
#include <variant>
#include <vector>
#include <cmath>
#include <numbers>

// Command types with specific data
struct Move {
    double x, y;
    void execute() const { std::cout << "Moving to (" << x << ", " << y << ")\n"; }
};

struct Rotate {
    double angle;  // Angle in degrees
    void execute() const { std::cout << "Rotating by " << angle << " degrees\n"; }
};

struct Scale {
    double factor;
    void execute() const { std::cout << "Scaling by factor " << factor << "\n"; }
};

// Helper structure for the overloaded pattern at namespace scope
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;  // Deduction guide

// Processor class with variant-based polymorphism
class Processor {
public:
    using CommandVariant = std::variant<Move, Rotate, Scale>;

    void processCommand(const CommandVariant& command) const {
        std::visit(Overloaded {
            [](const Move& move) { 
                std::cout << "Processing Move Command\n"; 
                move.execute(); 
            },
            [](const Rotate& rotate) { 
                std::cout << "Processing Rotate Command\n"; 
                double radians = rotate.angle * std::numbers::pi / 180.0;
                std::cout << "Calculated radians: " << radians << "\n";
                rotate.execute(); 
            },
            [](const Scale& scale) { 
                std::cout << "Processing Scale Command\n"; 
                std::cout << "Square of factor: " << scale.factor * scale.factor << "\n";
                scale.execute(); 
            }
        }, command);
    }

    void processAllCommands(const std::vector<CommandVariant>& commands) const {
        for (const auto& command : commands) {
            processCommand(command);
        }
    }
};

int main() {
    Processor processor;

    std::vector<Processor::CommandVariant> commands = {
        Move{10, 20},
        Rotate{90},
        Scale{1.5},
        Move{15, 30},
        Rotate{45},
        Scale{2.0}
    };

    processor.processAllCommands(commands);

    return 0;
}

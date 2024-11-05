#include <iostream>
#include <memory>
#include <unordered_map>
#include <string>
#include <stdexcept>

// Base class for expressions
class Expression {
public:
    virtual std::string toString() const = 0; // Convert expression to string
    virtual int evaluate() const = 0; // Evaluate expression
    virtual ~Expression() = default;
};

// Class for constants
class Constant : public Expression {
    int value;

public:
    Constant(int v) : value(v) {}
    std::string toString() const override {
        return std::to_string(value);
    }
    int evaluate() const override {
        return value;
    }
};

// Class for variables
class Variable : public Expression {
    std::string name;
    static std::unordered_map<std::string, int> variableMap;

public:
    Variable(const std::string& n) : name(n) {}

    std::string toString() const override {
        return name;
    }
    int evaluate() const override {
        if (variableMap.find(name) == variableMap.end()) {
            throw std::runtime_error("Cannot evaluate a variable without a value.");
        }
        return variableMap[name];
    }

    static void setVariableValue(const std::string& name, int value) {
        variableMap[name] = value;
    }
    static void displayVariableMap() {
        std::cout << "Variable Map Contents:" << std::endl;
        for (const auto& pair : variableMap) {
            std::cout << "Variable: " << pair.first << ", Value: " << pair.second << std::endl;
        }
    }
};

// Define static member
std::unordered_map<std::string, int> Variable::variableMap;

// Class for binary operations
class BinaryOperation : public Expression {
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    std::string op;

public:
    BinaryOperation(std::shared_ptr<Expression> l, std::shared_ptr<Expression> r, const std::string& operation)
        : left(l), right(r), op(operation) {}

    std::string toString() const override {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }

    int evaluate() const override {
        if (op == "+") {
            return left->evaluate() + right->evaluate();
        } else if (op == "-") {
            return left->evaluate() - right->evaluate();
        } else if (op == "*") {
            return left->evaluate() * right->evaluate();
        } else if (op == "/") {
            return left->evaluate() / right->evaluate();
        }
        throw std::invalid_argument("Invalid operation.");
    }
};

// Value numbering map, where the key is the unique expression string
std::unordered_map<std::string, std::shared_ptr<Expression>> valueNumberMap;

std::string createVNKey(const std::shared_ptr<Expression>& left, const std::shared_ptr<Expression>& right, const std::string& operation) {
    if (operation == "+" || operation == "*") {
        // For commutative operations, sort operands lexicographically
        std::string leftKey = left->toString();
        std::string rightKey = right->toString();
        if (leftKey > rightKey) std::swap(leftKey, rightKey);
        return leftKey + " " + operation + " " + rightKey;
    }
    // For non-commutative operations, keep the original order
    return left->toString() + " " + operation + " " + right->toString();
}

void displayValueNumberMap() {
    std::cout << "Current valueNumberMap contents:" << std::endl;
    for (const auto& pair : valueNumberMap) {
        std::cout << "Expression: " << pair.first << ", Value: " << pair.second->evaluate() << std::endl;
    }
}

std::shared_ptr<Expression> processBinaryOperation(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, const std::string& operation) {
    std::string key = createVNKey(left, right, operation); // Unique key for expression

    // Check if the expression has already been computed
    if (valueNumberMap.find(key) != valueNumberMap.end()) {
        return valueNumberMap[key]; // Return cached result
    }

    // Create new BinaryOperation and store in the map
    auto binOp = std::make_shared<BinaryOperation>(left, right, operation);
    valueNumberMap[key] = binOp;

    return binOp; // Return the newly created binary operation
}

int main() {
    // Define variables and constants
    Variable::setVariableValue("a", 5);
    auto a = std::make_shared<Variable>("a");
    auto b = processBinaryOperation(a, std::make_shared<Constant>(10), "+"); // Compute b

    if (b->evaluate() > 10) {
        auto d = processBinaryOperation(b, std::make_shared<Constant>(2), "*"); // Uses b
        std::cout << "d: " << d->evaluate() << std::endl;
    }

    // Another block
    if (b->evaluate() < 20) { // Reusing b
        auto e = processBinaryOperation(b, std::make_shared<Constant>(2), "*"); // Reuse b
        std::cout << "e: " << e->evaluate() << std::endl;
    }

    displayValueNumberMap();
    Variable::displayVariableMap();

    return 0;
}
#include <iostream>
#include <array>
#include <chrono>

// Matrix size constants for simplicity
constexpr size_t N = 3; // 3x3 matrices


// Base expression class
template <typename E>
class Expression {
public:
    auto operator()(size_t i, size_t j) const {
        return static_cast<const E&>(*this)(i, j);
    }

    size_t size() const {
        return N;
    }
};

// Matrix class representing a concrete matrix
template <typename T, size_t N>
class Matrix : public Expression<Matrix<T, N>> {
public:
    std::array<std::array<T, N>, N> data;

    Matrix() {
        for (auto& row : data) {
            row.fill(T());
        }
    }

    Matrix(std::initializer_list<std::initializer_list<T>> init) {
        size_t i = 0;
        for (auto& row : init) {
            std::copy(row.begin(), row.end(), data[i++].begin());
        }
    }

    T& operator()(size_t i, size_t j) { return data[i][j]; }
    T operator()(size_t i, size_t j) const { return data[i][j]; }

    // Assignment from any expression
    template <typename E>
    Matrix& operator=(const Expression<E>& expr) {
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                data[i][j] = expr(i, j);
            }
        }
        return *this;
    }
};

// Expression class for matrix addition
template <typename LHS, typename RHS>
class MatrixAdd : public Expression<MatrixAdd<LHS, RHS>> {
public:
    const LHS& lhs;
    const RHS& rhs;

    MatrixAdd(const LHS& lhs, const RHS& rhs) : lhs(lhs), rhs(rhs) {}

    auto operator()(size_t i, size_t j) const {
        return lhs(i, j) + rhs(i, j);
    }
};

// Expression class for matrix multiplication
template <typename LHS, typename RHS>
class MatrixMul : public Expression<MatrixMul<LHS, RHS>> {
public:
    const LHS& lhs;
    const RHS& rhs;

    MatrixMul(const LHS& lhs, const RHS& rhs) : lhs(lhs), rhs(rhs) {}

    auto operator()(size_t i, size_t j) const {
        decltype(lhs(i, j) * rhs(i, j)) result = 0;
        for (size_t k = 0; k < N; ++k) {
            result += lhs(i, k) * rhs(k, j);
        }
        return result;
    }
};

// Operator overloads to create expressions
template <typename LHS, typename RHS>
auto operator+(const Expression<LHS>& lhs, const Expression<RHS>& rhs) {
    return MatrixAdd<LHS, RHS>(static_cast<const LHS&>(lhs), static_cast<const RHS&>(rhs));
}

template <typename LHS, typename RHS>
auto operator*(const Expression<LHS>& lhs, const Expression<RHS>& rhs) {
    return MatrixMul<LHS, RHS>(static_cast<const LHS&>(lhs), static_cast<const RHS&>(rhs));
}

// Direct matrix operations (without expression templates)
template <typename T, size_t N>
void matrix_add(const Matrix<T, N>& A, const Matrix<T, N>& B, Matrix<T, N>& C) {
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            C(i, j) = A(i, j) + B(i, j);
        }
    }
}

template <typename T, size_t N>
void matrix_mul(const Matrix<T, N>& A, const Matrix<T, N>& B, Matrix<T, N>& C) {
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            C(i, j) = 0;
            for (size_t k = 0; k < N; ++k) {
                C(i, j) += A(i, k) * B(k, j);
            }
        }
    }
}

int main() {
    // Define matrices A, B, and D
    Matrix<double, N> A = {{ {1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0} }};
    Matrix<double, N> B = {{ {9.0, 8.0, 7.0}, {6.0, 5.0, 4.0}, {3.0, 2.0, 1.0} }};
    Matrix<double, N> D = {{ {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0} }};
    Matrix<double, N> C;

    // Measure time for direct matrix operations (no expression templates)
    Matrix<double, N> C_direct;

    auto start = std::chrono::high_resolution_clock::now();
    Matrix<double, N> tmp;
    matrix_add(A, B, tmp);
    matrix_mul(tmp, D, C_direct); // Direct evaluation of (A + B) * D
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_direct = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Output results
    std::cout << "Result of (A + B) * D (direct operations):" << std::endl;
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            std::cout << C_direct(i, j) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Time taken with direct matrix operations: " << duration_direct << " nanoseconds" << std::endl;
    // Measure time for expression template evaluation of (A + B) * D
    start = std::chrono::high_resolution_clock::now();
    C = (A + B) * D; // Fused operation using expression templates
    end = std::chrono::high_resolution_clock::now();
    auto duration_expr_template = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Output results
    
    std::cout << "Result of (A + B) * D (expression templates):" << std::endl;
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            std::cout << C(i, j) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Time taken with expression templates: " << duration_expr_template << " nanoseconds" << std::endl;



    return 0;
}

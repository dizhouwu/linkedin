#include <cppyy.h>
#include <iostream>
#include <vector>
using namespace std;

int add(int a, int b) {
    return a + b;
}


int main() {
    cout << "def add(a: int, b: int) -> int:
    return a + b Result: " << add(5, 3) << endl;  // Calling the function for testing
    return 0;
}

#include <iostream>
#include "BigInt.h"
using namespace std;

int main() {
    BigInt x1("9999999999");
    BigInt x2("1");
    cout << x1 << "+" << x2 << " = " << (x1 + x2) << endl;
    BigInt x3("2100000000");
    BigInt x4("2100000000");
    cout << x3 << "+" << x4 << " = " << (x3 += x4) << endl;
    BigInt y1(99);
    BigInt y2("99");
    size_t v = 0;
    BigInt y3(v);
    cout << y1 << "*" << y2 << " = " << (y1 * y2) << endl;
    cout << y1 << "*" << y3 << " = " << (y1 * y3) << endl;
    BigInt y4("9999999999");
    BigInt y5("9999999999");
    cout << y4 << "*" << y5 << " = " << (y4 *= y5) << endl;
    return 0;
}

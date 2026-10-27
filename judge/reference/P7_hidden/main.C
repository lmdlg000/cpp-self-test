#include <iostream>
#include "BigInt.h"
using namespace std;

int main() {
    BigInt a("123");
    a += a;                       // 别名：a = 246
    cout << a << endl;
    BigInt b("99");
    b *= b;                       // 别名：b = 9801
    cout << b << endl;
    BigInt c("007");              // 前导零规范化
    cout << c << endl;
    BigInt d("0000");             // 规范 0
    cout << d << endl;
    BigInt e("12x34");            // 非法字符：取前缀 12（cerr 诊断）
    cout << e << endl;
    BigInt f;
    cout << f << endl;            // 默认构造 0
    BigInt g("9999999999");
    BigInt h("1");
    cout << (g + h) << endl;      // 10000000000
    cout << (g * h) << endl;      // 9999999999
    return 0;
}

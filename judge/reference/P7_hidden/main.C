#include <iostream>
#include "DoubleVector.h"
using namespace std;

int main() {
    DoubleVector a;                        // 默认容量 10
    for (int i = 0; i < 6; ++i) {
        a.push_back(i + 0.5);              // 0.5 ... 5.5
    }
    a += a;                                // 别名：12 个元素，触发扩容
    cout << a.getCount() << ":" << a << endl;

    DoubleVector b(0);                     // 容量 0，必须能自己长出来
    for (int i = 0; i < 3; ++i) {
        b.push_back(-i * 0.25);            // 0, -0.25, -0.5
    }
    cout << b.getCount() << ":" << b << endl;

    DoubleVector e;                        // 空数组
    cout << e.getCount() << ":" << e << endl;

    DoubleVector c;
    c = c;                                 // 自赋值
    cout << c.getCount() << ":" << c << endl;

    DoubleVector d = b;                    // 深拷贝
    d.push_back(9.75);
    cout << b.getCount() << ":" << b << endl;
    cout << d.getCount() << ":" << d << endl;

    const DoubleVector k = d;              // const 对象：[] 与 << 必须能调用
    cout << k[3] << "," << k[100] << "," << k[-2] << endl;

    DoubleVector big(1);                   // 反复翻倍扩容
    for (int i = 0; i < 20; ++i) {
        big.push_back(i * 2.0 - 10.0);
    }
    cout << big.getCount() << ":" << big << endl;
    return 0;
}

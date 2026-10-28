#include <iostream>
#include "DoubleVector.h"
using namespace std;

int main() {
    DoubleVector va;                      // 默认容量 10
    for (int i = 0; i < 5; ++i) {
        va.push_back(i * 0.5);            // 0, 0.5, 1, 1.5, 2
    }
    DoubleVector vb(3);                   // 容量 3，元素 0 个
    for (int i = 0; i < 8; ++i) {
        vb.push_back(i * 1.25 - 2.0);     // -2, -0.75, 0.5, 1.75, 3, 4.25, 5.5, 6.75
    }
    cout << va.getCount() << "," << vb.getCount() << endl;
    cout << va[2] << "," << va[10] << "," << va[-1] << endl;
    DoubleVector vc;
    vc = va;
    vc += vb;
    cout << vc.getCount() << ":" << vc << endl;
    DoubleVector vd = vc;
    vd.push_back(100.25);
    cout << vd.getCount() << ":" << vd << endl;
    cout << vc.getCount() << ":" << vc << endl;
    return 0;
}

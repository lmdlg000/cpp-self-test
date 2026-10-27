// 演示：TLE —— 死循环，永远不会读输入
#include <iostream>
using namespace std;

int main() {
    long long x = 0;
    while (true) {
        x += 1;          // 死循环
    }
    return 0;
}

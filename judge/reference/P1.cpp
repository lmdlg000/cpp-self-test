// P1 参考实现（快递运费结算器）
// 用途：judge.py gen 生成期望输出 + 评测器自测
// 完全遵守 exercise/P1-shipping.md 的接口与输出约定
#include <iostream>
using namespace std;

int shippingFee(int grams);   // 原型（题面给定）

int main() {
    int R;
    cin >> R;                      // 单数
    for (int r = 0; r < R; ++r) {
        int total = 0;             // 本单总运费
        int cnt = 0;               // 本单有效件数
        int maxW = 0;              // 本单最重件的重量
        int maxFee = 0;            // 本单最重件的运费
        int w;
        while (cin >> w) {
            if (w == -1) break;    // 本单结束，-1 不计费
            if (w <= 0 || w > 20000) {
                cerr << "invalid: " << w << endl;
                continue;
            }
            int fee = shippingFee(w);      // 每个合法重量调用一次
            total += fee;
            cnt += 1;
            if (w > maxW) {
                maxW = w;
                maxFee = fee;
            }
        }
        cout << total << " " << cnt << " " << maxFee << endl;
    }
    return 0;
}

int shippingFee(int grams) {
    int fee = 8;
    if (grams > 1000) {
        int extra = (grams - 1000 + 499) / 500;   // 整数向上取整
        fee = 8 + 3 * extra;
    }
    if (fee > 50) fee = 50;
    return fee;
}

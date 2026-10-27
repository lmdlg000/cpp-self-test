// 演示：PE —— 答案内容正确，但行末多了一个空格（空白差异）
#include <iostream>
using namespace std;

int shippingFee(int grams) {
    int fee = 8;
    if (grams > 1000) {
        int extra = (grams - 1000 + 499) / 500;
        fee = 8 + 3 * extra;
    }
    if (fee > 50) fee = 50;
    return fee;
}

int main() {
    int R;
    cin >> R;
    for (int r = 0; r < R; ++r) {
        int total = 0, cnt = 0, maxW = 0, maxFee = 0, w;
        while (cin >> w) {
            if (w == -1) break;
            if (w <= 0 || w > 20000) {
                cerr << "invalid: " << w << endl;
                continue;
            }
            int fee = shippingFee(w);
            total += fee; cnt += 1;
            if (w > maxW) { maxW = w; maxFee = fee; }
        }
        cout << total << " " << cnt << " " << maxFee << " ";   // 行末多一个空格
        cout << endl;
    }
    return 0;
}

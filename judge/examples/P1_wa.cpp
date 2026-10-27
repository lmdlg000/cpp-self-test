// 演示：WA —— 向上取整写错（用了向下取整），封顶也漏了
#include <iostream>
using namespace std;

int shippingFee(int grams) {
    if (grams <= 1000) return 8;
    int extra = (grams - 1000) / 500;   // 错：不足 500g 没有向上取整
    return 8 + 3 * extra;               // 错：没有封顶 50
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
        cout << total << " " << cnt << " " << maxFee << endl;
    }
    return 0;
}

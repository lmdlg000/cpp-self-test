#include <iostream>
#include "shipping.h"
using namespace std;

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

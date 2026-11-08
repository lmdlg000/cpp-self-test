#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

struct Task {
    long long a;   // 需要的最小能量
    long long b;   // 消耗的能量
    int idx;       // 输入编号（1 起）
};

bool better(const Task& x, const Task& y) {
    long long kx = x.a - x.b;
    long long ky = y.a - y.b;
    if (kx != ky) return kx > ky;
    return x.idx < y.idx;
}

int main() {
    int q = 0;
    if (!(cin >> q)) return 0;
    for (int t = 0; t < q; ++t) {
        int n = 0;
        cin >> n;
        vector<Task> v(n);
        for (int i = 0; i < n; ++i) {
            cin >> v[i].a >> v[i].b;
            v[i].idx = i + 1;
        }
        sort(v.begin(), v.end(), better);
        long long need = 0;      // 已消耗的总能量
        long long best = 0;      // 所需初始能量
        for (int i = 0; i < n; ++i) {
            if (v[i].a + need > best) best = v[i].a + need;
            need += v[i].b;
        }
        cout << best << endl;
        for (int i = 0; i < n; ++i) {
            if (i) cout << " ";
            cout << v[i].idx;
        }
        cout << endl;
    }
    return 0;
}

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
using namespace std;

class Puzzle {
public:
    virtual ~Puzzle() {}
    virtual void read(istream& in) = 0;
    virtual void solve() const = 0;
};

// ---------------- 1. 分形边长 ----------------
class FractalPuzzle : public Puzzle {
    unsigned long long n;
public:
    FractalPuzzle() : n(0) {}
    void read(istream& in) { in >> n; }
    void solve() const {
        const long long MOD = 10007;
        long long base = 3 % MOD;
        long long res = 1 % MOD;
        unsigned long long e = n;
        while (e > 0) {
            if (e & 1ULL) res = res * base % MOD;
            base = base * base % MOD;
            e >>= 1;
        }
        cout << res << endl;
    }
};

// ---------------- 2. 三块板拼 4x4 ----------------
typedef pair<int, int> Cell;

class TilingPuzzle : public Puzzle {
    vector<vector<Cell> > pieces;

    static void normalize(vector<Cell>& s) {
        int mr = s[0].first, mc = s[0].second;
        for (size_t i = 1; i < s.size(); ++i) {
            if (s[i].first < mr) mr = s[i].first;
            if (s[i].second < mc) mc = s[i].second;
        }
        for (size_t i = 0; i < s.size(); ++i) {
            s[i].first -= mr;
            s[i].second -= mc;
        }
        sort(s.begin(), s.end());
    }
    static bool sameShape(const vector<Cell>& x, const vector<Cell>& y) {
        if (x.size() != y.size()) return false;
        for (size_t i = 0; i < x.size(); ++i) {
            if (x[i] != y[i]) return false;
        }
        return true;
    }
    static vector<Cell> rotate90(const vector<Cell>& s) {
        vector<Cell> t;
        for (size_t i = 0; i < s.size(); ++i) {
            t.push_back(Cell(s[i].second, -s[i].first));
        }
        normalize(t);
        return t;
    }
    static vector<vector<Cell> > orientations(const vector<Cell>& s) {
        vector<vector<Cell> > out;
        vector<Cell> cur = s;
        normalize(cur);
        for (int k = 0; k < 4; ++k) {
            bool dup = false;
            for (size_t i = 0; i < out.size(); ++i) {
                if (sameShape(out[i], cur)) dup = true;
            }
            if (!dup) out.push_back(cur);
            cur = rotate90(cur);
        }
        return out;
    }
    bool place(int g[4][4], const vector<vector<vector<Cell> > >& ori, int used) const {
        int r = -1, c = -1;
        for (int i = 0; i < 4 && r < 0; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (g[i][j] == 0) { r = i; c = j; break; }
            }
        }
        if (r < 0) return true;
        for (int p = 0; p < 3; ++p) {
            if (used & (1 << p)) continue;
            for (size_t o = 0; o < ori[p].size(); ++o) {
                const vector<Cell>& sh = ori[p][o];
                for (size_t k = 0; k < sh.size(); ++k) {
                    int dr = r - sh[k].first;
                    int dc = c - sh[k].second;
                    bool ok = true;
                    for (size_t q = 0; q < sh.size() && ok; ++q) {
                        int rr = sh[q].first + dr, cc = sh[q].second + dc;
                        if (rr < 0 || rr > 3 || cc < 0 || cc > 3 || g[rr][cc] != 0) ok = false;
                    }
                    if (!ok) continue;
                    for (size_t q = 0; q < sh.size(); ++q) {
                        g[sh[q].first + dr][sh[q].second + dc] = p + 1;
                    }
                    if (place(g, ori, used | (1 << p))) return true;
                    for (size_t q = 0; q < sh.size(); ++q) {
                        g[sh[q].first + dr][sh[q].second + dc] = 0;
                    }
                }
            }
        }
        return false;
    }
public:
    TilingPuzzle() {}
    void read(istream& in) {
        pieces.clear();
        for (int i = 0; i < 3; ++i) {
            int k = 0;
            in >> k;
            vector<Cell> s;
            for (int j = 0; j < k; ++j) {
                int r = 0, c = 0;
                in >> r >> c;
                s.push_back(Cell(r, c));
            }
            pieces.push_back(s);
        }
    }
    void solve() const {
        int total = 0;
        for (int i = 0; i < 3; ++i) total += (int)pieces[i].size();
        if (total != 16) { cout << "NO" << endl; return; }
        vector<vector<vector<Cell> > > ori(3);
        for (int i = 0; i < 3; ++i) ori[i] = orientations(pieces[i]);
        int g[4][4];
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) g[i][j] = 0;
        }
        cout << (place(g, ori, 0) ? "YES" : "NO") << endl;
    }
};

// ---------------- 3. 环形灯泡 ----------------
class BulbPuzzle : public Puzzle {
    int n;
    vector<int> b;
public:
    BulbPuzzle() : n(0) {}
    void read(istream& in) {
        in >> n;
        b.assign(n, 0);
        for (int i = 0; i < n; ++i) in >> b[i];
    }
    void solve() const {
        int best = -1;
        for (int y2 = 0; y2 < 2; ++y2) {
            for (int y1 = 0; y1 < 2; ++y1) {
                vector<int> x(n, 0);
                x[n - 2] = y2;
                x[n - 1] = y1;
                for (int i = 0; i < n - 2; ++i) {
                    int p1 = (i - 1 + n) % n;
                    int p2 = (i - 2 + n) % n;
                    x[i] = b[i] ^ x[p1] ^ x[p2];
                }
                bool ok = true;
                for (int i = n - 2; i < n && ok; ++i) {
                    int p1 = (i - 1 + n) % n;
                    int p2 = (i - 2 + n) % n;
                    if ((x[i] ^ x[p1] ^ x[p2]) != b[i]) ok = false;
                }
                if (!ok) continue;
                int cnt = 0;
                for (int i = 0; i < n; ++i) cnt += x[i];
                if (best < 0 || cnt < best) best = cnt;
            }
        }
        if (best < 0) cout << "NO" << endl;
        else cout << best << endl;
    }
};

int main() {
    int q = 0;
    if (!(cin >> q)) return 0;
    vector<Puzzle*> list;
    for (int t = 0; t < q; ++t) {
        int type = 0;
        cin >> type;
        Puzzle* p = NULL;
        if (type == 1) p = new FractalPuzzle();
        else if (type == 2) p = new TilingPuzzle();
        else p = new BulbPuzzle();
        p->read(cin);
        p->solve();
        list.push_back(p);
    }
    for (size_t i = 0; i < list.size(); ++i) delete list[i];
    return 0;
}

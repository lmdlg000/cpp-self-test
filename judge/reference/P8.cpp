#include <iostream>
#include <cstddef>
#include <cmath>
using namespace std;

class Equation {
    int     deg;    // 次数：规范后 deg >= 0；零多项式为 deg == 0 且 coef[0] == 0
    double* coef;   // coef[0..deg]，coef[0] 是最高次项系数，coef[deg] 是常数项
public:
    Equation();                                 // 零多项式
    Equation(const double* c, int d);           // 从 c[0..d]（高次在前）构造，含规范化
    Equation(const Equation& e);
    Equation& operator=(const Equation& e);
    ~Equation();

    int    getDeg() const;                      // 次数
    double operator[](int k) const;             // x^k 的系数，越界返回 0.0
    double operator()(double x) const;          // 代入求值
    Equation derivative() const;                // 导数
    int    solve(double x0, double& root) const; // 牛顿迭代：成功返回迭代次数，失败返回 -1

    friend ostream& operator<<(ostream& os, const Equation& e);
};

Equation::Equation() : deg(0) {
    coef = new double[1];
    coef[0] = 0.0;
}

Equation::Equation(const double* c, int d) : deg(0) {
    int first = 0;
    while (c != 0 && first <= d && c[first] == 0.0) {
        ++first;
    }
    if (c == 0 || first > d) {          // 全零（或空指针）→ 零多项式
        coef = new double[1];
        coef[0] = 0.0;
        return;
    }
    deg = d - first;
    coef = new double[deg + 1];
    for (int i = 0; i <= deg; ++i) {
        coef[i] = c[first + i];
    }
}

Equation::Equation(const Equation& e) : deg(e.deg) {
    coef = new double[deg + 1];
    for (int i = 0; i <= deg; ++i) {
        coef[i] = e.coef[i];
    }
}

Equation& Equation::operator=(const Equation& e) {
    if (this != &e) {
        double* nc = new double[e.deg + 1];
        for (int i = 0; i <= e.deg; ++i) {
            nc[i] = e.coef[i];
        }
        delete[] coef;
        coef = nc;
        deg = e.deg;
    }
    return *this;
}

Equation::~Equation() {
    delete[] coef;
}

int Equation::getDeg() const {
    return deg;
}

double Equation::operator[](int k) const {
    if (k < 0 || k > deg) {
        return 0.0;
    }
    return coef[deg - k];
}

double Equation::operator()(double x) const {
    double r = 0.0;
    for (int i = 0; i <= deg; ++i) {
        r = r * x + coef[i];
    }
    return r;
}

Equation Equation::derivative() const {
    if (deg == 0) {
        return Equation();
    }
    double* tmp = new double[deg];
    for (int i = 0; i < deg; ++i) {
        tmp[i] = (deg - i) * coef[i];
    }
    Equation d(tmp, deg - 1);
    delete[] tmp;
    return d;
}

int Equation::solve(double x0, double& root) const {
    const double eps = 1e-10;    // |x_{k+1} - x_k| 的收敛阈值
    const int    maxIter = 100;  // 迭代上限
    const double dMin = 1e-12;   // 导数"接近 0"的判定阈值

    double x = x0;
    root = x0;
    for (int k = 1; k <= maxIter; ++k) {
        double fx = (*this)(x);
        double dfx = 0.0;
        for (int i = 0; i < deg; ++i) {
            dfx = dfx * x + (deg - i) * coef[i];
        }
        if (fabs(dfx) < dMin) {      // 切线水平，Newton 走不下去
            root = x;
            return -1;
        }
        double xn = x - fx / dfx;
        if (fabs(xn - x) < eps) {
            root = xn;
            return k;
        }
        x = xn;
    }
    root = x;
    return -1;
}

ostream& operator<<(ostream& os, const Equation& e) {
    bool printed = false;
    for (int i = 0; i <= e.deg; ++i) {
        double c = e.coef[i];
        if (c == 0.0) {
            continue;
        }
        int p = e.deg - i;
        if (!printed) {
            if (c < 0) {
                os << "-";
            }
        } else {
            os << (c < 0 ? "-" : "+");
        }
        double a = (c < 0) ? -c : c;
        if (p == 0) {
            os << a;
        } else {
            if (a != 1.0) {
                os << a;
            }
            os << "x";
            if (p > 1) {
                os << "^" << p;
            }
        }
        printed = true;
    }
    if (!printed) {
        os << "0";
    }
    return os;
}

// ---------- 以下 main 由题目给出，不得修改 ----------
int main() {
    int q = 0;
    if (!(cin >> q)) return 0;
    for (int t = 0; t < q; ++t) {
        int n = 0;
        cin >> n;
        double c[8];
        for (int i = 0; i <= n; ++i) {
            cin >> c[i];
        }
        double x0 = 0.0;
        cin >> x0;

        Equation eq(c, n);
        cout << eq << endl;
        cout << eq.derivative() << endl;
        cout << eq.getDeg() << " " << eq[0] << " " << eq[eq.getDeg() + 1] << endl;
        double root = 0.0;
        int it = eq.solve(x0, root);
        if (it < 0) {
            cout << "FAIL" << endl;
        } else {
            cout << root << " " << it << endl;
        }
    }
    return 0;
}

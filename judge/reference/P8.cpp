#include <iostream>
#include <cstddef>
#include <cmath>
using namespace std;

// ==================== 以下 Poly 由题目给出，直接使用 ====================
class Poly {
    int    n;      // 次数（规范化后），0 <= n <= 6
    double c[7];   // c[0] 最高次，c[n] 常数项
public:
    Poly(const double* coef, int deg) : n(0) {
        int first = 0;
        while (coef != 0 && first <= deg && coef[first] == 0.0) {
            ++first;
        }
        if (coef == 0 || first > deg) {
            n = 0;
            c[0] = 0.0;
            return;
        }
        n = deg - first;
        for (int i = 0; i <= n; ++i) {
            c[i] = coef[first + i];
        }
    }
    int getDeg() const { return n; }
    double operator()(double x) const {          // Horner 求值
        double r = 0.0;
        for (int i = 0; i <= n; ++i) {
            r = r * x + c[i];
        }
        return r;
    }
    double derivativeAt(double x) const {        // 导数值
        double r = 0.0;
        for (int i = 0; i < n; ++i) {
            r = r * x + (n - i) * c[i];
        }
        return r;
    }
    friend ostream& operator<<(ostream& os, const Poly& p) {
        bool printed = false;
        for (int i = 0; i <= p.n; ++i) {
            double co = p.c[i];
            if (co == 0.0) {
                continue;
            }
            int power = p.n - i;
            if (!printed) {
                if (co < 0) os << "-";
            } else {
                os << (co < 0 ? "-" : "+");
            }
            double a = (co < 0) ? -co : co;
            if (power == 0) {
                os << a;
            } else {
                if (a != 1.0) os << a;
                os << "x";
                if (power > 1) os << "^" << power;
            }
            printed = true;
        }
        if (!printed) {
            os << "0";
        }
        return os;
    }
};
// ==================== Poly 结束 ====================

template <class T>
class Node {
    T     key;
    Node* next;
public:
    Node(const T& k) : key(k), next(NULL) {}
    template <class TT> friend class List;
};

template <class T>
class List {
    Node<T>* head;

    void clear() {
        Node<T>* p = head;
        while (p != NULL) {
            Node<T>* nx = p->next;
            delete p;
            p = nx;
        }
        head = NULL;
    }
public:
    List() : head(NULL) {}
    List(const List<T>& o) : head(NULL) {
        for (int i = 0; i < o.size(); ++i) {
            push_back(o[i]);
        }
    }
    List<T>& operator=(const List<T>& o) {
        if (this != &o) {
            clear();
            for (int i = 0; i < o.size(); ++i) {
                push_back(o[i]);
            }
        }
        return *this;
    }
    ~List() {
        clear();
    }
    void push_back(const T& k) {
        Node<T>* nd = new Node<T>(k);
        if (head == NULL) {
            head = nd;
            return;
        }
        Node<T>* p = head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = nd;
    }
    bool empty() const { return head == NULL; }
    int size() const {
        int k = 0;
        for (Node<T>* p = head; p != NULL; p = p->next) {
            ++k;
        }
        return k;
    }
    const T& operator[](size_t i) const {
        Node<T>* p = head;
        for (size_t k = 0; k < i && p != NULL; ++k) {
            p = p->next;
        }
        return p->key;
    }
    friend ostream& operator<<(ostream& os, const List<T>& l) {
        os << "[";
        bool first = true;
        for (Node<T>* p = l.head; p != NULL; p = p->next) {
            if (!first) {
                os << " ";
            }
            os << p->key;
            first = false;
        }
        os << "]";
        return os;
    }
};

class RootFinder {
protected:
    double       tol;
    int          maxIter;
    int          iters;
    List<double> trace;

    bool converged(double a, double b) const {   // |a - b| < tol
        return fabs(a - b) < tol;
    }
    void record(double x) {
        trace.push_back(x);
        ++iters;
    }
public:
    RootFinder(double t, int m) : tol(t), maxIter(m), iters(0) {}
    int getIterations() const { return iters; }
    const List<double>& getTrace() const { return trace; }
    const char* name() const { return "root"; }
};

class NewtonFinder : public RootFinder {
public:
    NewtonFinder(double t, int m) : RootFinder(t, m) {}
    const char* name() const { return "newton"; }
    bool solve(const Poly& f, double x0, double, double& root) {
        double x = x0;
        for (int k = 1; k <= maxIter; ++k) {
            double dfx = f.derivativeAt(x);
            if (fabs(dfx) < 1e-12) {
                return false;
            }
            double xn = x - f(x) / dfx;
            record(xn);
            if (converged(x, xn)) {
                root = xn;
                return true;
            }
            x = xn;
        }
        return false;
    }
};

class SecantFinder : public RootFinder {
public:
    SecantFinder(double t, int m) : RootFinder(t, m) {}
    const char* name() const { return "secant"; }
    bool solve(const Poly& f, double x0, double x1, double& root) {
        double a = x0, b = x1;
        for (int k = 1; k <= maxIter; ++k) {
            double fb = f(b);
            double d = fb - f(a);
            if (fabs(d) < 1e-12) {
                return false;
            }
            double x2 = b - fb * (b - a) / d;
            record(x2);
            if (converged(b, x2)) {
                root = x2;
                return true;
            }
            a = b;
            b = x2;
        }
        return false;
    }
};

class BisectFinder : public RootFinder {
public:
    BisectFinder(double t, int m) : RootFinder(t, m) {}
    const char* name() const { return "bisect"; }
    bool solve(const Poly& f, double a0, double b0, double& root) {
        double a = a0, b = b0;
        double fa = f(a), fb = f(b);
        if (fa == 0.0) {
            root = a;
            return true;
        }
        if (fb == 0.0) {
            root = b;
            return true;
        }
        if (fa * fb > 0.0) {
            return false;
        }
        for (int k = 1; k <= maxIter; ++k) {
            double m = (a + b) / 2.0;
            record(m);
            if (converged(a, b)) {
                root = m;
                return true;
            }
            double fm = f(m);
            if (fm == 0.0) {
                root = m;
                return true;
            }
            if (fa * fm < 0.0) {
                b = m;
                fb = fm;
            } else {
                a = m;
                fa = fm;
            }
        }
        return false;
    }
};

// ---------- 模板驱动：三种求根器共用一套输出 ----------
template <typename S>
void runOne(const Poly& f, double a, double b) {
    S s(1e-10, 100);
    double root = 0.0;
    cout << s.name() << endl;
    if (s.solve(f, a, b, root)) {
        cout << root << " " << s.getIterations() << endl;
    } else {
        cout << "FAIL" << endl;
    }
    List<double> tr = s.getTrace();
    cout << tr << endl;
}

// ---------- 以下 main 由题目给出，不得修改 ----------
int main() {
    int q = 0;
    if (!(cin >> q)) return 0;
    for (int t = 0; t < q; ++t) {
        int m = 1;
        int n = 0;
        cin >> m >> n;
        double c[8];
        for (int i = 0; i <= n; ++i) {
            cin >> c[i];
        }
        double a = 0.0, b = 0.0;
        cin >> a >> b;

        Poly f(c, n);
        cout << f << endl;
        if (m == 1) {
            runOne<NewtonFinder>(f, a, b);
        } else if (m == 2) {
            runOne<SecantFinder>(f, a, b);
        } else {
            runOne<BisectFinder>(f, a, b);
        }
    }
    return 0;
}

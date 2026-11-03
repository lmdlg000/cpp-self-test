# P8 · 求根器家族（模板链表 + 继承 + 三种迭代法）

> 难度：★★★★★　·　对应课件：第 2~9 周（循环与收敛判定、引用参数、类与资源管理、`const`、类模板、链表、继承）　·　构建：单文件 OJ（README 第 1.1 节）　·　提交文件：`P8.cpp`
>
> 通用约定（评测命令、输入/输出格式、禁用项、健壮性 R1~R9、评分）见 [README](README.md)；本文只写本题的额外要求。

## 1. 题目背景

上学期的求根作业只有"一个多项式 + 一个牛顿法"。这一版要把它做成**求根器家族**：

- 三种方法各有脾气——牛顿法收敛最快但要导数、割线法不用导数、二分法最稳但最慢；
- 每种方法都要**留下迭代轨迹**（每次迭代算出的新近似根），用自己写的**模板链表**存起来，最后整条打印出来对比收敛速度；
- 三种方法共享"阈值、迭代上限、收敛判定、轨迹记录"这套骨架，所以抽出一个**基类** `RootFinder`，`NewtonFinder` / `SecantFinder` / `BisectFinder` 三个**派生类**只写各自的迭代公式；
- 输出逻辑对三种求根器完全一样，用**函数模板** `runOne<Solver>` 统一驱动。

请实现 `Node<T>` / `List<T>`（模板链表）、`RootFinder` 基类与三个派生类、以及驱动模板 `runOne`。多项式 `Poly` 与 `main` 由题目给定，直接使用。

## 2. 考查内容

| 考点 | 出处 |
| --- | --- |
| **类模板**：`template <class T>` 的 `Node<T>` / `List<T>`，模板里再写模板（`friend class List`）；模板代码必须和使用处同一个编译单元（单文件题天然满足） | 第 9 周 *Class Templates*、*Node.h with template*、*List.h with template*；第 8 周 *Compile Time Instantiation Creates a Restriction* |
| **链表**：`head` 指针、`push_back` 尾插、`operator[]` 遍历、`size` / `empty`、析构里逐个 `delete` | 第 9 周 *数据结构：List(链表)*、*List.C part 1~3* |
| **模板类的资源三件套**：`List<T>` 有 `head` 指针 → 拷贝构造 / `operator=` 必须**深拷贝**（否则两条链表共用节点，析构两次 = double free） | 第 5~6 周 *Deep copy*；第 9 周 List 的析构 |
| **继承**：`public` 继承、构造函数初始化列表把参数转发给基类、`protected` 成员派生类可用、派生类**隐藏**基类的 `name()`、基类指针/引用的常识 | 第 9 周 *C++ inheritance*、*Inheritance error 2*、*Fix: access base private through public functions*、*Inheritance usage* |
| **函数模板**：`template <typename S> void runOne(...)`，用不同类型实例化同一套输出逻辑（静态分派，本讲未涉及虚函数） | 第 9 周 *C++ Function Templates* |
| 三种迭代算法与收敛判定：牛顿、割线、二分；相邻两次差的绝对值小于阈值、迭代上限、导数或分母为 0、区间不变号 | 第 2 周 *while / for*；第 1~2 周浮点运算 |
| `const` 正确性与输出格式：`getIterations` / `getTrace` / `operator<<` 都不改对象；多项式文本、轨迹文本的固定写法 | 第 7 周 *const*；第 2 周 `cout` 格式 |

**与课件示例的区别**：课件用 `Pair<T>` 演示类模板、用 `List` / `List<T>` 演示链表、用 `Person`/`Student`/`Staff` 演示继承——三块内容是分开讲的。本题把它们**拧到一个程序里**：`List<T>` 是求根器的成员（模板 + 链表），`RootFinder` 家族是继承（基类管状态、派生类管算法），`runOne` 是函数模板（一套输出驱动三种类型）。课件里的 `Person` 例子只做到"基类指针调基类版本"（没有虚函数），本题也**不要求虚函数**：main 用 `runOne<NewtonFinder>(...)` 这种模板实例化做静态分派，正好衔接第十周的动态多态。

## 3. 必做接口

只需一个 `.cpp`（建议命名 `P8.cpp`）。**`Poly` 与 `main` 由题目给出**，原样粘贴；你需要在它们之间补上 `Node`/`List`/`RootFinder` 家族与 `runOne`。

### 3.1 给定的 `Poly`（直接粘贴，不要改）

```cpp
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
```

`Poly` 的系数是**整数倍**、次数 ≤ 6，构造时已去掉高次的前导 0（全 0 得到 `0`）。

### 3.2 你要实现的模板链表（签名不得改动）

```cpp
template <class T>
class Node {
    T     key;
    Node* next;
public:
    Node(const T& k);
    template <class TT> friend class List;
};

template <class T>
class List {
    Node<T>* head;
public:
    List();
    List(const List<T>& o);                 // 深拷贝
    List<T>& operator=(const List<T>& o);   // 深拷贝、自赋值安全
    ~List();
    void push_back(const T& k);             // 尾插
    bool empty() const;
    int  size() const;                      // 元素个数
    const T& operator[](size_t i) const;    // 第 i 个元素
    friend ostream& operator<<(ostream& os, const List<T>& l);   // 见第 5 节格式
};
```

- `Node<T>` 的 `key` / `next` 与 `List<T>` 的 `head` **名字和类型不得改**；`friend` 关系要写对，否则 `List` 访问不到 `Node` 的私有成员；
- `operator<<` 建议**在类里直接定义**（inline friend），这样每个实例化都自带一份，不会踩"模板声明与定义分家"的坑；
- `List` 只存 `head` 一个指针（和课件一致，`push_back` 从头走到尾即可，本题链表最长 100 个节点）。

### 3.3 你要实现的继承家族（签名不得改动）

```cpp
class RootFinder {
protected:
    double       tol;       // 收敛阈值
    int          maxIter;   // 迭代上限
    int          iters;     // 已迭代次数
    List<double> trace;     // 迭代轨迹（模板链表）
    bool converged(double a, double b) const;   // 公共收敛判定（见第 6 节）
    void record(double x);                      // 记录一个点：push_back + iters 自增
public:
    RootFinder(double t, int m);
    int getIterations() const;
    const List<double>& getTrace() const;
    const char* name() const;                   // 基类返回 "root"
};

class NewtonFinder : public RootFinder {
public:
    NewtonFinder(double t, int m);
    const char* name() const;                   // "newton"
    bool solve(const Poly& f, double x0, double x1, double& root);
};

class SecantFinder : public RootFinder {
public:
    SecantFinder(double t, int m);              // 同上
    const char* name() const;                   // "secant"
    bool solve(const Poly& f, double x0, double x1, double& root);
};

class BisectFinder : public RootFinder {
public:
    BisectFinder(double t, int m);              // 同上
    const char* name() const;                   // "bisect"
    bool solve(const Poly& f, double x0, double x1, double& root);
};
```

- 三个类必须**公有继承** `RootFinder`（`class NewtonFinder : public RootFinder`），构造函数用**初始化列表**把 `t, m` 转给基类；
- 三个 `solve` 的签名**完全一样**（都收两个 `double`）：牛顿法只用 `x0`，割线法用 `x0` 和 `x1`，二分法把两者当区间 `[x0, x1]` 的两端。这样 `runOne` 才能用同一个模板驱动三种求根器；
- `name()` 在基类返回 `"root"`，三个派生类**隐藏**它并返回自己的名字（课件里 `Student::dailyActivity` 隐藏 `Person::dailyActivity` 的同款写法）。**不要求 `virtual`**——`runOne<S>` 拿到的是具体类型 `S`，调到的自然是派生版本；
- `solve` 成功返回 `true` 并把近似根写进 `root`（引用参数，R6 之外的"返回结果"全靠它）；失败返回 `false`。

### 3.4 你要实现的驱动模板（签名不得改动）

```cpp
template <typename S>
void runOne(const Poly& f, double a, double b);
```

它按第 5 节的格式打印**三行**（方法名 / 结果 / 轨迹），内部要：构造 `S s(1e-10, 100)` → 打印 `s.name()` → 调 `s.solve(f, a, b, root)` → 打印结果 → 把 `s.getTrace()` 拷进一个 `List<double>` 再打印（**这一步会走 `List` 的拷贝构造**，浅拷贝在这里就是 double free）。

### 3.5 给定的 `main`（原样粘贴，不得修改）

```cpp
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
```

`main` 每组固定做两件事：打印多项式，然后按 `m` 实例化对应的求根器。**任何对 `main` 的改动（包括多打一个空格）都按 R8 处理。**

## 4. 输入格式

第一行 `q`（`1 ≤ q ≤ 200`，数据组数）。随后每组一行（token 用任意空白分隔）：

```text
m  n  c0  c1  ...  cn  a  b
```

- `m`：方法编号，`1` = 牛顿法、`2` = 割线法、`3` = 二分法；
- `n`：多项式次数，`0 ≤ n ≤ 6`；
- `c0 … cn`：从**最高次到常数项**的系数，整数，可能有前导 `0`；
- `a b`：实数。`m = 1` 时 `a` 是初值 `x0`（`b` 忽略）；`m = 2` 时是割线的两个起点 `x0`、`x1`；`m = 3` 时是区间两端。

## 5. 输出格式

每组数据输出 **4 行**：

1. 多项式文本（由给定的 `Poly::operator<<` 产生，你不用管）；
2. 方法名：`newton` / `secant` / `bisect`；
3. 结果：成功 `root iters`（近似根 + 迭代次数，空格分隔）；失败单独一行 `FAIL`；
4. 迭代轨迹：`[` + 各点用**一个空格**分隔 + `]`；空轨迹打印 `[]`。

轨迹里放的是**每次迭代算出的新近似根**（不含初始值）：
- 牛顿法：每次的 `x(k+1)`；
- 割线法：每次的 `x(k+2)`；
- 二分法：每次的中点 `m = (a + b) / 2`。

数值一律用 `cout` 默认格式（6 位有效数字），**不要** `fixed` / `setprecision`；行末不要多余空格，末尾只有一个换行。

示例：`[2.1 2.09457 2.09455 2.09455 2.09455]`、`[]`。

## 6. 计算规则与约束

1. **`RootFinder` 的公共部分**：
   - 构造：`tol = t`、`maxIter = m`、`iters = 0`、`trace` 为空；
   - `converged(a, b)`：`fabs(a - b) < tol` —— 三种方法共用同一个判据（牛顿/割线比相邻两点，二分法比区间宽度）；
   - `record(x)`：`trace.push_back(x)` 且 `++iters`；
   - `getIterations()` 返回 `iters`，`getTrace()` 返回 `trace` 的 `const` 引用。
2. **牛顿法**（`NewtonFinder::solve`）：`x` 从 `x0` 出发，每轮
   - 算 `dfx = f.derivativeAt(x)`；**若 `fabs(dfx) < 1e-12`** → 切线水平，直接返回 `false`（这一轮**不记录**）；
   - `xn = x - f(x) / dfx` → `record(xn)` → 若 `converged(x, xn)` 则 `root = xn`、返回 `true`；否则 `x = xn` 继续；`x1` 参数忽略。
3. **割线法**（`SecantFinder::solve`）：`a = x0`、`b = x1`，每轮
   - 算 `d = f(b) - f(a)`；**若 `fabs(d) < 1e-12`** → 返回 `false`（不记录）；
   - `x2 = b - f(b) * (b - a) / d` → `record(x2)` → 若 `converged(b, x2)` 则 `root = x2`、返回 `true`；否则 `a = b; b = x2` 继续。
4. **二分法**（`BisectFinder::solve`）：`a = x0`、`b = x1`，先算 `fa = f(a)`、`fb = f(b)`：
   - 若 `fa == 0.0` → `root = a`、返回 `true`（**0 次迭代**，轨迹为空）；
   - 若 `fb == 0.0` → `root = b`、返回 `true`（同上）；
   - 若 `fa * fb > 0.0` → 区间内没有变号，返回 `false`（不记录）；
   - 否则每轮：`m = (a + b) / 2` → `record(m)` → **若 `converged(a, b)`** 则 `root = m`、返回 `true` → 否则算 `fm = f(m)`：`fm == 0` 时 `root = m`、返回 `true`；`fa * fm < 0` 时 `b = m, fb = fm`；否则 `a = m, fa = fm`。
   - 注意"先记录、再判宽度"：区间宽度第一次小于 `tol` 时，那个中点也要进轨迹。
5. **迭代上限**：三种方法都在 `iters` 达到 `maxIter`（= 100）仍未收敛时返回 `false`，轨迹里最多留 100 个点。
6. **输出配合**：`runOne` 里 `List<double> tr = s.getTrace();` 走的是**拷贝构造**——`List` 的拷贝必须是深拷贝（R5）。
7. **数值**：一律 `double`；允许 `<cmath>` 的 `fabs`。本题只用 `+ - * /` 与比较，结果确定性可复现。
8. **允许使用**（第 8~9 周内容）：`template`（类模板、函数模板、`template <typename T, int N>`）、`std::string` / `stringstream`（拼输出用得上）、`<fstream>`（本地调试）。**仍然禁止**：`vector` / `map` / `array` 等 STL 容器、`auto`、范围 `for`、`lambda`、智能指针、`<bits/stdc++.h>`、`rand` / `srand` / `time`。阈值请用 `const` 而不是 `#define`。
9. **文件划分**：单文件题，所有模板与类都写在同一个 `.cpp` 里（模板的定义必须对被使用处可见，这正好是第 8 周 *Compile Time Instantiation Creates a Restriction* 讲的限制）。

## 7. 样例

**【样例 1 输入】**

```text
6
1 3 1 0 -2 -5 2 3
2 3 1 0 -2 -5 1 2
3 3 1 0 -2 -5 2 3
1 2 1 -2 1 0 1
2 2 1 0 -2 1 2
3 2 1 0 -2 1 2
```

**【样例 1 输出】**（stdout）

```text
x^3-2x-5
newton
2.09455 5
[2.1 2.09457 2.09455 2.09455 2.09455]
x^3-2x-5
secant
2.09455 7
[2.2 2.08897 2.09423 2.09455 2.09455 2.09455 2.09455]
x^3-2x-5
bisect
2.09455 35
[2.5 2.25 2.125 2.0625 2.09375 2.10938 2.10156 2.09766 2.0957 2.09473 2.09424 2.09448 2.0946 2.09454 2.09457 2.09456 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455 2.09455]
x^2-2x+1
newton
1 28
[0.5 0.75 0.875 0.9375 0.96875 0.984375 0.992188 0.996094 0.998047 0.999023 0.999512 0.999756 0.999878 0.999939 0.999969 0.999985 0.999992 0.999996 0.999998 0.999999 1 1 1 1 1 1 1 1]
x^2-2
secant
1.41421 7
[1.33333 1.4 1.41463 1.41421 1.41421 1.41421 1.41421]
x^2-2
bisect
1.41421 35
[1.5 1.25 1.375 1.4375 1.40625 1.42188 1.41406 1.41797 1.41602 1.41504 1.41455 1.41431 1.41418 1.41425 1.41422 1.4142 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421 1.41421]
```

**【样例 1 诊断】**（stderr，评测忽略）：无输出

**【说明】**

6 组数据是"同一个方程、三种方法"与"同一个方程、重根"的对照：

- **第 1~3 组**：`x^3-2x-5` 用牛顿、割线、二分，根都是 `2.09455`，但迭代次数是 **5 / 7 / 35**——这就是轨迹要留下的东西：牛顿二次收敛、割线超线性、二分每次只砍一半；
- **第 3 组**二分法从 `[2, 3]` 开始，第一个中点 `2.5`，之后 `2.25`、`2.125`…… 35 次后区间宽度才小于 `1e-10`；轨迹后半段全是 `2.09455`，因为 6 位有效数字已经看不出差别；
- **第 4 组**：`(x-1)^2` 是**二重根**，牛顿法退化成线性收敛，要 28 次；轨迹前半段 `0.5 → 0.75 → 0.875`，每次误差减半；
- **第 5、6 组**：`x^2-2` 的割线与二分，割线 7 次就到 `1.41421`。

**【样例 2 输入】**

```text
6
1 3 1 0 -3 1 1 2
3 3 1 0 -2 -5 3 4
2 2 1 0 -1 -1 1
3 2 1 0 -4 2 3
1 3 1 0 0 -8 2 5
3 1 2 -4 0 5
```

**【样例 2 输出】**（stdout）

```text
x^3-3x+1
newton
FAIL
[]
x^3-2x-5
bisect
FAIL
[]
x^2-1
secant
FAIL
[]
x^2-4
bisect
2 0
[]
x^3-8
newton
2 1
[2]
2x-4
bisect
2 37
[2.5 1.25 1.875 2.1875 2.03125 1.95312 1.99219 2.01172 2.00195 1.99707 1.99951 2.00073 2.00012 1.99982 1.99997 2.00005 2.00001 1.99999 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2]
```

**【样例 2 诊断】**（stderr，评测忽略）：无输出

**【说明】**

- **第 1 组**：`x^3-3x+1` 从 `x0 = 1` 出发，`f'(1) = 0`（初值恰在驻点）→ 牛顿法第一轮就返回 `false`，`FAIL` + 空轨迹；
- **第 2 组**：`x^3-2x-5` 在 `[3, 4]` 上两端同号（`f(3) = 16`、`f(4) = 51`），二分法无从下手 → `FAIL`；
- **第 3 组**：`x^2-1` 取 `x0 = -1`、`x1 = 1`，`f(-1) = f(1) = 0`，割线的分母 `d = 0` → `FAIL`；
- **第 4 组**：`x^2-4` 在 `[2, 3]` 上，左端点就是根 → **0 次迭代**、`root = 2`、轨迹 `[]`；
- **第 5 组**：`x^3-8` 从 `x0 = 2` 出发，第一步就落在根上 → 1 次迭代、轨迹 `[2]`；
- **第 6 组**：`2x-4` 在 `[0, 5]` 上用二分，区间从 5 砍到 `1e-10` 以下要 37 次——**二分法与方程"长得简单"无关，只看区间宽度**。

## 8. 健壮性要求

通用要求见 README 第 4 节，本题额外/重点要求：

- **R1（多组数据）**：一个进程内处理 `q` 组，每组新建一个求根器；不许用全局/`static` 变量累计轨迹或迭代次数（`trace` 是成员，不是全局）。
- **R2（非法数据）**：本题输入保证合法，不需要读入诊断；"算不下去"（驻点、分母为 0、区间不变号、超上限）统一用 `FAIL` 表示。
- **R3（边界）**：0 次迭代（端点即根）、1 次迭代、重根、驻点初值、区间同号、`q = 1`、前导零系数、负初值、100 次上限。
- **R4（动态内存）**：`new Node<T>` ↔ `delete` 严格配对；`List` 析构要把整条链走完；`List<double> tr = s.getTrace();` 产生的拷贝必须独立。用 `valgrind --leak-check=full --error-exitcode=1 ./a.out < in.txt` 或 `g++ -g -O1 -fsanitize=address,undefined` 自测（R7），要求 **0 errors、0 leaks**。
- **R5（资源三件套 + 继承）**：`List<T>` 的拷贝构造 / `operator=` 是深拷贝且自赋值安全（模板类的浅拷贝一样会 double free）；派生类的构造必须转发给基类，否则 `tol`/`maxIter` 是未初始化的垃圾。
- **R6（引用参数）**：`solve` 的根通过 `double& root` 带回；失败时不要往 `root` 里写垃圾再让 `runOne` 打印。
- **R8（不改 `main`）**：原样粘贴，一个字符都不改；调试输出只能进 `cerr` 并记得删。
- **本题额外要求（模板的编译期约束）**：`runOne<NewtonFinder>` 要求 `NewtonFinder` 有 `name()`、`solve(const Poly&, double, double, double&)`、`getIterations()`、`getTrace()` 四件套且**签名一致**——少一个就是编译错误（CE），改签名同理（第 8 周 *Compile Time Instantiation Creates a Restriction*）。

**甲方保证**：`1 ≤ q ≤ 200`；`0 ≤ n ≤ 6`；系数为整数且 `|c| ≤ 100`；`|a|, |b| ≤ 1000`；同一输入两次运行输出完全一致。**甲方不保证**：三种方法都收敛；区间一定变号；初值不在驻点上；根的位数好看；`a < b`（二分法自己按给定顺序处理）。

## 9. 自测清单

- [ ] 同一个 `x^3-2x-5` 跑三种方法：迭代次数应为 `5 / 7 / 35`，根都是 `2.09455`；
- [ ] 牛顿法轨迹长度 == 迭代次数；割线法、二分法同理（`trace.size() == iters`）；
- [ ] 二分法第一个中点是 `(a + b) / 2`，之后每次砍一半，轨迹单调逼近根；
- [ ] 二重根 `(x-1)^2` 用牛顿法 → 28 次（说明是线性收敛）；
- [ ] `x^3-3x+1` 从 1 出发（驻点）→ `FAIL` + `[]`；
- [ ] `x^2-1` 割线取 `-1` 与 `1` → 分母 0 → `FAIL` + `[]`；
- [ ] `x^2-4` 在 `[2, 3]` 上二分开局 → `2 0` + `[]`（0 次迭代）；
- [ ] 前导零：`n = 4` 且 `0 0 1 -2 -5` 仍按 `x^2-2x-5` 处理；
- [ ] 三种方法都能被 `runOne<...>` 编译通过（四件套签名齐全）；
- [ ] 把 `List<double>` 的拷贝构造临时删掉（用默认浅拷贝）→ 程序应当在 valgrind/ASan 下报 double free，证明深拷贝真的被用到；
- [ ] `valgrind --leak-check=full --error-exitcode=1 ./a.out < sample1.in`：`0 errors`、`definitely lost: 0 bytes`；或 ASan 版本跑通无报告；
- [ ] 输入的换行随便打乱（甚至全写一行）输出不变。

## 10. 常见错误

1. **模板声明与定义分家**：把 `List<T>` 的成员实现写进另一个文件（或写在类外却忘了 `template <class T>` 前缀）→ 链接期未定义符号。单文件题里最省事的做法是**全部写在类内**。
2. **`Node` 的 `friend` 写错**：`friend class List;` 在模板里要写成 `template <class TT> friend class List;`，否则 `List<T>` 访问不到 `key`/`next`。
3. **`List<T>` 用默认拷贝**：`List<double> tr = s.getTrace();` 之后两条链表指向同一串节点，析构两次 → double free（R5）。拷贝构造要逐节点 `new`。
4. **`operator=` 先 `clear()` 再从 `o` 拷**：`a = a` 自赋值时把源节点删光了。要先判 `this != &o`。
5. **`push_back` 忘了接尾**：只 `new` 了节点却没把上一个节点的 `next` 指过去，链表永远只有第一个元素。
6. **析构只 `delete head`**：后面整条链泄漏；要 `while (head)` 逐个删（先存 `next` 再删）。
7. **模板类写成 `List` 却忘了 `<T>`**：`List::push_back`、`Node* head`（漏 `<T>`）会编译不过或变成别的类型。
8. **派生类构造函数没转发参数**：`NewtonFinder(double t, int m) : RootFinder(t, m) {}` 少了初始化列表 → 基类用了默认构造（本题基类没有默认构造，直接 CE）或 `tol` 是垃圾值。
9. **把 `tol`/`maxIter` 设成 `private`**：派生类的 `solve` 用不了；基类里应是 `protected`（课件 *Inheritance error 2* 讲的就是这个）。
10. **`name()` 忘了在派生类里定义**：`runOne<S>` 调到基类的 `"root"`，样例第 2 行立刻不对。
11. **迭代次数与轨迹对不上**：`record()` 里加了 `push_back` 却没 `++iters`（或反过来）；比较 `getIterations()` 与轨迹长度即可自检。
12. **二分法顺序写反**：先判宽度再记录中点 → 轨迹少一个点、迭代次数少 1；规格是"先 `record(m)` 再判 `converged(a, b)`"。
13. **二分法忘了同号检查**：`fa * fb > 0` 时还硬分，最后收敛到一个"假根"或死循环到 100 次。
14. **牛顿法导数判据漏了**：驻点处 `f/0` 得到 `inf`/`nan`，再往后全是 `nan`，输出里冒出 `nan` → WA。要先判 `fabs(dfx) < 1e-12`。
15. **割线法用 `f(b)` 而不是 `f(b) - f(a)` 当分母**：公式错，收敛到别的地方。
16. **`solve` 签名不一致**：`runOne<S>` 只认 `bool solve(const Poly&, double, double, double&)`；写成 `float`、少一个参数、`double& root` 放前面都会 CE。
17. **`getTrace()` 返回值而不是 `const` 引用**：`List<double> tr = s.getTrace();` 会多一次拷贝（结果还对），但签名与题面不符；写成 `List<double>` 返回则 §3.3 的签名不满足。
18. **输出格式漂移**：轨迹逗号/空格不对、`FAIL` 写成 `Fail`/`fail`、根用 `fixed` 打成 `2.094551`——逐字符比对，一个字符就 WA。
19. **用 `vector` 存轨迹**：本题要的就是手写链表，禁用 STL 容器。
20. **改动 `main`**：哪怕"只是加个空格"也按 R8 算。

## 11. 提交与评测

- 提交**一个** `.cpp`（建议 `P8.cpp`），内容 = 头文件 + 给定的 `Poly` + 你写的 `Node`/`List`/`RootFinder` 家族 + `runOne` + 给定的 `main`。
- 评测命令（README 第 1.1 节）：

  ```bash
  g++ -DONLINE_JUDGE -O2 -w -fmax-errors=3 -std=c++20 P8.cpp -lm -o P8
  ```

- 注意 `-w` 关掉了全部警告：编译通过 ≠ 正确；浅拷贝、漏 `delete`、`nan` 输出都不会被编译器拦下。
- 限制：每组数据 1 s / 256 MB。
- 评分：功能 50（两组样例 + 隐藏用例）、健壮性 30（R1~R8 中适用条目，重点 R4/R5/R6）、规范 20（接口签名、继承关系、模板写法、输出格式、`return 0`）。
- 自测时先跑样例逐字符比对（含 `[]` 与行末换行），再跑第 9 节清单；迭代次数不对时，先确认 `record()` 的调用位置与三个阈值（`1e-10` / `1e-12` / 100）。

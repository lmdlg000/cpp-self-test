# P4 · 图书架类：固定容量与拷贝语义

> 难度：★★★　·　对应课件：第 5~6 周　·　构建：单文件 OJ（README 第 1.1 节）　·　提交文件：`P4.cpp`
>
> 通用约定（评测命令、输入/输出格式、禁用项、健壮性 R1~R9、评分）见 [README](README.md)；本文只写本题的额外要求。

## 1. 题目背景

宿舍书架的一层**只能放固定数量的书**：放满了就真的放不下，不能凭空变长；从中间抽走一本，后面的书要**依次前移补位**，书架的排列顺序保持不变。请用两个类实现这样一层书架：`Book` 表示一本书，`Shelf` 管理这层书架上的书（含内存）。

## 2. 考查内容

| 考点 | 课件依据 |
| --- | --- |
| 类与对象、构造函数默认参数、`public`/`private` | 第 5 周 *Class and Object*、*Constructor to initialize Member data*、*Public, private, protected* |
| 对象数组作为成员（`Book* books`），析构函数里 `delete []` | 第 5 周 *Object\* in a class*；第 6 周 *Forgot to delete Object\* in a class* |
| 浅拷贝 → double free；**深拷贝**才能让副本独立 | 第 5/6 周 *Why Copy Constructor*、*Deep copy* |
| `operator=`：深拷贝 + `if (this != &other)` 自赋值安全 | 第 6 周 `Complex::operator=` 的写法 |
| 固定容量：满则拒绝，**刻意不扩容** | 第 6 周 `Vector` 的容量管理（本题取其"有容量概念"的一面，反其"自动翻倍"的一面） |
| 越界安全：`remove` 越界不操作、`at` 越界返回默认对象 | 第 5 周 Project 5 `Team::release` / `Team::get` 的语义（R6） |
| 内存无错误、无泄漏 | 第 6 周 *Valgrind reports*（R7） |

**与课件示例的区别**：它不是第 6 周 `Vector::Push` 的翻倍扩容（本题容量固定、满了就 `false`），也不是第 5 周 Project 5 里 `Team::release` 的"用末位覆盖被删位置"（本题删除后**整体前移**、保持顺序）；`Book` 也不是 `BankAccount`/`Person` 那种示例类，`Shelf` 更不是 `Monster`/`Team`/`Trainer`、`Game`/`Round`。本题只要求题面给定的 `operator=`，**不考其它运算符重载**，也不涉及字符串类。与课件示例、Project 1~6 的题目均不重复。

## 3. 必做接口

**类声明固定**（可以直接粘贴；成员函数体由你补全，写在类内或像课件那样用 `Book::`、`Shelf::` 在类外定义都可以）。本题只交一个 `.cpp`，`main` 已给出。

```cpp
#include <iostream>
#include <cstddef>
using namespace std;

class Book {
    int id; int pages;
public:
    Book(int id = 0, int pages = 0);
    int getId() const;
    int getPages() const;
};

class Shelf {
    Book* books;   // 动态数组
    int cap;       // 容量（构造时给定，之后不变）
    int len;       // 当前图书数
public:
    Shelf(int capacity = 5);
    Shelf(const Shelf& other);              // 深拷贝
    ~Shelf();
    Shelf& operator=(const Shelf& other);   // 深拷贝、自赋值安全
    bool add(const Book& b);                // 满 → 返回 false 且不修改（不扩容）
    bool remove(int i);                     // 越界(0<=i<len 之外) → false 不修改；否则元素整体前移补位、len--
    Book at(int i) const;                   // 越界返回 Book()
    int totalPages() const;                 // const
    int count() const;                      // const
};
```

### 3.1 每个成员函数的要求

| 成员 | 要求 |
| --- | --- |
| `Book(id, pages)` | 两个成员都赋初值；**必须有默认参数**（`add` 会构造临时对象，`at` 越界要返回 `Book()`，`new Book[cap]` 要对每本书调用默认构造函数）。 |
| `Shelf(capacity = 5)` | `cap = capacity`、`len = 0`、`books = new Book[cap]`。**`cap` 之后就再也不变**（不扩容）。默认参数 `5` 是给 `main` 里 `Shelf M;` 用的，不能删。 |
| `Shelf(const Shelf& other)` | **深拷贝**：新申请 `other.cap` 大小的数组，把 `other` 的 `len` 本书逐个拷过来；`this->books` 与 `other.books` **不能是同一块内存**。注意**容量要跟着一起拷**（副本的 `cap` 等于 `other.cap`）：否则副本在原本未满的书架上会被误判成"满"，`C` 操作就会输出错误结果。 |
| `~Shelf()` | `delete [] books;`，然后把 `books` 置 `NULL`。`Book` 是对象数组，`delete []` 会对每本书调用析构函数（第 6 周构造/析构顺序）。 |
| `operator=` | 先判 `this == &other`（自赋值直接 `return *this`）；否则按 **`other.cap`** 申请新数组 → 拷 `other` 的 `len` 本书 → `delete [] books` → 接管新数组并更新 `cap`、`len` → `return *this`。**注意：新数组必须按 `other.cap` 分配，不能按自己原来的 `cap`！** `main` 的 `E` 里 `Shelf M;` 只有默认容量 5，而 `L` 的容量可能是任意值、书也可能很多；若沿用 `M` 自己的 5 格去装 `L` 的书，就是缓冲区溢出。 |
| `add(b)` | `len == cap` 时**返回 `false` 且不修改对象**（不扩容、不覆盖、不输出）；否则 `books[len] = b; len++;` 返回 `true`。 |
| `remove(i)` | `i < 0` 或 `i >= len` 时返回 `false` 且**完全不修改对象**；否则把 `i+1 … len-1` 的书**依次左移一位**补上空位，`len--`，返回 `true`（保持其余图书的相对顺序）。 |
| `at(i)` | `i < 0` 或 `i >= len` 时返回 `Book()`（`0 0`）；否则返回 `books[i]` 的**副本**（按值返回，不得返回引用或指针）。 |
| `totalPages()` | 返回所有书页数之和；是 `const` 成员函数，不得修改对象。 |
| `count()` | 返回 `len`；`const` 成员函数。 |

`Book` 没有资源（只有两个 `int`），因此不需要三件套；`Shelf` 持有堆内存，**必须**有析构、拷贝构造、`operator=` 三件套。

### 3.2 给定的 `main`（原样贴在你的 `.cpp` 里，不得修改）

```cpp
int main() {
    int cap, q;
    cin >> cap >> q;
    Shelf L(cap);
    for (int i = 0; i < q; i++) {
        char op; cin >> op;
        if (op == 'A') { int id, pages; cin >> id >> pages; cout << (L.add(Book(id, pages)) ? "ok" : "full") << endl; }
        else if (op == 'R') { int idx; cin >> idx; cout << (L.remove(idx) ? "ok" : "no") << endl; }
        else if (op == 'S') { cout << L.totalPages() << " " << L.count() << endl; }
        else if (op == 'C') { Shelf M = L; M.add(Book(999, 1)); cout << L.count() << " " << L.totalPages() << " " << M.count() << " " << M.totalPages() << endl; }
        else if (op == 'E') { Shelf M; M = L; M = M; cout << M.count() << " " << M.totalPages() << endl; }
        else if (op == 'P') { int idx; cin >> idx; Book b = L.at(idx); cout << b.getId() << " " << b.getPages() << endl; }
    }
    return 0;
}
```

`main` 里 `C`、`E` 两个操作是本题的"照妖镜"：

- `C`：`Shelf M = L;` 走**拷贝构造函数**，然后 `M.add(Book(999,1))`。若书架没满，`M` 应该多一本书而 **`L` 不变**；若书架已经满了，`M.add` 返回 `false`（`main` 不打印返回值），两张表数字相同。**只有深拷贝 + 连容量一起拷，两种情况才对。**
- `E`：`Shelf M;` 先按**默认参数 `capacity = 5`** 构造，然后 `M = L;` 走 `operator=`，紧接着 `M = M;` 是**自赋值**。`operator=` 里若没有判 `this == &other`，或先 `delete [] books` 再读 `other.books`，会 use-after-free / double free；若新数组按 `M` 自己的 `cap`（5）分配，`L` 的书一多就越界写。

## 4. 输入格式

1. 第 1 行：`cap q` —— 书架容量、操作条数；
2. 随后 `q` 行操作，每行首字符 `op` 取 `A`/`R`/`S`/`C`/`E`/`P`：

| 操作 | 参数 | 含义 |
| --- | --- | --- |
| `A id pages` | 两个整数 | 尝试把一本书放到书架末尾 |
| `R idx` | 一个整数 | 抽走下标为 `idx` 的书 |
| `S` | 无 | 查询总页数与书的数量 |
| `C` | 无 | 拷贝一份 → 副本再放一本书 → 打印两张表 |
| `E` | 无 | 默认构造 + 赋值 + 自赋值 → 打印结果 |
| `P idx` | 一个整数 | 打印下标为 `idx` 的书 |

输入是空白分隔的 token，用 `cin >> op >> …` 读，不要依赖行结构。

## 5. 输出格式

输出**完全由给定的 `main` 决定**，你不需要写任何 `cout`：

| 操作 | 输出 |
| --- | --- |
| `A` | `ok`（放得下）或 `full`（已满，未放入） |
| `R idx` | `ok`（抽走成功）或 `no`（越界，什么都没改） |
| `S` | `总页数 书数`（**页数在前**） |
| `C` | `L书数 L总页数 M书数 M总页数`（**书数在前**） |
| `E` | `M书数 M总页数` |
| `P idx` | `id pages`；越界时 `at` 返回 `Book()`，打印 `0 0` |

行末无多余空格，末尾无多余空行。

## 6. 计算规则与约束

- 容量 `cap` 在 `Shelf(cap)` 时确定，**之后不变**；`add` 在 `len == cap` 时只返回 `false`，这本书**不进入书架**。
- `remove(i)` 删除后**保持剩余图书的相对顺序**（`i+1` 起的书整体左移一位），**不是**把最后一本搬到 `i` 位置。
- 越界判断：`remove` 与 `at` 都用 `i < 0 || i >= len`（`i == len` 也是越界）。
- 数据范围（甲方保证）：`1 <= cap <= 500`；`1 <= q <= 1000`；`0 <= id <= 9999`；`0 <= pages <= 2000`；`R`/`P` 的 `idx` 非负。总页数最多 `500 × 2000 = 1000000`，`int` 不会溢出。
- **禁止**：STL 容器（`vector`/`array`…）、`std::string`、`auto`、范围 `for`、`lambda`、`template`、`<bits/stdc++.h>`、`rand()`/`srand()`/`time()`；除题面要求的 `operator=` 外**不要写任何运算符重载**（不要 `operator[]`、`operator<<`、`operator+`），不要用字符串类。见 README 第 3 节。
- 允许包含：`<iostream>`、`<cstddef>`、`<cstdlib>`、`<cmath>`、`<cstring>`、`<cctype>`。
- 构造/析构顺序（第 6 周）：`books` 是**对象数组**指针（`new Book[cap]`），一次 `new` 会调用 `cap` 次 `Book` 默认构造；一次 `delete [] books` 会调用 `cap` 次 `Book` 析构，之后再释放内存。

## 7. 样例

全部样例的 stderr 均为空。

**【样例 1 输入】**

```text
3 9
A 1 100
A 2 200
A 3 300
A 4 400
S
R 1
P 0
P 1
S
```

**【样例 1 输出】**（stdout）

```text
ok
ok
ok
full
600 3
ok
1 100
3 300
400 2
```

**【样例 1 诊断】**（stderr，评测忽略）：无输出

**【说明】** `cap = 3`：放满 3 本后第 4 本返回 `full`（不扩容、不覆盖，书架仍是 3 本 600 页）。`R 1` 抽走中间的 `(2,200)`，后面的 `(3,300)` **前移**到下标 1，所以 `P 0` 仍是 `1 100`、`P 1` 变成 `3 300`，`S` 为 `400 2`。若用"末位覆盖"的写法（把 `(3,300)` 留在下标 2 而把某本搬到下标 1），`P 1` 与后面的 `P` 检查就会露馅。

**【样例 2 输入】**

```text
1 7
A 10 50
A 11 60
S
R 0
S
P 0
R 0
```

**【样例 2 输出】**（stdout）

```text
ok
full
50 1
ok
0 0
0 0
no
```

**【样例 2 诊断】**（stderr，评测忽略）：无输出

**【说明】** `cap = 1` 的极小书架：只能放 1 本，第 2 本 `full`；抽走唯一一本后书架为空，`S` 为 `0 0`、`P 0` 返回默认对象 `0 0`、`R 0` 返回 `no`。空架子上任何下标都越界，`remove`/`at` 都不能访问 `books[0]`。

**【样例 3 输入】**

```text
5 12
S
R 0
P 0
A 1 10
A 2 20
A 3 30
R 0
S
P 0
R 1
S
P 1
```

**【样例 3 输出】**（stdout）

```text
0 0
no
0 0
ok
ok
ok
ok
50 2
2 20
ok
20 1
0 0
```

**【样例 3 诊断】**（stderr，评测忽略）：无输出

**【说明】** 空书架上的 `S`/`R`/`P` 全部安全。放 3 本后 `R 0` 抽走**首本** `(1,10)`，`(2,20)`、`(3,30)` 前移，`S` 为 `50 2`、`P 0` 为 `2 20`。再 `R 1` 抽走**末本**（此时的下标 1 是 `(3,30)`），只剩 `(2,20)`：`S` 为 `20 1`，`P 1` 越界输出 `0 0`。

**【样例 4 输入】**

```text
4 6
A 1 100
A 2 200
C
E
S
C
```

**【样例 4 输出】**（stdout）

```text
ok
ok
2 300 3 301
2 300
300 2
2 300 3 301
```

**【样例 4 诊断】**（stderr，评测忽略）：无输出

**【说明】** 深拷贝的核心用例：`L` 有 2 本（300 页，容量 4 未满）。`C`：副本 `M` 拷到 2 本、容量仍为 4，于是 `M.add(Book(999,1))` 成功，打印 `2 300 3 301`——若 `M` 与 `L` 共用内存，`L` 会跟着变成 3 本；若拷贝构造只分配 `len` 个而把容量也设成 2，`M` 会被误判为满，输出会变成 `2 300 2 300`。`E`：`M` 默认容量 5，`M = L` 后为 `2 300`，`M = M` 自赋值后不变，打印 `2 300`（对应 `S` 的 `300 2` 是反过来的顺序）。最后一次 `C` 证明 `L` 从头到尾没变。

**【样例 5 输入】**

```text
4 11
A 1 10
A 2 20
A 3 30
A 4 40
C
R 1
P 0
P 1
P 2
S
E
```

**【样例 5 输出】**（stdout）

```text
ok
ok
ok
ok
4 100 4 100
ok
1 10
3 30
4 40
80 3
3 80
```

**【样例 5 诊断】**（stderr，评测忽略）：无输出

**【说明】** 满架上的 `C`：`L` 已有 4 本、容量 4，副本 `M` 拷过去也是满的，`M.add(Book(999,1))` 返回 `false`，所以两边都是 `4 100`（注意这里**不能**因为"加书失败"就崩溃或改动 `L`）。随后 `R 1` 抽走 `(2,20)`，`(3,30)`、`(4,40)` 依次前移一位：`P 0/1/2` 分别为 `1 10`、`3 30`、`4 40`，多元素前移一次验证到位；`S` 为 `80 3`，`E` 打印 `3 80`。

## 8. 健壮性要求

通用 R 编号含义见 README 第 4 节，本题评分特别看：

- **R5（类有资源三件套）**：`Shelf` 的析构、拷贝构造、`operator=` 必须齐全且都是**深拷贝**；`operator=` 必须自赋值安全（`M = M` 不得出错），且必须按 `other` 的容量重新分配（见 §3.1）。`Book` 无资源、不需要三件套；`Shelf` 的 `books` 是**指针成员**，`new[]`/`delete[]` 必须自己配对。
- **R6（越界安全）**：`remove(i)` 越界返回 `false` 且不修改对象、不访问 `books[i]`；`at(i)` 越界返回 `Book()`。
- **R4（动态内存）**：`new[]` ↔ `delete[]` 配对，无 leak / use-after-free / double free。
- **R3（边界）**：`cap == 1`、空书架、满书架、`R`/`P` 越界（含 `idx == len`、很大的 `idx`）、拷贝满书架、`L` 的书超过 `Shelf M;` 的默认容量 5。
- **R7（内存自测）**：用 `valgrind --leak-check=full --error-exitcode=1 ./a.out` 自测，要求 **0 errors、0 leaks**；没有 valgrind 就用 `g++ -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer`。**`-O2 -w` 下浅拷贝的 double free 很可能不崩**，别指望编译器。
- **R8（不改 `main`）**：`main` 一个字都不能改；不要加自己的 `cout`。
- 本题输入全部合法（不做 `cerr` 诊断），`q` 条操作一次运行处理完，不要求多组数据、无哨兵 `-1`（R1/R2 不适用于本题）。

**甲方保证**：`1 <= cap <= 500`，`1 <= q <= 1000`；`0 <= id <= 9999`，`0 <= pages <= 2000`；`R`/`P` 的下标非负。
**甲方不保证**：下标一定在 `0 .. len-1` 内（越界是考点）；也不保证 `C`/`E` 时书架是空的或未满的。

## 9. 自测清单

- [ ] 样例 1~5 的 stdout 与题面**逐字节**一致（`diff` 无输出）。
- [ ] `cap = 3` 放满后第 4 本 `full`（样例 1）。
- [ ] `cap = 1`（样例 2）：满、抽空、空架 `S`/`P`/`R` 都正确。
- [ ] `remove` 删**首位**、**中间**、**末位**各一次，并用连续 `P` 验证是"前移补位"而不是"末位覆盖"（样例 1/3/5）。
- [ ] 空书架上的 `S`（`0 0`）、`R 0`（`no`）、`P 0`（`0 0`）都不崩。
- [ ] 越界 `R`/`P`：`idx == count()`、`idx` 远大于 `count()`（如 `P 999`）都安全。
- [ ] `C` 在**未满**书架（样例 4）：副本加书成功、`L` 不变；`C` 在**满**书架（样例 5）：`M.add` 失败但两边都不变、不崩。
- [ ] `E`：`M = L` + `M = M` 数字正确，且 sanitizer 无报错。
- [ ] **自己写一个"容量大于 5 的 `L` + `E`"测试**：例如程序输入 `cap = 20`，先放 12 本书再执行 `E`。若 `operator=` 按 `M` 自己的默认容量 5 分配，这里会越界写；正确实现应输出 `12 总页数`。
- [ ] **自己写一个直接测类的测试**：`Shelf a(2); a.add(Book(1,10)); a.add(Book(2,20)); a.add(Book(3,30));`（第 3 本应 `false`）→ `Shelf b = a; b.add(Book(4,40));`（应 `false`，容量 2 已满）→ `b.remove(0)`（`b` 里应剩 `2 20`，`a` 仍是 2 本）→ `b = b;`（数据不变）。
- [ ] `q = 1000` 的大输入（`C`/`E`/`A`/`R`/`P`/`S` 混合、`cap` 很大与很小各一次）跑通，sanitizer 0 errors、0 leaks。
- [ ] 代码里没有 `vector`/`string`/`auto`/范围 `for`，也没有除 `operator=` 以外的运算符重载。

## 10. 常见错误

1. **默认拷贝构造（浅拷贝）**：`Shelf M = L;` 之后 `M.books == L.books`，两个对象析构时同一块内存被 `delete []` 两次 → double free。课件原话：*"the same memory will be freed twice, crash"*。必须自己写深拷贝。
2. **只写拷贝构造、忘了 `operator=`**（或反过来）：`C` 走拷贝构造、`E` 走赋值，两条路径都要深拷贝，缺一条就漏。
3. **`operator=` 不判自赋值**：`M = M` 时先 `delete [] books` 再读 `other.books` 就是 use-after-free。正确做法是先判 `this != &other`（或先申请新数组、拷完再释放旧的）。
4. **`operator=` 按自己的 `cap` 分配新数组**（本题最隐蔽的坑）：`E` 里 `Shelf M;` 的默认容量只有 5，而 `L` 可能有几十本书，按 5 分配再拷 `L.len` 本 ⇒ 越界写，`-O2` 下常常"样例全过、压力数据炸"。
5. **拷贝构造只把 `cap` 设成 `len`**：副本在原本未满的书架上被误判为满，`C` 输出错误（样例 4 会从 `2 300 3 301` 变成 `2 300 2 300`）。
6. **`add` 满时不返回 `false`**：要么悄悄越界写 `books[len]`，要么自己扩了容——本题**明确不扩容**，满就是 `full`。
7. **`remove` 用"末位覆盖"**（把最后一本搬到被删位置）：这是 Project 5 里 `Team::release` 的做法，本题**不允许**——顺序会乱，`P` 立刻出错。
8. **`remove` 忘记 `len--`**，或前移循环边界写错（`k < len` 而不是 `k < len - 1`），最后一位没清干净，`count()`/`at` 都错。
9. **`at` 返回引用或指针**：越界时无法返回"默认对象"（不能返回局部变量，第 7 周 *Never return reference to local variable*）；本题签名固定为**按值返回** `Book`，越界时 `return Book();`。
10. **`at`/`remove` 越界时仍访问 `books[i]`**：空书架上的 `P 0` 会读未初始化内存或崩。
11. **`Book` 少了默认参数**：`Book(int id, int pages)` 没有默认值 ⇒ `new Book[cap]` 编译不过。
12. **用 `vector`/`string` 代替动态数组**：直接违反 README 第 3 节，按规范项扣分。

## 11. 提交与评测

- 提交**单个 `.cpp` 文件**（命名 `P4.cpp`）：`#include` + 两个类的声明与实现 + 给定的 `main`，全部放在同一个文件里。
- 编译命令（README 第 1.1 节）：

  ```bash
  g++ -DONLINE_JUDGE -O2 -w -fmax-errors=3 -std=c++20 P4.cpp -lm -o P4
  ```

- 评分见 README 第 5 节：功能正确 50 + 健壮性 30 + 规范 20。健壮性重点看三件套是否齐全、`C`/`E` 输出是否正确、以及 sanitizer/valgrind 是否有 leak 或 double free。

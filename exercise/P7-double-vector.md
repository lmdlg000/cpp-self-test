# P7 · 动态数组类 DoubleVector（自增长 + 运算符重载）

> 难度：★★★★★　·　对应课件：第 3~7 周综合（多文件工程 + 资源三件套 + 运算符重载）　·　构建：多文件 + makefile（README 第 1.2 节）　·　提交：整个目录（含 makefile），必须命名 `Project7/`
>
> 通用约定（输入/输出格式、禁用项、健壮性 R1~R9、评分）见 [README](README.md)；本文只写本题的额外要求。>   
> **除"放在 `Project7` 目录、`main` 不同"以外，工程与健壮性要求大致与 [P6](P6-batch-makefile.md) 一致**（第 3 节 makefile 要求、第 8 节 R9 等条目都沿用 P6 的口径）。

## 1. 题目背景

传感器采集程序事先不知道会来多少个采样点：来一个存一个，容量不够了自己长大；下游模块还希望把它整体拷一份、两段序列首尾拼起来、按下标安全取值、直接 `cout` 出来看内容。请实现这样一个**双精度动态数组类 `DoubleVector`**——容量与元素数分离管理，全程**无内存泄漏、无 double free、无 use-after-free**（R4/R7）。

## 2. 考查内容

| 考点                                                                    | 出处                                                                            |
| --------------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| 头文件保护宏、`.h` 只放声明、`.C` 放实现、`makefile` 的 `all`/`clean`（R9）              | 第 3 周 *Header Files / Separate Declaration and Definition / makefile*；P6 同款要求 |
| `new double[]` / `delete[]` 配对、容量不足时**重新分配 + 搬数据 + 释放旧缓冲**            | 第 3 周 *new/delete*、*Common Errors*；第 6 周 *Vector 扩容*                          |
| 资源三件套：析构、拷贝构造、`operator=`（深拷贝、自赋值安全）                                  | 第 5~6 周 *Why Copy Constructor / Deep copy*；R5                                 |
| 运算符重载：`=`（赋值）、`+=`（队尾合并）、`[]`（取值）、`friend operator<<`，返回值/引用支持链式调用    | 第 6 周 slide 21~29（`operator=` / `+` / `++` / `[]`）；P5 的重载经验                   |
| `const` 正确性：`operator[]`、`getCount`、`operator<<` 都不修改对象，`const` 对象也能用 | 第 7 周 *const*；评分"规范"项                                                         |
| 越界下标**不访问内存**、返回默认值（R6）                                               | 第 5 周 Project 5 `get` / `at` 的做法                                              |
| 别名安全：`a += a` 时源缓冲可能在扩容中被释放（典型 use-after-free）                        | 第 5~6 周深拷贝 + 第 3 周悬空指针                                                        |

**与课件示例的区别**：课件第 6 周的 `Vector` 扩容示例是**课内演示版**（单文件、`[]` 非 `const`、`+=` 边遍历边 `push_back`）。本题是它的**工程升级版**：

- 拆成 `DoubleVector.h` / `DoubleVector.C` / `main.C` + `makefile` 四件套（R9）；
- `[]` 与 `getCount` 必须是 `const` 成员（隐藏 `main.C` 会用 `const DoubleVector` 对象调用），且越界返回 `0.0`（R6）；
- `+=` 必须扛住 `a += a` 的别名——课件版在自加时会在扩容中 `delete[]` 掉源缓冲，直接悬空；
- 还要处理**零容量**（`DoubleVector(0)`）、**空数组输出**、**自赋值**（`c = c`）这些边界。

与 P4 的图书架也不同：P4 容量固定、只考"拷贝语义"；本题容量**动态增长**，考的是"容量 `size` 与元素数 `count` 分离"的容器不变式。与 P5 的 `Rational` 相比，重载的运算符家族换成了容器语义（`[]`/`+=`），并且多了"扩容时旧指针失效"这一层。

## 3. 必做接口

提交一个**目录**（**必须命名 `Project7/`**，与 P6 的差别之一），里面**必须**有这 4 个文件：

| 文件               | 内容                                                                   |
| ---------------- | -------------------------------------------------------------------- |
| `DoubleVector.h` | `class DoubleVector` 的**声明** + 头文件保护宏                                |
| `DoubleVector.C` | `class DoubleVector` 全部成员与友元函数的**实现**，首行 `#include "DoubleVector.h"` |
| `main.C`         | 题目给定，**不得修改**                                                        |
| `makefile`       | 构建规则，要求与 P6 第 3 节完全一致（见下）                                            |

**`DoubleVector`（放在 `DoubleVector.h`，签名不得改动）**：

```cpp
class DoubleVector {
    size_t  size;   // 当前容量（可存元素个数），始终 >= count
    size_t  count;  // 当前元素个数
    double* data;   // 堆上缓冲区，长度 == size
    // 可自行增加私有辅助函数（如 grow 扩容），但不得增删改下列接口
    // 上面 3 个私有数据成员的**名字与类型固定**，不得增删改
public:
    DoubleVector(size_t sz = 10);
    DoubleVector(const DoubleVector& v);
    ~DoubleVector();
    void push_back(double x);
    DoubleVector& operator=(const DoubleVector& v);
    DoubleVector& operator+=(const DoubleVector& right);
    double operator[](int i) const;
    size_t getCount() const;

    friend std::ostream& operator<<(std::ostream& os, const DoubleVector& v);
};
```

- 三个私有数据成员固定为 `size_t size; size_t count; double* data;`，**名字、类型、个数都不得改**；在此之外可以增加私有辅助函数（例如 `void grow();`），但 public 接口一个都不能动；
- 三个成员的**语义与不变式**见第 6 节第 1 条（`size` 是容量、`count` 是元素数，`size >= count`）；
- `DoubleVector(size_t sz = 10)` 的**默认参数不能去掉**：`main.C` 里 `DoubleVector va;` 用默认容量，隐藏用例还会用 `DoubleVector(1)`、`DoubleVector(0)`；
- `operator[]` 与 `getCount` **必须带 `const`**（`double operator[](int i) const;`、`size_t getCount() const;`）：隐藏用例里有 `const DoubleVector k = d;` 之后 `k[3]`、`cout << k` 的写法，少了 `const` 就编译不过；
- `operator=` / `operator+=` **修改自身并返回引用**（供 `(vc += vb)` 这样链式使用、供 `c = d = e` 连等），`operator<<` 是 `friend`；
- `push_back` 的扩容策略固定：`count == size` 时容量**翻倍**（`size == 0` 时变成 `1`），容量**只增不减**。

**`main.C`（题目给定，请原样粘贴，不得修改）**：

```cpp
#include <iostream>
#include "DoubleVector.h"
using namespace std;

int main() {
    DoubleVector va;                      // 默认容量 10
    for (int i = 0; i < 5; ++i) {
        va.push_back(i * 0.5);            // 0, 0.5, 1, 1.5, 2
    }
    DoubleVector vb(3);                   // 容量 3，元素 0 个
    for (int i = 0; i < 8; ++i) {
        vb.push_back(i * 1.25 - 2.0);     // -2, -0.75, 0.5, 1.75, 3, 4.25, 5.5, 6.75
    }
    cout << va.getCount() << "," << vb.getCount() << endl;
    cout << va[2] << "," << va[10] << "," << va[-1] << endl;
    DoubleVector vc;
    vc = va;
    vc += vb;
    cout << vc.getCount() << ":" << vc << endl;
    DoubleVector vd = vc;
    vd.push_back(100.25);
    cout << vd.getCount() << ":" << vd << endl;
    cout << vc.getCount() << ":" << vc << endl;
    return 0;
}
```

`main.C` 只 `#include <iostream>` 与 `"DoubleVector.h"`；它负责"塞数据 → 拷/拼 → 印"。**任何对 `main.C` 的改动（包括多打一个空格、改 `using namespace`）都按 R8 处理。**

**`makefile`（要求与 P6 第 3 节完全一致，一份可用示例）**：

- 默认目标 `all` 生成可执行文件 **`Project7`**；
- `clean` 删除全部 `.o` 与 `Project7`；
- 变量 `CXX = g++`、`CXXFLAGS = -std=c++20 -O2 -w -Wall`，链接时用 `$(CXX) $(CXXFLAGS)`；
- 每个 `.o` 要写清依赖的 `.h`；**命令前必须是 Tab**（不是空格）；
- 不许写死绝对路径，也不许 `cd` 到别处。

```makefile
CXX = g++
CXXFLAGS = -std=c++20 -O2 -w -Wall
OBJ = main.o DoubleVector.o
TARGET = Project7

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

main.o: main.C DoubleVector.h
	$(CXX) $(CXXFLAGS) -c main.C

DoubleVector.o: DoubleVector.C DoubleVector.h
	$(CXX) $(CXXFLAGS) -c DoubleVector.C

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
```

目标名、变量名你可以自己起，但这些**行为**必须满足（R9，与 P6 相同）：`make` 能生成 `Project7`、`make clean` 能清干净、`make` 之后目录里只多出 `.o` 与 `Project7`。

> Windows 提示：MSYS2 的 `make` + `g++` 本地练习时产物是 `Project7.exe`（`TARGET` 仍写 `Project7`）；`make clean` 里 `rm -f` 由 MSYS2 的 `sh` 执行。评测机是 Linux，产物就是 `Project7`。

## 4. 输入格式

**无输入**。本题不读 `stdin`——`main.C` 里所有数据都是写死的常量（隐藏用例则是助教**替换 `main.C`** 后编译运行，`DoubleVector` 的接口签名保证了替换后仍然链接得上）。

## 5. 输出格式

输出**全部由 `main.C` 决定**，你只需要保证 `operator<<` 印对：

- 元素之间用**一个逗号** `,` 分隔，**逗号前后都没有空格**，末尾**不带逗号**；
- 空数组（`count == 0`）**什么字符都不印**（`cout << v` 之后紧跟 `endl` 就是一个空行）；
- 每个元素用 `cout` 的**默认格式**输出（6 位有效数字，README 第 2 节第 4 条）：`0.5`、`1`、`-0.75`、`100.25` 就照原样印，**不要**加 `fixed` / `setprecision` / `setw`，也不要自己补 `.0`；
- `operator<<` 返回 `ostream&`（`return os;`），以便 `cout << a << b` 连用；行末用 `endl`（`main.C` 已写好）；
- `stderr` 评测忽略：本题正常路径不需要任何诊断输出（调试信息提交前删掉）。

## 6. 计算规则与约束

1. **存储不变式（全题遵守）**：
   - `size` 是**容量**，`count` 是**元素个数**，恒有 `size >= count`；
   - `data` 指向一块长度恰为 `size` 的 `new double[size]` 缓冲，有效元素在 `data[0] .. data[count-1]`，`data[count] .. data[size-1]` 是未使用空间（内容无意义，不要输出）；
   - 容量**只增不减**：任何操作都不会让 `size` 变小（`operator=` 除外，它按源对象的容量重置，见第 5 条）；
   - 不允许出现 `data == NULL` 表示"空"的写法：`size == 0` 时 `new double[0]` 依然要调，析构时 `delete[]` 依然要配对。
2. **构造 `DoubleVector(size_t sz = 10)`**：`size = sz`、`count = 0`、`data = new double[sz]`。`sz == 0` 合法（容量 0，之后靠 `push_back` 自己长）。
3. **析构**：`delete[] data;`（必须是 `delete[]`，第 3 周 *Common Errors*）。
4. **拷贝构造 `DoubleVector(const DoubleVector& v)`**：**深拷贝**——`size = v.size`、`count = v.count`、`data = new double[size]`，再逐个复制 `v.data[0..count-1]`。绝不能让两个对象共用一块缓冲（否则析构两次 = double free）。
5. **`operator=`**：自赋值安全（`this == &v` 直接 `return *this`）；**先分配新的、再 `delete[]` 旧的**（P6 常见错误 8 的教训）；结果 `count == v.count`，容量与源一致（`size == v.size`），元素逐个复制；返回 `*this`。
6. **`push_back(double x)`**：
   - 若 `count < size`，直接 `data[count++] = x;`；
   - 若 `count == size`，先扩容：`newSize = (size == 0) ? 1 : size * 2`，`new double[newSize]`、搬 `count` 个旧元素、`delete[]` 旧缓冲、更新 `data` 与 `size`，再写入。
7. **`operator+=`（队尾合并）**：把 `right` 的 `count` 个元素**依次 `push_back`** 到 `*this` 尾部，返回 `*this`。
   - **必须扛住别名 `a += a`**：`push_back` 可能在扩容时 `delete[]` 掉当前缓冲，而 `right.data` 正指向那块内存——边遍历边 `push_back` 会立刻 use-after-free（结果错乱甚至崩溃）。
   - 推荐做法：先把 `right` 的 `count` 个元素抄进一个临时 `double[]`（或在扩容前把源数据保存下来），再逐个 `push_back`，最后 `delete[]` 临时缓冲。
   - `a += a` 的结果必须是"元素数翻倍、前后两段相同"，且不泄漏、不崩溃。
8. **`operator[](int i) const`（越界安全，R6）**：`i < 0` 或 `static_cast<size_t>(i) >= count` 时返回 `0.0`，**且绝不访问 `data[i]`**；否则返回 `data[i]`。参数是 `int`（不是 `size_t`），所以负下标是合法调用，必须自己判断。
9. **`getCount() const`**：返回 `count`（注意 `size` 是容量，不是元素数——这是本题最常见的口误）。
10. **`operator<<`**：`friend`、不修改对象；按 `data[0..count-1]` 顺序输出，相邻元素之间插一个 `,`（第 1 个元素前、最后一个元素后都不加）；`count == 0` 时不输出任何字符；返回 `os`。
11. **禁止**：`std::string`、`std::vector`、`std::array` 等所有 STL 容器与字符串设施；`auto`、范围 `for`、`lambda`、`template`、智能指针、`std::copy`、`<bits/stdc++.h>`、`rand`/`srand`/`time`（README 第 3 节）。数组操作手写循环。
12. **文件划分（R9，与 P6 相同）**：`.h` 只放类声明与保护宏（`inline` 成员函数除外），实现全部在 `DoubleVector.C`；`DoubleVector.C` 首行 `#include "DoubleVector.h"`；`main.C` 只 `#include` 头文件。

## 7. 样例

**【样例 1 输入】**（无输入，`main.C` 固定）

```text
```

**【样例 1 输出】**（stdout）

```text
5,8
1,0,0
13:0,0.5,1,1.5,2,-2,-0.75,0.5,1.75,3,4.25,5.5,6.75
14:0,0.5,1,1.5,2,-2,-0.75,0.5,1.75,3,4.25,5.5,6.75,100.25
13:0,0.5,1,1.5,2,-2,-0.75,0.5,1.75,3,4.25,5.5,6.75
```

**【样例 1 诊断】**（stderr，评测忽略）：无输出

**【说明】**

- **第 1 行**：`va` 塞了 5 个、`vb` 塞了 8 个，`getCount()` 必须报**元素数**（5 和 8），不是容量（10 和 8）；
- **第 2 行**：`va[2]` 是第 3 个元素 `1`；`va[10]` 超出 `count`（虽然还在容量 10 之内，但 `10 >= count` 就算越界）返回 `0`；`va[-1]` 负下标返回 `0`。三个值以 `1,0,0` 印出，说明"越界只看 `count`、不看 `size`"；
- **第 3 行**：`vc = va`（赋值）后 `vc` 有 5 个元素，再 `vc += vb` 拼上 8 个 → 13 个；`va` 只有 5 个元素，`size` 是 10，所以这一步**不扩容**；
- **第 4 行**：`vd = vc` 走**拷贝构造**（`DoubleVector vd = vc;` 是初始化，不是赋值），`vd` 再 `push_back(100.25)` → 14 个；此时 `vd` 从容量 20 起长，还能放下；
- **第 5 行**：`vc` 仍是 **13** 个、内容没变 —— 这就是**深拷贝**的直接证据：改 `vd` 不能影响 `vc`（浅拷贝在这里多半已经 double free 或内容串了）。

**样例 2 使用的 `main.C`（隐藏用例：助教替换 `main.C` 后重新 `make`；自测时可以临时换上去，跑完记得换回来）**：

```cpp
#include <iostream>
#include "DoubleVector.h"
using namespace std;

int main() {
    DoubleVector a;                        // 默认容量 10
    for (int i = 0; i < 6; ++i) {
        a.push_back(i + 0.5);              // 0.5 ... 5.5
    }
    a += a;                                // 别名：12 个元素，触发扩容
    cout << a.getCount() << ":" << a << endl;

    DoubleVector b(0);                     // 容量 0，必须能自己长出来
    for (int i = 0; i < 3; ++i) {
        b.push_back(-i * 0.25);            // 0, -0.25, -0.5
    }
    cout << b.getCount() << ":" << b << endl;

    DoubleVector e;                        // 空数组
    cout << e.getCount() << ":" << e << endl;

    DoubleVector c;
    c = c;                                 // 自赋值
    cout << c.getCount() << ":" << c << endl;

    DoubleVector d = b;                    // 深拷贝
    d.push_back(9.75);
    cout << b.getCount() << ":" << b << endl;
    cout << d.getCount() << ":" << d << endl;

    const DoubleVector k = d;              // const 对象：[] 与 << 必须能调用
    cout << k[3] << "," << k[100] << "," << k[-2] << endl;

    DoubleVector big(1);                   // 反复翻倍扩容
    for (int i = 0; i < 20; ++i) {
        big.push_back(i * 2.0 - 10.0);
    }
    cout << big.getCount() << ":" << big << endl;
    return 0;
}
```


**【样例 2 输入】**（无输入，`main.C` 换成上面那份，接口不变）

```text
```

**【样例 2 输出】**（stdout）

```text
12:0.5,1.5,2.5,3.5,4.5,5.5,0.5,1.5,2.5,3.5,4.5,5.5
3:0,-0.25,-0.5
0:
0:
3:0,-0.25,-0.5
4:0,-0.25,-0.5,9.75
9.75,0,0
20:-10,-8,-6,-4,-2,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28
```

**【样例 2 诊断】**（stderr，评测忽略）：无输出

**【说明】**

- **第 1 行（别名）**：`a` 有 6 个元素、容量 10，`a += a` 要再塞 6 个，前 4 个还装得下、后 2 个触发翻倍扩容——如果实现里"边遍历 `right.data` 边 `push_back`"，扩容那一下就把源缓冲释放了，这一行立刻出错或崩溃；
- **第 2 行（零容量）**：`DoubleVector b(0)` 的 `new double[0]` 合法，第一次 `push_back` 要把容量从 0 长到 1，之后再翻倍到 2、4；
- **第 3、4 行（空数组 / 自赋值）**：两个 `0:` —— 空数组不印任何元素（只剩 `:` 和换行），`c = c` 必须安全（先判 `this != &v`），`count` 还是 0；
- **第 5、6 行（深拷贝）**：给 `d` 加一个 `9.75` 之后，`b` 仍是 3 个元素 —— 两个对象的缓冲互不相干；
- **第 7 行（const 对象 + 越界）**：`const DoubleVector k = d;` 之后 `k[3]` 能编译（`operator[]` 有 `const`），值是 `9.75`；`k[100]`、`k[-2]` 越界返回 `0`；
- **第 8 行（反复扩容）**：容量从 1 一路 1→2→4→8→16→32，塞进 20 个元素，顺序与数值都不能乱。

## 8. 健壮性要求

通用要求见 README 第 4 节；**除下列本题额外/重点条目外，R9 等多文件工程要求与 P6 第 8 节一致**：

- **R1（可重复运行）**：同一程序跑两次输出必须完全一致（本题无输入，天然满足；但不许用 `static` 缓冲跨对象共享状态）。
- **R2（非法数据）**：本题无外部输入，不涉及诊断输出；调试用的 `cout` 必须删掉或进 `cerr`。
- **R3（边界）**：容量 0（`DoubleVector(0)`）、容量 1（`DoubleVector(1)`）、空数组、单元素、`count == size` 恰好装满、刚扩容完、`a += a` 别名、`c = c` 自赋值、负下标与超大下标（`v[-1]`、`v[100]`）、`DoubleVector vd = vc;` 与 `vd = vc;` 两种拷贝路径。
- **R4（动态内存）**：`new double[]` ↔ `delete[]` 严格配对（不许 `delete`、不许 `free`）；扩容后旧缓冲必须 `delete[]`；拷贝构造 / `operator=` 是**深拷贝**；`+=` 的临时缓冲不能漏。用 `valgrind --leak-check=full --error-exitcode=1 ./Project7` 或 `g++ -g -O1 -fsanitize=address,undefined main.C DoubleVector.C -o p7_asan` 自测（R7），要求 **0 errors、0 leaks**。
- **R5（资源三件套）**：析构 + 拷贝构造 + `operator=` 齐全且正确（默认浅拷贝会导致 double free——P6 常见错误 7 的同款坑）。
- **R6（越界不访问内存）**：`operator[]` 越界**返回 `0.0`**，不得读 `data[i]`、不得断言、不得抛异常。
- **R8（不改 `main.C`）**：原样粘贴，一个字符都不改；调试输出只能进 `cerr`，提交前删掉。
- **R9（多文件工程）**：与 P6 第 8 节该条完全相同——保护宏、`.h` 只声明、`makefile` 的 `all`/`clean` 可用、`make` 之后只多出 `.o` 与 `Project7`、不写死绝对路径。
- **本题额外要求**：`operator[]` 与 `getCount` 必须 `const`（否则隐藏 `main.C` 里的 `const DoubleVector` 编译失败 = CE）；`+=` 在 `a += a` 下结果正确且不泄漏；`operator<<` 不得修改被打印对象（传 `const` 引用）。

**甲方保证**：元素个数不超过 10000；元素是 `0.25` 的整数倍（二进制可精确表示，`cout` 默认格式不会出现浮点噪声）。**甲方不保证**：容量够用（随时可能触发扩容）；数组非空；下标在范围内；`+=` 的左右不是同一对象（但你必须自己防住）；`=` 的左右不是同一对象。

## 9. 自测清单

- [ ] 在 `Project7/` 目录里 `make`：生成 `Project7`，没有 `missing separator`；`make clean` 后三个产物（`main.o`、`DoubleVector.o`、`Project7`）全消失，再 `make` 仍成功；
- [ ] `./Project7` 输出与第 7 节样例 1 **逐字符**一致（含 `:` 与逗号、行末换行）；
- [ ] 把 `main.C` 换成样例 2 那份（自测完记得换回来）：输出与样例 2 逐字符一致；
- [ ] `getCount()` 报元素数而不是容量：`DoubleVector v;` 塞 3 个后 `getCount() == 3`（容量是 10）；
- [ ] 越界：`v[count]`、`v[count + 5]`、`v[-1]` 全返回 `0`，且 valgrind / ASan 无任何报告；
- [ ] 扩容：`DoubleVector v(1);` 塞 5 个元素，容量路径 1→2→4→8，元素顺序不错、不丢数据；
- [ ] 别名：`a += a`（6 个元素 → 12 个，前后两段相同），ASan 无 use-after-free；
- [ ] 自赋值：`c = c` 后对象完好；`a = a;` 也不崩；
- [ ] 深拷贝：`DoubleVector d = b; d.push_back(9.75);` 之后 `b` 的 `count` 与内容不变；
- [ ] 空数组：`cout << DoubleVector() << endl;` 输出一个空行（只有一个换行）；
- [ ] `valgrind --leak-check=full --error-exitcode=1 ./Project7`：`0 errors`、`definitely lost: 0 bytes`；或 ASan 版本跑通无报告；
- [ ] `main.C` 与题目给定版本逐字符比对（`diff main.C 题面版本` 应为空）。

## 10. 常见错误

1. **makefile 用空格缩进**（`missing separator`）、**`clean` 漏删可执行文件**、**写死绝对路径**、**头文件没保护宏**、**`.C` 忘了 `#include "DoubleVector.h"`** —— P6 常见错误 1~5 原样适用。
2. **把容量当元素数**：`getCount()` 返回 `size`（容量）→ 样例 1 第 1 行印成 `10,8`；`operator<<` 按 `size` 遍历 → 印出未初始化的垃圾值（还可能是 `nan`）。
3. **`operator[]` 拿 `size` 判越界**：`va[10]` 在容量 10 的数组里"看起来合法"，于是返回未使用空间的垃圾；判越界只能看 `count`。
4. **浅拷贝**：不写拷贝构造 / `operator=`，`DoubleVector d = b` 后两个对象共用 `data`，析构两次 → double free（P6 常见错误 7）。
5. **`operator=` 先 `delete[] data` 再从 `v` 拷贝**：`d = d` 自赋值时 `v.data` 已是悬空指针 → use-after-free（P6 常见错误 8）。要先判自赋值，或先 `new` 新缓冲再删旧的。
6. **`operator=` 忘了改 `size`**：只改了 `count` 和内容，容量还是旧的 —— 之后 `push_back` 会写到自己以为还有的空间里。
7. **扩容时忘了搬数据**：`new double[newSize]` 之后直接 `delete[] data`，旧元素全丢；或者先 `delete[]` 再 `new`，缓冲区悬空。
8. **扩容后没更新 `size`**：容量还是旧值 → 下一次 `push_back` 又"满了"，或者写越界。
9. **`DoubleVector(0)` 用 `data = NULL` 走"懒分配"**：紧接着 `push_back` 里 `count == size`（0 == 0）成立却没进扩容分支（比如写成 `if (count >= size && size > 0)`）→ 往空指针写，段错误。记住 0 容量也要扩容到 1。
10. **`+=` 边遍历边 `push_back`**：`a += a` 时扩容 `delete[]` 掉 `right.data` 指向的缓冲 → 读到已释放内存（值错乱 / 崩溃 / 死循环）。先把源元素抄到临时缓冲。
11. **`+=` 返回 `void`（或返回值而非引用）**：`(vc += vb)` 要能直接进 `operator<<`，必须返回 `DoubleVector&`（返回 `DoubleVector` 也行但会多一次拷贝，属于规范扣分项）。
12. **把 `right` 当成本对象的私有副本**：参数是 `const DoubleVector& right`，既不能改它，也不保证它和 `*this` 是两块内存——`a += a` 时两者就是同一个对象，任何"先动 `count` 再读 `right`"的写法都会把源数据算乱。
13. **`operator<<` 写成普通成员函数**：`v << cout` 才能用（顺序反了）→ `cout << v` 编译失败。必须是 `friend`（或全局函数）。
14. **`operator<<` 用模板/`std::string` 拼逗号**：禁用项；而且多印一个尾逗号（`...,6.75,`）直接 WA。
15. **空数组印 `(empty)` 或印一个 `,`**：规格是"什么都不印"。
16. **`operator[]` / `getCount` 漏了 `const`**：隐藏用例里 `const DoubleVector k = d; k[3]` 编译不过 → 整题 CE。
17. **`new`/`delete` 混用**：`new double[n]` 必须配 `delete[]`（第 3 周 *Common Errors*）；`delete data;` 是 UB。
18. **改动 `main.C`**：哪怕"只是加个空格"也按 R8 算；调试输出进 `cerr` 并记得删。
19. **`push_back` 里写成 `data[count] = x; count++;` 但扩容判断放在后面**：满了的那一次就写越界（`-fsanitize=address` 会直接报 heap-buffer-overflow）。
20. **`cout` 加 `fixed` / `setprecision`**：输出格式会变成 `0.500000`，与样例不符（本题用默认 6 位有效数字）。

## 11. 提交与评测

**本题不在单文件 OJ 上提交**，也不用 README 第 1.1 节的 `g++ ... {src_path}` 命令。请提交整个工程目录 **`Project7/`**，里面恰好包含：

```text
Project7/
├── DoubleVector.h
├── DoubleVector.C
├── main.C
└── makefile
```

助教的评测流程（与 P6 相同，只是目录与产物名不同）：

```bash
cd Project7
make                  # 生成可执行文件 Project7
./Project7            # 本题无输入，直接跑
make clean            # 清理，确认没有多余产物
```

- **不要**提交 `.o`、`Project7`、`.zip` 里的多余目录（`make` 之后只应该多出 `.o` 与 `Project7`）。
- `makefile` 里 `CXXFLAGS` 必须包含 `-std=c++20 -O2 -w -Wall`（同 P6）。
- 限制：每次运行 1 s / 256 MB。
- **评测方式**：隐藏用例是助教**替换 `main.C`**（换成别的 `push_back`/`+=`/`=`/下标组合，并构造 `const` 对象）后 `make && ./Project7`，逐字符比对输出；因此**接口签名一个都不能改**（尤其 `operator[]` / `getCount` 的 `const`），改动 = 链接失败或编译失败 = CE。
- 评分：功能 50（固定样例 + 隐藏 `main` 用例）、健壮性 30（R1~R9，重点 R4/R5/R6/R7）、规范 20（接口签名、文件划分、`makefile` 质量、可读性、`return 0`）。
- 自测时先把 `make` 与 `make clean` 各跑一遍，再逐字符比对样例输出，最后务必过一遍 valgrind/ASan（重点看 `+=` 的别名与扩容路径）。

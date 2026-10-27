# P6 · 质检批次汇总

> 难度：★★★★　·　对应课件：第 3~5 周综合　·　构建：多文件 + makefile（README 第 1.2 节）　·　提交：整个目录（含 makefile）
>
> 通用约定（评测命令、输入/输出格式、禁用项、健壮性 R1~R9、评分）见 [README](README.md)；本文只写本题的额外要求。

## 1. 题目背景

工厂质检线按"批次"记录抽检读数：一次抽检是一个批次，批次里是一串测量值（合法范围 `0`~`1000`，超范围说明仪器误报，要报警但不能算进成绩）。全部批次跑完后，质检员要拿到每批的**件数 / 最小值 / 最大值 / 总和**，还要拿到整场的**总件数与全局平均**，以及"**最不稳定批次**"——批内极差（最大值 − 最小值）最大的那一批。

本题是**多文件工程题**：不是把代码塞进一个 `.cpp`，而是要按课件第 3 周的做法拆成 `.h` + `.C` + `makefile`，用 `make` 构建。

## 2. 考查内容

| 考点 | 出处 |
| --- | --- |
| 头文件：`#ifndef _X_H_ / #define _X_H_ / #endif` 保护宏；`.h` 只放声明 | 第 3 周 *Header Files* |
| 声明与实现分离：成员函数定义写在同名 `.C` 里，`#include "X.h"` | 第 3 周 *Separate Declaration and Definition* |
| `makefile`：默认目标、目标依赖、**Tab 缩进**、`all` / `clean`、变量（`CXX` / `CXXFLAGS` / `$(OBJ)`） | 第 3 周 *makefile* |
| `new int[n]` / `delete[]`、扩容四步（`new` → 拷贝 → `delete[]` 旧 → 改指针） | 第 3 周 *Extend an array* |
| 二维动态数组：`Batch**`（对象指针数组）+ 逐层清理 | 第 4 周 slide 27~29；第 5 周 slide 37 |
| 类、构造函数、析构函数、对象指针成员、拷贝构造与深拷贝、`operator=` | 第 5 周 slide 8~18、32~42 |

**与课件示例的区别**：课件用 Project 3 的 `game()` / `oneRound()`（猜数字计分求标准差）和 Project 4 的 `Game` / `Round`（合并首末轮、`Merge2` / `ReportAve`）来演示二维动态数组与对象指针。本题是**质检批次汇总**：不猜数字、不算标准差、不做回合合并、没有任何随机数；场景、类名（`Batch` / `Inspector`）、方法（`minValue` / `maxValue` / `sum` / `report`）都是新的。真正与课件不同的硬要求是 **R9：必须是能 `make` 的多文件工程**——课件只在幻灯片里演示过 `makefile`，没有留成作业。

## 3. 必做接口

提交一个**目录**（建议命名 `P6/`），里面**必须**有这 6 个文件：

| 文件 | 内容 |
| --- | --- |
| `Measurement.h` | `class Batch` 的**声明** + 头文件保护宏 |
| `Measurement.C` | `class Batch` 全部成员函数的**实现**，首行 `#include "Measurement.h"` |
| `Inspector.h` | `class Inspector` 的**声明** + 头文件保护宏（内部需要 `Batch`，可 `#include "Measurement.h"`） |
| `Inspector.C` | `class Inspector` 全部成员函数的**实现**，首行 `#include "Inspector.h"`，用到 `cin/cout/cerr` 时再加 `#include <iostream>` |
| `main.C` | 题目给定，**不得修改** |
| `makefile` | 构建规则，见下面的要求与示例 |

**`Batch`（放在 `Measurement.h`，签名不得改动）**：

```cpp
class Batch {
    int* v; int len; int cap;      // cap 初始 4，满时翻倍
public:
    Batch(); Batch(const Batch& o); ~Batch(); Batch& operator=(const Batch& o);
    void add(int x); int size() const; int get(int i) const;   // get 越界返回 0
    int minValue() const; int maxValue() const; long long sum() const;   // len==0 时返回 0
};
```

**`Inspector`（放在 `Inspector.h`，签名不得改动）**：

```cpp
class Inspector {
    Batch** b; int count; int cap;   // cap 初始 2，满时翻倍
public:
    Inspector(); ~Inspector();
    void run();          // 读入所有批次：每批 new 一个 Batch，用拷贝构造放进堆（b[count++] = new Batch(tmp);）
    void report() const; // 输出
};
```

**`main.C`（题目给定，请原样粘贴，不得修改）**：

```cpp
#include "Inspector.h"

int main() {
    Inspector ins;
    ins.run();
    ins.report();
    return 0;
}
```

`main.C` 只 `#include "Inspector.h"`，不 `#include` 任何 `.C`；它只负责"构造 → `run()` → `report()` → `return 0`"。

**`makefile`（要求 + 一份可用示例）**：

- 默认目标 `all` 生成可执行文件 `inspector`；
- `clean` 删除全部 `.o` 与 `inspector`；
- 变量 `CXX = g++`、`CXXFLAGS = -std=c++20 -O2 -w -Wall`，链接时用 `$(CXX) $(CXXFLAGS)`；
- 每个 `.o` 要写清依赖的 `.h`；**命令前必须是 Tab**（不是空格）；
- 不许写死绝对路径，也不许 `cd` 到别处。

```makefile
CXX = g++
CXXFLAGS = -std=c++20 -O2 -w -Wall
OBJ = main.o Measurement.o Inspector.o
TARGET = inspector

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

main.o: main.C Inspector.h Measurement.h
	$(CXX) $(CXXFLAGS) -c main.C

Measurement.o: Measurement.C Measurement.h
	$(CXX) $(CXXFLAGS) -c Measurement.C

Inspector.o: Inspector.C Inspector.h Measurement.h
	$(CXX) $(CXXFLAGS) -c Inspector.C

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
```

目标名、变量名你可以自己起，但上面这些**行为**必须满足（R9）：`make` 能生成 `inspector`、`make clean` 能清干净、`make` 之后目录里只多出 `.o` 与 `inspector`。

> Windows 提示：如果用 MSYS2 的 `make` + `g++` 本地练习，可执行文件会落地成 `inspector.exe`（`TARGET` 仍写 `inspector` 即可）；`make clean` 里那行 `rm -f` 由 MSYS2 的 `sh` 执行。评测机是 Linux，产物就是 `inspector`。

## 4. 输入格式

一串**空白分隔的整数**（可以写在一行，也可以写成很多行；不要依赖行结构，README 第 2 节第 2 条）。

- 每一**批**是若干测量值，以 `-1` 结束该批；
- **整场以"某一批的第一个数就是 `-1`"结束**：最后一个批次的 `-1` 之后再读到一个 `-1`，表示没有更多批次了；
- 合法测量值是 `0`~`1000`；`< 0`（`-1` 除外）或 `> 1000` 是非法读数；
- 输入末尾不要再补多余的 `-1`。

例：`8 15 12 -1 20 -1 -1` 表示两批：`[8, 15, 12]` 和 `[20]`，然后整场结束。

## 5. 输出格式

`report()` 输出若干行：

1. **每批一行**（按读入顺序，从第 0 批开始）：`件数 最小 最大 总和`，四个整数、单空格分隔；
2. **最后一行**：`总件数 全局平均 极差最大批次下标 该批极差`；
   - 全局平均是 `double`，用 `cout` 默认格式（6 位有效数字）；
   - "极差最大批次下标"从 `0` 开始编号；**并列时取下标最小的那一批**；
   - **总件数为 0 时**，这一行固定输出 `0 0 -1 0`。

行末不要多余空格，末尾只有一个换行；不要输出任何提示语（调试信息只能进 `cerr`）。

## 6. 计算规则与约束

1. **批的划分只看 `-1`**，与数据是否合法无关：一个批次可以因为读数全部非法而**没有一件有效数据（空批）**。
2. **非法读数**：`x < 0`（`-1` 除外）或 `x > 1000` 时输出诊断并跳过，不计入任何统计：
   ```cpp
   cerr << "invalid: " << x << endl;
   ```
3. **`Batch` 的容量**：初始 4，`add` 时 `len == cap` 就把容量翻倍（`new` → 拷贝 → `delete[]` 旧的 → 改指针），再放到末尾。一个批次可能有几百件，必须真的扩容。
4. **`Inspector` 的容量**：`b` 初始指向 2 个 `Batch*` 槽位，`count == cap` 时把槽位数翻倍（`new` 新外层 → 拷 `count` 个指针 → `delete[]` 旧外层），**不要**动 `b[i]` 指向的 `Batch` 对象。
5. **所有权**：`Batch::v` 归 `Batch`（`~Batch` 里 `delete[] v`）；`b[i]` 指向的 `Batch` 归 `Inspector`（`~Inspector` 里**逐个 `delete b[i]`**，再 `delete[] b`）；`run()` 里栈上的临时批次通过 `new Batch(tmp)` 拷进堆（深拷贝），**绝不能把栈对象的地址存进 `b[]`**。
6. **统计口径**：`件数` 是该批有效读数个数；`最小`/`最大` 是批内有效读数的极值；`总和` 是该批有效读数之和（用 `long long`，几百件 × 1000 也不会溢出 `int`，但仍请用 `long long`）。
   - 空批（`len == 0`）：`minValue` / `maxValue` / `sum` 都返回 `0`，该行输出 `0 0 0 0`，其极差按 `0` 计。
7. **总件数** = 所有批次件数之和；**全局平均** = 全部有效读数之和 ÷ 总件数（用 `double`，别写成整除）；总件数为 0 时整行固定 `0 0 -1 0`（此时不要去算平均、也不要去取"极差最大批次"）。
8. **极差** = 该批 `maxValue() - minValue()`；空批极差为 0；取极差最大的批次下标，并列取最小下标。
9. **禁止**：`vector`、`string` 等 STL 容器、`auto`、范围 `for`、`lambda`、`template`、`<bits/stdc++.h>`、`rand` / `srand` / `time`；除 `Batch` 的拷贝赋值 `operator=`（题面要求）之外，不要用运算符重载。
10. **文件划分（R9）**：`.h` 里只放类声明与保护宏（`inline` 成员函数除外），实现必须写在 `.C` 里；`.C` 必须 `#include` 自己的头文件；`main.C` 只 `#include` 头文件。

## 7. 样例

**【样例 1 输入】**

```text
8 15 12 -1 -1
```

**【样例 1 输出】**（stdout）

```text
3 8 15 35
3 11.6667 0 7
```

**【样例 1 诊断】**（stderr，评测忽略）：无输出

**【说明】**

单批：`[8, 15, 12]`，`8 + 15 + 12 = 35`，极差 `15 - 8 = 7`。总件数 3，全局平均 `35 / 3 = 11.6667`；只有一批，极差最大批次下标是 `0`。

**【样例 2 输入】**

```text
20 -1 5 25 -1 -1
```

**【样例 2 输出】**（stdout）

```text
1 20 20 20
2 5 25 30
3 16.6667 1 20
```

**【样例 2 诊断】**（stderr，评测忽略）：无输出

**【说明】**

两批：`[20]` 与 `[5, 25]`。两批的件数、极值分别打印；总件数 `1 + 2 = 3`，全局平均 `50 / 3 = 16.6667`；极差 `0` 与 `20`，所以"极差最大批次"是下标 `1`、极差 `20`。

**【样例 3 输入】**

```text
1500 10 2000 -1 30 -1 40 45 -1 -1
```

**【样例 3 输出】**（stdout）

```text
1 10 10 10
1 30 30 30
2 40 45 85
4 31.25 2 5
```

**【样例 3 诊断】**（stderr，评测忽略）：

```text
invalid: 1500
invalid: 2000
```

**【说明】**

三批：`[10]`、`[30]`、`[40, 45]`；`1500` 与 `2000` 超过 1000，是非法读数，被 `cerr` 记录后跳过（**不结束批次**）。总件数 4，总和 `10 + 30 + 85 = 125`，全局平均 `31.25`；极差分别是 `0`、`0`、`5`，所以下标 `2`、极差 `5`。

**【样例 4 输入】**

```text
-1
```

**【样例 4 输出】**（stdout）

```text
0 0 -1 0
```

**【样例 4 诊断】**（stderr，评测忽略）：无输出

**【说明】**

边界：整场一开始就是 `-1`，**一个批次都没有**。`report()` 不打印任何批次行，只打印最后一行的 `0 0 -1 0`；`run()` 不能解引用 `b[0]`，`~Inspector` 也不能崩。

**【样例 5 输入】**

```text
0 -1 10 10 10 10 10 10 10 10 10 -1 -1
```

**【样例 5 输出】**（stdout）

```text
1 0 0 0
9 10 10 90
10 9 0 0
```

**【样例 5 诊断】**（stderr，评测忽略）：无输出

**【说明】**

两批件数悬殊：`[0]`（1 件）与 9 个 `10`。总件数 10，总和 90，全局平均 `9`。如果错误地写成"先把每批平均一下再平均"，会得到 `(0 + 10) / 2 = 5` —— **必须按件数加权**。两批极差都是 0，并列时取最小下标 `0`。

**【样例 6 输入】**

```text
2000 3000 -1 -1
```

**【样例 6 输出】**（stdout）

```text
0 0 0 0
0 0 -1 0
```

**【样例 6 诊断】**（stderr，评测忽略）：

```text
invalid: 2000
invalid: 3000
```

**【说明】**

边界：这一批的读数（`2000`、`3000`）全部非法，于是得到一个**空批**：该批行输出 `0 0 0 0`（`minValue` / `maxValue` / `sum` 在 `len == 0` 时都返回 `0`，不许去读 `v[0]`）；总件数为 0，所以最后一行是 `0 0 -1 0`。

## 8. 健壮性要求

通用要求见 README 第 4 节，本题额外/重点要求：

- **R1（多批）**：一个进程内跑完整场，批次数从 0 到几十批都要能处理；不能写死"最多 2 批"。
- **R2（非法数据）**：越界读数用 `cerr << "invalid: " << x << endl;` 提示后跳过，**不计入**件数/极值/总和，`stdout` 不受影响。
- **R3（边界）**：0 批（只有 `-1`）、1 批、空批（整批非法）、全 0 值、全 1000 值、极差并列、件数悬殊的两批。
- **R4（动态内存）**：`Batch` 里 `new int[]` ↔ `delete[]`；`Inspector` 里 `new Batch*[]` ↔ `delete[]`，`b[i]` ↔ `delete`；扩容只 `delete[]` 旧外层；`run()` 里栈上临时批次不能把地址存进 `b[]`。用 `valgrind --leak-check=full --error-exitcode=1 ./inspector < in.txt` 或 `-fsanitize=address,undefined` 自测（R7），要求 `0 errors`、`definitely lost: 0 bytes`。
- **R5（资源三件套）**：`Batch` 必须自己写析构、拷贝构造、`operator=`（深拷贝、自赋值安全）。默认版本是浅拷贝，`new Batch(tmp)` 之后两个对象会共用一块 `int` 数组，析构两次就崩。
- **R6（越界不访问内存）**：`Batch::get(i)` 在 `i < 0 || i >= len` 时返回 `0`，**不能**返回 `v[i]`；空批的 `minValue` / `maxValue` / `sum` 一律返回 `0`。
- **R8（不改 `main.C`）**：`main.C` 原样粘贴；调试输出只能进 `cerr`，提交前删掉。
- **R9（多文件工程）**：头文件保护宏齐全、`.h` 只放声明、`.C` 只放实现、`makefile` 的 `all` / `clean` 均可用、`make` 之后只多出 `.o` 与 `inspector`、换一台机器 `make` 仍然成功（不许写死绝对路径）。
- **本题额外要求**：`report()` 必须是 `const` 成员函数且不修改任何状态；`Inspector` 不在 `report()` 里泄漏临时对象。

**甲方保证**：输入是一串空白分隔整数；`-1` 只作为结束标记；非法读数一定是越界整数（如 `-5`、`2000`）；批次数与每批件数都可能很大（几十批、每批几百件）；一整场的结束标记之后不会再有输入。**甲方不保证**：每批至少有一个合法读数（可能是空批）；总件数一定大于 0。

## 9. 自测清单

- [ ] 在 `P6/` 目录里 `make`：能生成 `inspector`，没有 `missing separator` 之类的报错；
- [ ] `make` 之后再 `ls`：目录里只多出 `main.o`、`Measurement.o`、`Inspector.o`、`inspector`；
- [ ] `make clean`：四个产物全部消失；再 `make` 一次仍然成功（可重复构建）；
- [ ] 把 `P6/` 整个拷到另一个路径（或另一台机器）再 `make`，仍然成功（不能有绝对路径）；
- [ ] 单批：`8 15 12 -1 -1` → `3 8 15 35` 与 `3 11.6667 0 7`；
- [ ] 两批：`20 -1 5 25 -1 -1` → 两行批次 + `3 16.6667 1 20`；
- [ ] 多批 + 非法：`1500 10 2000 -1 30 -1 40 45 -1 -1`；
- [ ] 空场：只有 `-1` → 只输出 `0 0 -1 0`；
- [ ] 空批：`2000 3000 -1 -1` → `0 0 0 0` 与 `0 0 -1 0`；
- [ ] 加权验证：`0 -1 10 10 10 10 10 10 10 10 10 -1 -1` → 全局平均 `9`（不是 `5`）；
- [ ] 扩容：一批 300 件（`Batch` 从 4 翻到 512）、20 批以上（`Inspector` 从 2 翻到 32）；
- [ ] `valgrind --leak-check=full --error-exitcode=1 ./inspector < 某个样例.in`：`0 errors`、`definitely lost: 0 bytes`；或 `g++ -g -O1 -fsanitize=address,undefined main.C Measurement.C Inspector.C -o inspector_asan` 跑全部样例无报告。

## 10. 常见错误

1. **makefile 用空格缩进**：`missing separator` —— 命令行的缩进必须是 **Tab**。编辑器里别把 Tab 自动换成空格。
2. **`clean` 漏删可执行文件**：`make clean` 之后 `inspector` 还在，第二次 `make` 就"什么都不做"，隐藏用例可能跑的是旧程序。
3. **写死绝对路径**：`g++ -I/home/xxx/P6 ...` 换机器就挂。
4. **头文件没保护宏**，或在 `.h` 里写函数定义：`main.o` 与 `Inspector.o` 都包含它时，链接报 `multiple definition`。
5. **`.C` 忘了 `#include "X.h"`**：`Batch` / `Inspector` 未声明，编译报错。
6. **`b[count] = &tmp;`**：把栈上临时批次的地址存进堆数组，`run()` 一返回就是悬空指针（R4）。必须 `new Batch(tmp)`。
7. **浅拷贝**：不写 `Batch` 的拷贝构造 / `operator=`，`new Batch(tmp)` 后两个对象共用 `int` 数组，析构两次 → double free（第 5 周 *Why Copy Constructor*）。
8. **`operator=` 自赋值**：先 `delete[] v` 再从 `o` 拷贝，而 `o` 就是自己 → use-after-free；要先判 `this != &o`（最好先 `new` 再删旧的）。
9. **`~Inspector` 只删外层**：忘了逐批 `delete b[i]`，几百个 `Batch` 全泄漏。
10. **`delete` / `delete[]` 混用**：`new Batch` 用 `delete`，`new Batch*[n]` 用 `delete[]`，`new int[n]` 用 `delete[]`（第 3 周 *Common Errors*）。
11. **扩容时把 `Batch` 对象也删了**：扩容只该 `delete[]` 旧外层指针数组。
12. **`get(i)` 越界返回 `v[i]`**，或者空批直接读 `v[0]` 求极值（`len == 0` 时 `v` 里是垃圾）。
13. **整数除法**：`double avg = totalSum / totalCnt;` 得到整除（`125/4` 变成 `31`）；要 `(double)totalSum / totalCnt`。
14. **"各批平均再平均"**：必须按件数加权（样例 5 专门考）。
15. **极差并列取错了批次**：必须取下标最小者；比较用 `>`（严格大于）而不是 `>=`。
16. **总件数为 0 时还去算平均/找最大极差**：除以 0 或访问 `b[-1]`。
17. **把 `-1` 当非法读数**：`-1` 是批次结束标记，不要对它输出 `invalid`。
18. **在 `report()` 里往 `stdout` 打调试信息**：一律 WA；调试用 `cerr` 并记得删。

## 11. 提交与评测

**本题不在单文件 OJ 上提交**，也不用 README 第 1.1 节的 `g++ ... {src_path}` 命令。请提交整个工程目录（建议 `P6/`），里面恰好包含：

```text
P6/
├── Measurement.h
├── Measurement.C
├── Inspector.h
├── Inspector.C
├── main.C
└── makefile
```

助教的评测流程：

```bash
cd P6
make                  # 生成可执行文件 inspector
./inspector < input.txt > output.txt
make clean            # 清理，确认没有多余产物
```

- **不要**提交 `.o`、`inspector`、输入/输出样例文件、`.zip` 里的多余目录（`make` 之后只应该多出 `.o` 与 `inspector`，其它文件一律由 `makefile` 产生或提交时就不存在）。
- `makefile` 里 `CXXFLAGS` 必须包含 `-std=c++20 -O2 -w -Wall`（`-w` 会关掉警告，但 R9 检查的是"能不能构建成功"，不是"有没有警告"）。
- 限制：每组数据 1 s / 256 MB。
- 评分：功能 50（样例 + 隐藏用例）、健壮性 30（R1~R9）、规范 20（接口签名、文件划分、`makefile` 质量、可读性、`return 0`）。
- 自测时先把 `make` 与 `make clean` 各跑一遍，再逐字符比对样例输出（含空格、小数点与换行）。

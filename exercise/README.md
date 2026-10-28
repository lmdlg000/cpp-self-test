# FCAA101-02 程序设计语言A · 练习 Project 集

> 依据 2026 秋课件（第 1~7 周，`0825` ~ `1020` 共 7 份讲义）设计，共 **7 道练习 project**，难度递进。
>
> **考点与健壮性要求严格对齐课件，但题目场景全部重新设计，不与课件的课堂示例、课堂作业、以及 `Project 1~6` 重复**（第 8 节给出对照表与"禁止照抄"清单）。换句话说：练的是同一批能力，做的是没见过的新题。
>
> 本目录（`exercise/`）**只放题面 markdown**，不提供参考实现。

## 0. 题目目录

| 编号 | 文件 | 题目 | 主要考点 | 课件依据 | 构建方式 | 难度 |
| --- | --- | --- | --- | --- | --- | --- |
| P1 | [P1-shipping.md](P1-shipping.md) | 快递运费结算器 | 变量与整数运算、向上取整、分支、循环、函数、哨兵、`cerr` | 第 1~2 周 | 单文件 | ★ |
| P2 | [P2-health-stats.md](P2-health-stats.md) | 体检数据统计 | 数组、函数原型、引用参数、手写排序、浮点输出、`new`/`delete[]` | 第 2~3 周 | 单文件 | ★★ |
| P3 | [P3-minesweeper.md](P3-minesweeper.md) | 扫雷雷数图 | `int**` 动态二维数组、逐行 `delete[]`、越界安全访问、双重循环 | 第 3~4 周 | 单文件 | ★★★ |
| P4 | [P4-shelf-class.md](P4-shelf-class.md) | 图书架类：固定容量与拷贝语义 | 类、构造函数、析构函数、深拷贝构造、`operator=`、`const` 成员函数 | 第 5~6 周 | 单文件 | ★★★ |
| P5 | [P5-rational.md](P5-rational.md) | 有理数类 Rational | 运算符重载、`friend operator<<`、`const` 正确性、`char*` 文本化、GCD | 第 6~7 周 | 单文件 | ★★★★ |
| P6 | [P6-batch-makefile.md](P6-batch-makefile.md) | 质检批次汇总（多文件工程） | 多文件工程、`makefile`、`Batch*` 指针数组、二维动态内存清理 | 第 3~5 周综合 | **多文件 + `make`** | ★★★★ |
| P7 | [P7-double-vector.md](P7-double-vector.md) | 动态数组类 DoubleVector（自增长 + 运算符重载） | 多文件工程、资源三件套、`=`/`+=`/`[]`/`<<` 重载、容量翻倍扩容、`const` 正确性、越界返回 `0.0` | 第 3~7 周综合 | **多文件 + `make`** | ★★★★★ |

建议顺序：P1 → P2 → P3 → P4 → P5 → P6 → P7。P6 是第一道"工程题"，P7 在工程骨架之上再加"容量自增长 + 运算符重载 + 别名安全"，放在最后做。

## 1. 构建与评测方式

### 1.1 单文件题（P1 ~ P5）

全部代码写在**一个 `.cpp`** 里，用固定命令编译（GCC 13 / C++20）：

```bash
g++ -DONLINE_JUDGE -O2 -w -fmax-errors=3 -std=c++20 {src_path} -lm -o {exe_path}
```

- **`-w` 关闭了全部警告**：编译通过 ≠ 程序正确。数组越界、变量未初始化、内存泄漏、`delete` 与 `delete[]` 混用、函数漏 `return`，编译器都不会拦你（第 3 周 *Common Errors*）。请自己保证内存安全。
- `{src_path}` 就是提交的那一个文件，评测机只会编译它。**不要提交多文件**，也不要在 `.cpp` 里 `#include` 自己另外写的 `.h`（本地可以，提交前必须合并）。
- 建议限制：每组数据 1 s / 256 MB。**栈上不要开大数组**（`int a[1000000];` 会爆栈），大数组用 `new`（第 3 周 *Array size unknown*）。
- `-DONLINE_JUDGE` 已定义：可以用它隔离本地调试输出，例如

  ```cpp
  #ifndef ONLINE_JUDGE
      cout << "debug: k = " << k << endl;
  #endif
  ```

  **评测时任何多余的 `cout` 都会 WA**。

### 1.2 多文件工程题（P6、P7）

P6、P7 是**多文件工程题，不走上面的单文件 OJ 命令**：必须提交目录 + `makefile`，用

```bash
make          # 生成可执行文件
make clean    # 清理中间文件与可执行文件
```

构建。题面里写明了必须包含哪些 `.h` / `.C` 文件、`makefile` 的必需目标（`all` / `clean`）、以及"main.C 只负责调用、不得改"。评分时助教直接 `make` 后跑隐藏输入。

多文件题请遵守课件第 3/5/6 周的做法：类声明放 `.h`（带 `#ifndef _X_H_ / #define _X_H_ / #endif` 头文件保护），成员函数实现放同名 `.C`（`.C` 里 `#include "X.h"`），`main.C` 只 `#include` 头文件；编译命令由 `makefile` 给出，不要写死绝对路径。

### 1.3 两种方式的共同底线

- 不许用 `#include <bits/stdc++.h>`；按课件包含 `<iostream>`、`<cstdlib>`、`<cmath>`、`<cstring>`、`<cctype>`、`<cstddef>`。
- 单文件题里可以写多个类，但不许把文件拆开；多文件题里不许把实现塞进 `.h`（`inline` 除外）。

## 2. 输入输出约定

1. 只用 `cin` / `cout` / `cerr`（第 1~2 周）。
2. 输入是**空白分隔的 token**，行数不重要。不要假设"一行只有一个数"，也不要依赖 `getline` 的行结构（题面要求按行读时会单独说明）。
3. 输出**逐字符比对**：
   - 不要输出任何提示语（`cout << "Please input..."` 一律 WA）；
   - 行末不要多余空格，末尾不要多余空行；
   - 行末用 `\n` 或 `endl`（题面不区分）。
4. 浮点数一律用 `cout` 默认格式输出（**6 位有效数字**），不要用 `fixed` / `setprecision`（题面另有要求除外）。题面样例输出就是标准答案。
5. 诊断信息**只用 `cerr`**（第 3 周 Project 2 要求"非法输入用 `cerr` 提示"）。`cerr` 不参与评测，样例里以"诊断（stderr，评测忽略）"给出。
6. 每题至少 2 组样例，含边界情况；每组按固定四段写：

   - `**【样例 k 输入】**` — 下面接一个 `text` 代码块（三个反引号 + `text`）；
   - `**【样例 k 输出】**（stdout）` — 下面接一个 `text` 代码块；
   - `**【样例 k 诊断】**（stderr，评测忽略）：无输出`，有诊断时则下面接一个 `text` 代码块；
   - `**【说明】**` — 逐条解释这组数据的关键点（边界、易错处、为什么是这个答案）。

## 3. 编码规范与限制（对齐课件）

- **禁用 STL 容器与 `std::string`**：`vector`、`string`、`map`、`array`、`stack`… 均不得使用。全部用**原始数组 + `new`/`delete[]`**（第 3~5 周），需要字符串时用 `char*` + `<cstring>`（第 7 周）；P5 的文本化要求用 `char*` 缓冲区，不许用 `to_string`。
- 禁用课件未讲授的特性：`auto`、范围 `for`、智能指针、`lambda`、`template`、`std::sort`、`std::to_string`、异常（`throw`/`try`，除题面允许）、结构化绑定。
- **函数/类接口签名固定**：题面给出的一律不得改名、不得增删参数、不得改返回类型；题面写"`main` 由题目给出，不得修改"时，粘贴即可，只写类/函数（第 5 周 Project 5、第 6 周 `main.C` 的做法）。
- 大对象用 `const` 引用或指针传递（第 6/7 周：*Prefer pass-by-reference to pass-by-value*），不返回局部变量的引用/指针（第 7 周：*Never return reference to local variable*）。
- `main` 末尾 `return 0`；缩进一致、命名可读（第 3 周 *Why indent code?*）。
- 禁止 `rand()` / `srand()` / `time()`：所有题目都必须**确定性**，同一输入两次运行输出必须完全一致。

## 4. 通用健壮性要求（全题评分依据）

表中每条都来自课件；题目文件只写"本题额外要求"。

| 编号 | 要求 | 课件依据 |
| --- | --- | --- |
| R1 | **多组数据 / 多轮处理**：遇哨兵 `-1` 退出；同一程序可重复运行多次，不需要重启 | 第 2 周作业 *"You code should be able to run multiple times"*；Project 1~4 的 `seed == -1` |
| R2 | **非法/越界数据**：用 `cerr` 提示后用 `continue` 跳过，**不计入统计**，`stdout` 不受影响 | 第 3 周 Project 2 `compScore` 要求 |
| R3 | **边界**：0 条数据、1 条数据、空容器、所有值相同、最小/最大值、全非法输入 | 第 3/5 周 *"甲方保证…"* 之外的情况必须自保 |
| R4 | **动态内存**：`new[]`↔`delete[]` 配对；二维数组**逐行 `delete[]` 后再删外层**；无 leak、无 use-after-free、无 double free | 第 3 周 *Common Errors 1~3*；第 4 周二维 `new/delete` |
| R5 | **类有资源三件套**：析构函数、拷贝构造函数、`operator=`（深拷贝、自赋值安全）；分清"成员对象"与"成员指针"的构造/析构顺序 | 第 5/6 周 *Why Copy Constructor / Deep copy / Object\* in a class* |
| R6 | **越界下标不得访问内存**：`remove/at/operator[]` 越界时"不操作 / 返回默认对象 / 返回 `NULL`" | 第 5 周 Project 5 `release`、`get`、`get_pointer` |
| R7 | 本地用 `valgrind --leak-check=full --error-exitcode=1 ./a.out` 自测，要求 **0 errors、0 leaks**；没有 valgrind 就用 `g++ -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer`（第 6 周 Valgrind 演示） | 第 6 周 *Valgrind reports* |
| R8 | 不改 `main`、不改变输出格式；只动题面要求写的部分 | 课件 *"main 函数（不得修改）"* |
| R9 | 多文件题（P6、P7）：头文件保护宏、`.h` 只放声明、`makefile` 的 `all`/`clean` 目标可用、`make` 之后不产生多余文件 | 第 3 周 *Header Files / Separate Declaration and Definition / makefile* |

## 5. 统一评分（每题 100 分）

| 项目 | 分值 | 说明 |
| --- | --- | --- |
| 功能正确 | 50 | 全部样例与隐藏用例通过 |
| 健壮性 | 30 | 第 4 节 R1~R9 中适用条目全部满足（尤其内存与边界） |
| 规范 | 20 | 接口签名、禁用项、缩进与可读性、`return 0`、多文件题的文件划分 |

## 6. 统一题面模板

每个题目文件**必须**包含以下小节，顺序固定：

1. **题目背景** — 一句话业务场景
2. **考查内容** — 对应课件第几周/哪几条要求；明确"与课件示例的区别"
3. **必做接口** — 签名固定不得修改（含给定 `main` 时贴出完整 `main`）
4. **输入格式**
5. **输出格式**
6. **计算规则与约束**
7. **样例** — ≥2 组，含边界；`stdin` / `stdout` / `stderr` 三段
8. **健壮性要求** — 本题额外要求（引用第 4 节 R 编号）+ 明确"甲方保证 / 甲方不保证"
9. **自测清单** — 可勾选的 checklist，含建议构造的输入
10. **常见错误** — 来自课件的典型坑（越界、`delete` vs `delete[]`、浅拷贝、悬空指针…）
11. **提交与评测** — 构建命令（单文件命令 / `make`）、文件命名、评分构成

## 7. 自测建议（课件"如何调试"）

- **一点一点写、几行就编译**（第 4 周）：不要一次写 200 行再编译。
- **善用 `cout` 打点**：提交前删掉，或放在 `#ifndef ONLINE_JUDGE` 里。
- **自己构造边界数据**：空输入、单元素、全非法、最大规模；对拍前先手算。
- **内存检查必做**：`valgrind --leak-check=full ./a.out < in.txt`，看 `definitely lost: 0 bytes`。
- 每个样例都跑一遍，把 stdout 与题面样例**逐字符**比对（含空格、换行）。

## 8. 与课件的关系：考点对照 + 禁止照抄清单

### 8.1 周次 → 知识点 → 练习

| 课件 | 知识点 | 落在哪题 |
| --- | --- | --- |
| 第 1 周（0825） | 变量与类型、整型/浮点运算、`if/else`、`cin/cout`、编译流程 | P1 |
| 第 2 周（0901） | `switch`、`while` / `do-while` / `for`、`break/continue`、逻辑运算、`cerr` | P1、P2 |
| 第 3 周（0908） | 数组、`new`/`delete[]`、函数与原型、重载、引用参数、作用域、头文件、`makefile` | P2、P3、P6、P7 |
| 第 4 周（0915） | 指针、指针与数组、`int**` 二维动态数组、函数返回数组、运算符优先级 | P3、P6 |
| 第 5 周（0922） | 类与对象、构造/析构、public/private、对象指针、对象数组、拷贝构造与深拷贝 | P4、P6、P7 |
| 第 6 周（1013） | 构造/析构顺序、Valgrind、`operator=` / `+` / `++` / `[]`、`Vector` 扩容、引用传递 | P4、P5、P7 |
| 第 7 周（1020） | `const`、`inline`、返回引用、`char`/`char*`、`strcpy`/`strcmp`/`cctype`、`cin.getline` | P5、P7 |

### 8.2 禁止照抄（课件的示例与作业，练习一律**不得**作为题目）

- **猜数字 / 命中区间计分**（第 2 周作业、Project 1、Project 2 的 `compScore` 规则）→ 不要考"猜得越近分越高"。
- **10 次猜测的平均分 / 标准差**（Project 2、Project 3）→ 不要考"若干轮得分求 `std/k`"。
- **`Monster` / `Team` / `Trainer`**（第 5~7 周 Project 5，国庆 OJ 题）→ 不要考"怪物队伍、容量 +3、释放用末位覆盖"。
- **`Game` / `Round`、`Play` / `Calculate` / `Merge2` / `ReportAve`**（第 5 周 Project 4）→ 不要考"回合合并"。
- **`BigInt` 大整数**（第 7 周 Project 6）→ 不要考"高位在前的大整数比较"。
- **`Vector` 扩容示例**（第 6 周）→ 保留为 P7 的**底座**：P7 在其上加了多文件工程（R9）、`const` 接口、越界返回 `0.0`（R6）、`a += a` 别名安全；不要再单独出"只写扩容"的题。
- **`Complex` 复数类、`XString` 字符串类**（第 6/7 周示例）→ 不要考"复数加减 / 字符串拼接类"。
- **矩阵转置、`ArrayAddConst`**（第 4 周示例）→ 不要考"转置输出 / 全表加常数"。
- **银行账户 `BankAccount`、`Person`**（第 5 周示例）→ 不要考"存款取款 / 自我介绍"。

练习里出现的新场景：**快递计费、体检极值与中位数、扫雷雷数、图书架、有理数运算、质检批次汇总、动态数组扩容**。

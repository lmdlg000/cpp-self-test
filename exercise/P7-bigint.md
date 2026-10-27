# P7 · 大整数类 BigInt（算术运算版）

> 难度：★★★★★　·　对应课件：第 3~7 周综合（多文件工程 + 资源三件套 + 运算符重载）　·　构建：多文件 + makefile（README 第 1.2 节）　·　提交：整个目录（含 makefile），建议命名 `Project7/`
>
> 通用约定（输入/输出格式、禁用项、健壮性 R1~R9、评分）见 [README](README.md)；本文只写本题的额外要求。
> **除"放在 `Project7` 目录、`main` 不同"以外，工程与健壮性要求大致与 [P6](P6-batch-makefile.md) 一致**（第 3 节 makefile 要求、第 8 节 R9 等条目都沿用 P6 的口径）。

## 1. 题目背景

上一版 `BigInt` 只能存、比、印——这一版要**会算**。请实现一个非负大整数类 `BigInt`（位数不受 `int`/`long long` 限制），在原有构造、比较、输出的基础上，支持加法 `+`、`+=`、乘法 `*`、`*=`，并保证全程**无内存泄漏、无 double free、无 use-after-free**（R4/R7）。

## 2. 考查内容

| 考点 | 出处 |
| --- | --- |
| 头文件保护宏、`.h` 只放声明、`.C` 放实现、`makefile` 的 `all`/`clean`（R9） | 第 3 周 *Header Files / Separate Declaration and Definition / makefile*；P6 同款要求 |
| `char*` 存数字串、**高位在前**（MSD first）、手写十进制加法/乘法 | 第 7 周 `char`/`char*` 处理；第 3 周 `new`/`delete[]` |
| 资源三件套：析构、拷贝构造、`operator=`（深拷贝、自赋值安全） | 第 5~6 周 *Why Copy Constructor / Deep copy*；R5 |
| 运算符重载：二元 `+` `*`、复合赋值 `+=` `*=`、`friend operator<<`、六个比较 | 第 6 周 slide 21~29（`operator+`/`++`/`[]`）；P5 的重载经验 |
| `const` 正确性：比较与输出不修改对象 | 第 7 周 *const*；R8 |
| 内存安全：0 errors、0 leaks（`valgrind` / ASan） | 第 6 周 *Valgrind reports*；R4/R7 |

**与课件示例的区别**：课件第 7 周的 `BigInt` 只做"存取与比较"，本题**必须实现算术**（`+` `+=` `*` `*=`）；与 P5 的 `Rational` 也不同——不是分数四则，而是**任意位数的非负整数**，用 `char*` 逐位手算（课件明确禁用 `std::string`/`to_string`，P5 的文本化思路可以直接迁移）。与 P6 相比，类从"两个 + 指针数组"变成"一个自管理资源的类 + 四个算术运算符"，工程骨架（`Project7/` 目录 + `makefile`）完全同款。

## 3. 必做接口

提交一个**目录**（**必须命名 `Project7/`**，与 P6 的差别之一），里面**必须**有这 4 个文件：

| 文件 | 内容 |
| --- | --- |
| `BigInt.h` | `class BigInt` 的**声明** + 头文件保护宏 |
| `BigInt.C` | `class BigInt` 全部成员与友元函数的**实现**，首行 `#include "BigInt.h"` |
| `main.C` | 题目给定，**不得修改** |
| `makefile` | 构建规则，要求与 P6 第 3 节完全一致（见下） |

**`BigInt`（放在 `BigInt.h`，签名不得改动）**：

```cpp
class BigInt {
    bool   isZero;   // true 表示值为 0
    size_t len;      // 存储的位数
    size_t size;     // s 的分配容量（>= len + 1）
    char*  s;        // 数字串：s[0] 为最高位，s[len-1] 为最低位，s[len] = '\0'
    // 可自行增加私有辅助函数（如加法/乘法的核心实现），但不得增删改下列接口
    // 上面 4 个私有数据成员的**名字与类型固定**，不得增删改
public:
    BigInt(size_t v = 0);
    BigInt(const char* str);
    BigInt(const BigInt& o);
    BigInt& operator=(const BigInt& o);
    ~BigInt();

    BigInt operator+(const BigInt& o) const;
    BigInt operator*(const BigInt& o) const;
    BigInt& operator+=(const BigInt& o);
    BigInt& operator*=(const BigInt& o);

    bool operator==(const BigInt& o) const;
    bool operator!=(const BigInt& o) const;
    bool operator< (const BigInt& o) const;
    bool operator<=(const BigInt& o) const;
    bool operator> (const BigInt& o) const;
    bool operator>=(const BigInt& o) const;

    friend std::ostream& operator<<(std::ostream& os, const BigInt& b);
};
```

- 私有数据成员固定为 `bool isZero; size_t len; size_t size; char* s;` 四个，**名字、类型、个数都不得改**；在此之外可以增加私有辅助函数，但 public 接口一个都不能动；
- 四个成员的**语义与不变式**见第 6 节第 2 条（`isZero` 是"是否为 0"的权威标志，比较与输出都要看它）；
- `BigInt(size_t v = 0)` 的**默认参数不能去掉**：`main.C` 里 `BigInt y3(v);` 用 `size_t` 构造，将来隐藏用例还会用到无参构造（`BigInt z;` 即 0）；
- 二元 `+` / `*` **不修改任何一方**，返回新对象；`+=` / `*=` 修改自身并返回 `*this`（供 `(x3 += x4)` 这样链式输出）。

**`main.C`（题目给定，请原样粘贴，不得修改）**：

```cpp
#include <iostream>
#include "BigInt.h"
using namespace std;

int main() {
    BigInt x1("9999999999");
    BigInt x2("1");
    cout << x1 << "+" << x2 << " = " << (x1 + x2) << endl;
    BigInt x3("2100000000");
    BigInt x4("2100000000");
    cout << x3 << "+" << x4 << " = " << (x3 += x4) << endl;
    BigInt y1(99);
    BigInt y2("99");
    size_t v = 0;
    BigInt y3(v);
    cout << y1 << "*" << y2 << " = " << (y1 * y2) << endl;
    cout << y1 << "*" << y3 << " = " << (y1 * y3) << endl;
    BigInt y4("9999999999");
    BigInt y5("9999999999");
    cout << y4 << "*" << y5 << " = " << (y4 *= y5) << endl;
    return 0;
}
```

`main.C` 只 `#include <iostream>` 与 `"BigInt.h"`；它负责"构造 → 算 → 印"，`return 0` 结尾。**任何对 `main.C` 的改动（包括多打一个空格、改 `using namespace`）都按 R8 处理。**

**`makefile`（要求与 P6 第 3 节完全一致，一份可用示例）**：

- 默认目标 `all` 生成可执行文件 **`Project7`**；
- `clean` 删除全部 `.o` 与 `Project7`；
- 变量 `CXX = g++`、`CXXFLAGS = -std=c++20 -O2 -w -Wall`，链接时用 `$(CXX) $(CXXFLAGS)`；
- 每个 `.o` 要写清依赖的 `.h`；**命令前必须是 Tab**（不是空格）；
- 不许写死绝对路径，也不许 `cd` 到别处。

```makefile
CXX = g++
CXXFLAGS = -std=c++20 -O2 -w -Wall
OBJ = main.o BigInt.o
TARGET = Project7

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

main.o: main.C BigInt.h
	$(CXX) $(CXXFLAGS) -c main.C

BigInt.o: BigInt.C BigInt.h
	$(CXX) $(CXXFLAGS) -c BigInt.C

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
```

目标名、变量名你可以自己起，但这些**行为**必须满足（R9，与 P6 相同）：`make` 能生成 `Project7`、`make clean` 能清干净、`make` 之后目录里只多出 `.o` 与 `Project7`。

> Windows 提示：MSYS2 的 `make` + `g++` 本地练习时产物是 `Project7.exe`（`TARGET` 仍写 `Project7`）；`make clean` 里 `rm -f` 由 MSYS2 的 `sh` 执行。评测机是 Linux，产物就是 `Project7`。

## 4. 输入格式

**无输入**。本题不读 `stdin`——`main.C` 里所有数据都是写死的常量（隐藏用例则是助教**替换 `main.C` 里的常量**后编译运行，`BigInt` 的接口签名保证了替换后仍然链接得上）。

## 5. 输出格式

`main.C` 逐行打印五条等式，格式固定为 `左操作数` `运算符` `右操作数` ` = ` `结果`：

```text
9999999999+1 = 10000000000
2100000000+2100000000 = 4200000000
99*99 = 9801
99*0 = 0
9999999999*9999999999 = 99999999980000000001
```

- 运算符左右**没有空格**，` = ` 前后各一个空格（`main.C` 已写好，你只要保证 `operator<<` 印出纯数字）；
- 数字一律**无前导零**（0 就印 `0`）；行末不要多余空格，末尾不要多余空行；
- `stderr` 评测忽略：非法数据诊断只许进 `cerr`（见第 6 节）。

## 6. 计算规则与约束

1. **数值域**：非负整数（0 或正）；位数不限，`9999999999 * 9999999999` 这种 20 位结果必须正确。
2. **存储不变式（全题遵守）**：
   - `bool isZero` 是"值是否为 0"的**权威标志**：`isZero == true` ⇔ 值为 0；
   - **0 的规范表示**：`isZero == true`、`len == 0`、`s[0] == '\0'`（`size >= 1`）；
   - `isZero == false` 时 `len >= 1`，且 `s[0]` 不是 `'0'`（**无前导零**：`"007"` 规范化成 `7`，`"0000"` 规范化成 0）；
   - `s[0]` 是最高位、`s[len-1]` 是最低位（**高位在前**，人类书写顺序）；
   - `s[len] == '\0'`（`s` 可直接当 C 串打印，但**位数以 `len` 为准**，不要信 `strlen`）；
   - `size >= len + 1`；
   - 任何写入 `s`/`len` 的地方都要**同步维护 `isZero`**，绝不允许 `isZero == false` 却 `len == 0`、或 `isZero == true` 却带着数字。
3. **构造**：
   - `BigInt(size_t v)`：十进制逐位填入（先数位数再从后往前填，或临时数组反转——第 3 周 slide *From int to BigInt* 的两种方法都行）；`v == 0` 得到规范 0；
   - `BigInt(const char* str)`：跳过前导 `'0'`；`str` 为 `nullptr` 或全是 `'0'` 或空串 → 规范 0；**遇到非数字字符**：用
     ```cpp
     cerr << "invalid: " << str << endl;
     ```
     诊断后**取前缀数字部分**（全非法则为 0）——`stderr` 评测忽略；
   - `BigInt(const BigInt& o)`：**深拷贝**（`new char[o.size]` 后逐位复制，绝不能让两个对象共用一块缓冲）。
4. **`operator=`**：自赋值安全（`this == &o` 直接返回）；先分配新的、再 `delete[]` 旧的（P6 常见错误 8 的教训）；返回 `*this`。
5. **加法 `+` / `+=`**：逐位竖式，**从低位到高位**算（`s[len-1]` 是最低位），进位最多 1，结果位数最多 `max(len_a, len_b) + 1`；注意最高位进位要保留（`9999999999 + 1 = 10000000000`）。
6. **乘法 `*` / `*=`**：竖式乘法——对 `o` 的每一位（从低到高）乘上 `*this` 的每一位，累加进位到结果数组对应位；结果位数最多 `len_a + len_b`（`99 * 99 = 9801` 是 4 位 = 2+2；`9999999999^2 = 99999999980000000001` 是 20 位 = 10+10）；任何数乘 0 得 0。
7. **`+=` / `*=` 的返回**：修改自身后返回 `*this` 的引用（`main.C` 里 `(x3 += x4)`、`(y4 *= y5)` 要能直接进 `operator<<`）；注意 `a += a`、`a *= a` 这种**别名**情况（自己加/乘自己不能把源数据写坏）。
8. **比较**：六个比较运算符全部 `const`，用一个私有比较辅助函数统一实现（**先看 `isZero`**：两个都是 0 相等，只有一个是 0 则 0 更小；都不是 0 时位数不同先比位数，同位数再从高位到低位逐字符比——因为高位在前，字典序就是数值序）。
9. **`operator<<`**：`friend`、不修改对象；`isZero` 为 `true` 时印 `"0"`，否则按 `s[0..len-1]` 顺序印（高位已在外，**不要反转**）。
10. **禁止**：`std::string`、`to_string`、`vector` 等 STL 容器与字符串设施；`auto`、范围 `for`、`lambda`、`template`、`<bits/stdc++.h>`、`rand`/`srand`/`time`（README 第 3 节）。字符串处理只用 `<cstring>`（`strlen`/`memcpy`/`strcpy`）与手写循环。
11. **文件划分（R9，与 P6 相同）**：`.h` 只放类声明与保护宏（`inline` 成员函数除外），实现全部在 `BigInt.C`；`BigInt.C` 首行 `#include "BigInt.h"`；`main.C` 只 `#include` 头文件。

## 7. 样例

**【样例 1 输入】**

```text
（无输入，main.C 固定）
```

**【样例 1 输出】**（stdout）

```text
9999999999+1 = 10000000000
2100000000+2100000000 = 4200000000
99*99 = 9801
99*0 = 0
9999999999*9999999999 = 99999999980000000001
```

**【样例 1 诊断】**（stderr，评测忽略）：无输出

**【说明】**

固定 `main.C` 的全部输出。五条等式分别覆盖：

- **进位到更高一位**：`9999999999 + 1` 必须得到 11 位的 `10000000000`，结果数组要多留一位给进位；
- **`+=` 并返回 `*this`**：`2100000000 + 2100000000 = 4200000000`（超过 32 位 `int` 上限 2147483647，说明不能用 `int`/`long` 硬加，也不能用 `size_t` 中转——必须按位算）；
- **两位乘两位**：`99 * 99 = 9801`，结果恰好 4 位；
- **乘零**：`99 * 0 = 0`，结果是规范 0（印 `0`，不是空串、不是 `00`）；
- **十位乘十位**：`9999999999 * 9999999999 = 99999999980000000001`（20 位），竖式乘法的每一位进位都要收对。

## 8. 健壮性要求

通用要求见 README 第 4 节；**除下列本题额外/重点条目外，R9 等多文件工程要求与 P6 第 8 节一致**：

- **R1（可重复运行）**：同一程序跑两次输出必须完全一致（本题无输入，天然满足；但不许用 `static` 缓存跨对象状态）。
- **R2（非法数据）**：`BigInt(const char*)` 遇非数字字符按第 6 节第 3 条 `cerr` 诊断并取数字前缀，`stdout` 不受影响。
- **R3（边界）**：0、1、纯前导零串（`"0000"`、`"007"`）、最大 10 位数相乘、`+=`/`*=` 的别名（`a += a`、`a *= a`）、乘 0 与加 0；任何路径都要保持 `isZero` 与 `len`/`s` 一致（见第 6 节第 2 条）。
- **R4（动态内存）**：`new char[]` ↔ `delete[]` 配对；拷贝构造/`operator=` 是**深拷贝**；`+`/`*` 的临时对象析构干净。用 `valgrind --leak-check=full --error-exitcode=1 ./Project7` 或 `g++ -g -O1 -fsanitize=address,undefined main.C BigInt.C -o p7_asan` 自测（R7），要求 **0 errors、0 leaks**。
- **R5（资源三件套）**：析构 + 拷贝构造 + `operator=` 齐全且正确（默认浅拷贝会导致 double free——P6 常见错误 7 的同款坑，本题照样踩）。
- **R8（不改 `main.C`）**：原样粘贴，一个字符都不改；调试输出只能进 `cerr`，提交前删掉。
- **R9（多文件工程）**：与 P6 第 8 节该条完全相同——保护宏、`.h` 只声明、`makefile` 的 `all`/`clean` 可用、`make` 之后只多出 `.o` 与 `Project7`、不写死绝对路径。
- **本题额外要求**：所有比较与 `operator<<` 都是 `const`（或友元）；`operator+=`/`*=` 在别名情形（`a += a`）下结果必须正确且不泄漏。

**甲方保证**：构造用的字符串只含数字（隐藏用例会构造非数字串考察 R2，按第 6 节处理即可）；位数不超过 1000 位；运算结果位数不超过 2000 位。**甲方不保证**：没有前导零；没有 0；`+=`/`*=` 的左右不是同一对象（但你必须自己防住）。

## 9. 自测清单

- [ ] 在 `Project7/` 目录里 `make`：生成 `Project7`，没有 `missing separator`；`make clean` 后四个产物（`main.o`、`BigInt.o`、`Project7`）全消失，再 `make` 仍成功；
- [ ] `./Project7` 输出与第 7 节样例**逐字符**一致（含 ` = ` 的空格、行末换行）；
- [ ] `BigInt("0000")` 印 `0`、`BigInt("007")` 印 `7`（前导零规范化）；
- [ ] `isZero` 一致性：构造 0、加出 0、乘出 0 后 `isZero == true && len == 0`，且 `operator<<` 印 `0`、`cmp` 判等正确；
- [ ] 加法进位：`"9999999999" + "1"` → `10000000000`（结果比两个操作数都长）；
- [ ] 乘法位数：`"99" * "99"` → `9801`；`"9999999999" * "9999999999"` → `99999999980000000001`；
- [ ] 乘 0 / 加 0：任何数 `* BigInt(0)` 得 `0`；`+= 0` 不变；
- [ ] 别名：`a += a`（`a = "123"` → `246`）、`a *= a`（`a = "99"` → `9801`）结果正确；
- [ ] `valgrind --leak-check=full --error-exitcode=1 ./Project7`：`0 errors`、`definitely lost: 0 bytes`；或 ASan 版本跑通无报告；
- [ ] `main.C` 与题目给定版本逐字符比对（`diff main.C 题面版本` 应为空）。

## 10. 常见错误

1. **makefile 用空格缩进**（`missing separator`）、**`clean` 漏删可执行文件**、**写死绝对路径**、**头文件没保护宏**、**`.C` 忘了 `#include "BigInt.h"`** —— P6 常见错误 1~5 原样适用。
2. **浅拷贝**：不写拷贝构造/`operator=`，`BigInt b = a` 后两个对象共用 `s`，析构两次 → double free（P6 常见错误 7）。
3. **`operator=` 先 `delete[] s` 再从 `o` 拷贝**：`a = a` 自赋值时 `o.s` 已是悬空指针 → use-after-free（P6 常见错误 8）。要先判自赋值，或先 `new` 新缓冲再删旧的。
4. **低位当高位**：竖式加法/乘法从 `s[0]`（最高位）开始算，进位方向反了 → `123 + 456` 变成 `579` 的镜像。记住：**索引越大数字越小**。
5. **结果数组少留一位**：`"9999999999" + "1"` 的结果是 11 位，写成 `max(len)` 就丢了进位；乘法结果最多 `len_a + len_b` 位，写成 `max` 就溢出。
6. **乘法忘了错位**：竖式乘法里第 `i` 位乘第 `j` 位要加到结果的第 `i+j` 位上（低位索引方向要想清楚），错位就全盘皆输。
7. **`isZero` 与 `len`/`s` 脱节**：只写 `isZero` 忘了改 `len`（或反过来）→ 比较以为是 0、输出却印出数字（或印出空串）；`operator<<` 只看 `isZero`、`cmp` 只看 `len`，两边口径不一致就出鬼。
8. **0 印成空串或 `00`**：0 必须规范表示（`isZero == true`、`len == 0`），`operator<<` 要特判印 `"0"`；`"0000"` 构造出的对象必须规范化，否则比较和输出都会错。
9. **比较用 `strcmp` 直接比**：位数不同时字典序与数值序不一致（`"9" > "10"` 是字典序），必须先看 `isZero`、再比位数。
10. **`+=`/`*=` 返回 `void` 或返回值**：`main.C` 里 `(x3 += x4)` 要能被 `operator<<` 打印，必须返回 `BigInt&`。
11. **别名写坏源数据**：`a += a` 时如果边加边改 `o` 的缓冲（`o` 就是自己），结果就错；乘法同理，最好用局部数组累加，最后一次性写回。
12. **`new`/`delete` 混用**：`new char[n]` 必须配 `delete[]`（第 3 周 *Common Errors*）。
13. **忘了 `s[len] = '\0'`**：缓冲区不以 0 结尾，`operator<<` 若用 `strlen` 或 `%s` 会印出垃圾。
14. **改动 `main.C`**：哪怕"只是加个空格"也按 R8 算；调试输出进 `cerr` 并记得删。
15. **用 `int`/`long long` 硬加**：`2100000000 + 2100000000` 在 32 位 `int` 下溢出（UB），必须按 `char*` 逐位算。

## 11. 提交与评测

**本题不在单文件 OJ 上提交**，也不用 README 第 1.1 节的 `g++ ... {src_path}` 命令。请提交整个工程目录 **`Project7/`**，里面恰好包含：

```text
Project7/
├── BigInt.h
├── BigInt.C
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
- **评测方式**：隐藏用例是助教**替换 `main.C` 里的常量**（换成别的 `BigInt` 构造与运算组合）后 `make && ./Project7`，逐字符比对输出；因此**接口签名一个都不能改**，改动 = 链接失败 = CE。
- 评分：功能 50（固定样例 + 隐藏 `main` 用例）、健壮性 30（R1~R9，重点 R4/R5/R7 内存安全）、规范 20（接口签名、文件划分、`makefile` 质量、可读性、`return 0`）。
- 自测时先把 `make` 与 `make clean` 各跑一遍，再逐字符比对样例输出，最后务必过一遍 valgrind/ASan。

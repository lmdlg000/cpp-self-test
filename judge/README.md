# judge — 练习 Project 自动评分器（Online-Judge 风格）

针对 `exercise/` 下 P1~P7 七道练习 project 的自动评测工具。按 OJ 的思路设计：
**编译 → 限时运行 → 与期望输出比对 → 判定 → 汇总得分与通过率**，并明确区分
编译失败、运行超时、运行时错误、答案错误等情况。

- 仅依赖 Python 3.8+ 标准库，无需 pip 安装任何包；
- 需要 `g++`（P1~P5）和 `make`（P6、P7）在 PATH 中（MSYS2 即可）；
- Windows / Linux / macOS 通用。

## 1. 快速上手

```bash
# 评测一份 P1 提交（单文件题传 .cpp）
python judge.py run P1 path/to/P1.cpp

# 评测 P6 提交（工程题传整个目录）
python judge.py run P6 path/to/p6_dir

# 只跑名字里带 sample01 的用例；时间限制临时改成 2 秒
python judge.py run P1 P1.cpp --filter sample01 --tl 2000

# 查看某题有哪些用例
python judge.py list P1
```

控制台会输出逐用例结果表 + 汇总（通过数、通过率、得分），同时在
`reports/` 下生成一份 JSON 报告（可用 `--no-report` 关闭，`--report x.json` 指定路径）。
全通过退出码为 0，否则为 1，方便脚本化批改。

## 2. 判定类型

| 判定 | 含义 | 典型原因 | 提示信息里给什么 |
| --- | --- | --- | --- |
| **AC** | 通过 | 输出与期望一致 | — |
| **CE** | 编译失败 | 语法错误、缺头文件；P6 是 `make` 失败 | 编译器/make 输出末尾若干行 |
| **TLE** | 运行超时 | 死循环、算法过慢、卡在等待输入 | 时限与实际耗时 |
| **RE** | 运行时错误 | 段错误、越界、空指针、除零、double free | 崩溃信号/退出码的人话解释 + stderr 末尾 |
| **WA** | 答案错误 | 逻辑算错 | 第一处不一致的行号与期望/实际内容 |
| **PE** | 仅空白差异 | 行末多空格、多余空行（内容其实对） | 提示按不通过计，提醒检查格式 |

比对规则（默认 `exact`）：把 `\r\n` 归一成 `\n` 后**逐字符**比对，容忍"末尾少一个换行"，
但行末空格、多余空行都会判错（题面要求逐字符比对）。另外两种可配：
`token`（空白不敏感）、`float:1e-6`（浮点容差）。

## 3. 目录结构

```
judge/
├── judge.py               # 评测器本体（单文件，纯标准库）
├── README.md
├── problems/              # 每题一个目录：配置 + 测试数据
│   ├── P1/
│   │   ├── problem.json   # 构建命令、时限、总分、比对方式
│   │   └── tests/
│   │       ├── sample01.in    # 输入（stdin）
│   │       ├── sample01.out   # 期望输出（stdout）
│   │       ├── sample01.json  # 可选：用例元数据（权重/单独时限/备注）
│   │       └── …
│   ├── P2/ … P7/
├── reference/             # 参考实现（P1 单文件；P6/P7 多文件工程；P7_hidden 为隐藏 main.C 变体）
├── examples/              # 演示提交：AC / WA / CE / TLE / RE / PE / 多文件 make
└── reports/               # 评测 JSON 报告（自动创建）
```

## 4. 新增测试用例（三种方式）

**方式一：直接放文件（最常用）**

把 `<名字>.in`（程序输入）和 `<名字>.out`（期望输出）成对放进 `problems/Px/tests/`，
评测时自动发现，按文件名排序。命名随意，建议 `sample01`、`hidden01`、`edge03` 这样分类。

**方式二：`gen` 用参考实现批量生成期望输出**

```bash
# 先写好 hidden02.in，再用参考实现生成 hidden02.out
python judge.py gen P1 --ref reference/P1.cpp --inputs problems/P1/tests/hidden02.in

# 不指定 --inputs 则为 tests/ 下所有 .in 生成/刷新 .out
python judge.py gen P2 --ref path/to/P2_reference.cpp
```

参考实现可以是单文件 `.cpp`，也可以是带 makefile 的目录（配 `--build make --target xxx`）。
`gen` 会跳过 TLE/RE 的输入，不会写入错误结果。

**方式三：`import-samples` 从题面 markdown 提取样例**

题面统一用 `**【样例 k 输入】**` + `text` 代码块的格式，可直接抽取：

```bash
python judge.py import-samples ../exercise/P3-minesweeper.md
python judge.py import-samples ../exercise/P3-minesweeper.md --force   # 覆盖已有用例
```

默认按文件名 `P?-xxx.md` 推断题号，也可 `--problem P3` 指定。已内置全部 7 题的样例与隐藏用例（共 33 组，其中 P7 的 `hidden01` 由 `reference/P7_hidden` 生成）。

**用例元数据（可选）**：同名 `.json` 文件

```json
{
  "name": "hidden01 边界：向上取整",
  "weight": 2,            // 权重，计入得分折算，默认 1
  "tl_ms": 2000,          // 单独的时限，覆盖题目默认值
  "check_stderr": true,   // 是否连 stderr 一起比对（默认忽略 stderr）
  "err_file": "hidden01.err",
  "note": "extra=2→14 与 extra=3→17"
}
```

## 5. problem.json 配置项

| 字段 | 默认值 | 说明 |
| --- | --- | --- |
| `title` | — | 题目名称，报告里显示 |
| `build` | `single` | `single`（单文件编译）或 `make`（工程构建） |
| `source_name` | `main.cpp` | single 模式期望的源文件名（P1 即 `P1.cpp`） |
| `compile_cmd` | README 1.1 的 OJ 命令 | 编译命令，占位符 `{src}` 源文件、`{exe}` 产物、`{dir}` 构建目录 |
| `make_cmd` | `["make"]` | make 模式的构建命令 |
| `target` | — | make 模式产物名（P6 为 `inspector`，Windows 自动找 `inspector.exe`） |
| `time_limit_ms` | `1000` | 每个用例的时限 |
| `memory_limit_mb` | `256` | 内存上限（仅 Linux/macOS 生效；Windows 不强制） |
| `total_score` | `100` | 总分 |
| `compare` | `exact` | `exact` / `token` / `float:1e-6` |
| `strict_newline` | `false` | `true` 时连末尾换行也严格比对 |

命令行 `--tl / --build / --target / --source-name / --compare / --score` 可临时覆盖上述配置。

## 6. 命令一览

```bash
python judge.py run <题号> <提交> [选项]     # 评测
python judge.py gen <题号> --ref <参考实现>  # 生成期望输出
python judge.py import-samples <题面.md>    # 从题面提取样例
python judge.py list <题号>                 # 列出用例
```

`run` 常用选项：

| 选项 | 作用 |
| --- | --- |
| `--tests DIR` | 指定测试目录（默认 `<problem>/tests`，方便放隐藏用例） |
| `--filter STR` | 只跑名字含该子串的用例 |
| `--tl MS` / `--score N` / `--compare MODE` | 覆盖时限 / 总分 / 比对方式 |
| `--build single\|make` / `--target NAME` | 覆盖构建方式（如用 make 工程测 P1） |
| `--report PATH` / `--no-report` | 控制 JSON 报告 |
| `--keep-build` | 保留构建目录，便于复现问题 |
| `-v` | 失败用例展示完整信息 |
| `--quiet` | 不逐条打印进度 |

## 7. 批量批改（助教向）

```bash
# 评一个学生的全部题目
for p in P1 P2 P3 P4 P5; do
  python judge.py run $p "submissions/张三/$p.cpp" --report "reports/张三_$p.json"
done
python judge.py run P6 "submissions/张三/P6" --report "reports/张三_P6.json"
python judge.py run P7 "submissions/张三/Project7" --report "reports/张三_P7.json"

# 只筛出未满分的提交
python judge.py run P1 submissions/*/P1.cpp   # 逐个跑；退出码 1 即未全过
```

## 8. 得分口径

- **得分** = `total_score × 通过用例权重和 / 全部用例权重和`（默认权重都是 1，
  即按通过率折算 100 分）。
- **通过率** = AC 用例数 / 用例总数。
- PE 按不通过计；CE 整题 0 分。
- 评测器覆盖的是评分表里**功能正确（50 分）**的客观部分；健壮性（30 分，R1~R9）
  与规范（20 分）仍需人工按 README 第 5 节核对（如 valgrind、接口签名、代码风格）。

## 9. 排查指引（给学生看的）

- **CE**：看报告里"构建输出"的报错行号；先在本地用同样命令编译一遍。
- **TLE**：检查是否有 `while (cin >> x)` 忘了 `-1` 哨兵退出、是否死循环、
  大数据是否用了 O(n²) 以上算法。
- **RE**：报告会写明是段错误、除零还是 abort；九成是数组越界、空指针、
  `delete` 与 `delete[]` 混用、栈上开大数组。用
  `g++ -g -O1 -fsanitize=address,undefined ...` 或 valgrind 自查。
- **WA**：报告指出第一处不一致的行；先手算该组样例，检查边界（0 件、空容器、
  封顶、越界下标）。
- **PE**：内容是对的，但行末多了空格或末尾多了空行——逐字符对齐题面样例。

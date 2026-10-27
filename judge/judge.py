#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
judge.py — FCAA101-02 练习 Project 自动评分器（Online-Judge 风格）

核心能力：
  1. 编译 / 构建学生提交（单文件 g++ 命令，或多文件 make）；
  2. 逐用例限时运行，把程序 stdout 与期望输出比对；
  3. 明确区分判定：CE 编译失败 / TLE 运行超时 / RE 运行时错误 /
     WA 答案错误 / PE 仅空白差异 / AC 通过；
  4. 记录每条用例的判定、耗时、退出码、提示信息，汇总得分与通过率；
  5. 便捷新增用例：直接丢 tests/xx.in + xx.out，或用 gen 从参考实现
     批量生成期望输出，或用 import-samples 从题面 markdown 提取样例。

仅使用 Python 标准库，Python 3.8+ 可运行。
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# --------------------------------------------------------------------------
# 基本常量
# --------------------------------------------------------------------------

JUDGE_ROOT = Path(__file__).resolve().parent
PROBLEMS_ROOT = JUDGE_ROOT / "problems"
REPORTS_DIR = JUDGE_ROOT / "reports"

VERDICT_AC = "AC"
VERDICT_WA = "WA"
VERDICT_PE = "PE"      # 内容一致仅空白/换行差异（按 WA 计，但单独提示）
VERDICT_TLE = "TLE"
VERDICT_RE = "RE"
VERDICT_CE = "CE"
VERDICT_SKIP = "SKIP"

VERDICT_COLOR = {
    VERDICT_AC: "\033[32m",     # 绿
    VERDICT_WA: "\033[31m",     # 红
    VERDICT_PE: "\033[33m",     # 黄
    VERDICT_TLE: "\033[33m",    # 黄
    VERDICT_RE: "\033[35m",     # 紫
    VERDICT_CE: "\033[31m",     # 红
    VERDICT_SKIP: "\033[90m",   # 灰
}
RESET = "\033[0m"
BOLD = "\033[1m"

# 比对模式
COMPARE_EXACT = "exact"        # 逐字符（默认，题目要求）
COMPARE_TOKEN = "token"        # 空白不敏感
COMPARE_FLOAT_PREFIX = "float"  # float:1e-6 形式，数值容差

DEFAULT_CONFIG: Dict = {
    "title": "",
    "build": "single",                 # single | make
    "source_name": "main.cpp",         # single 模式期望的源文件名
    "compile_cmd": [
        "g++", "-DONLINE_JUDGE", "-O2", "-w", "-fmax-errors=3",
        "-std=c++20", "{src}", "-lm", "-o", "{exe}",
    ],
    "make_cmd": ["make"],
    "target": "",                      # make 模式产物名（如 inspector）
    "time_limit_ms": 1000,
    "memory_limit_mb": 256,            # 仅 Linux/macOS 生效（Windows 不强制）
    "build_timeout_ms": 60000,
    "total_score": 100,
    "compare": COMPARE_EXACT,
    "strict_newline": False,           # True = 连末尾换行也严格比对
}


# --------------------------------------------------------------------------
# 颜色输出
# --------------------------------------------------------------------------

def _use_color() -> bool:
    if os.environ.get("NO_COLOR"):
        return False
    return sys.stdout.isatty()


def paint(text: str, color: str) -> str:
    if not _use_color():
        return text
    return f"{color}{text}{RESET}"


def paint_verdict(verdict: str) -> str:
    return paint(f"{verdict:>4}", VERDICT_COLOR.get(verdict, ""))


# --------------------------------------------------------------------------
# 数据结构
# --------------------------------------------------------------------------

@dataclass
class TestCase:
    name: str
    in_path: Path
    out_path: Path
    weight: float = 1.0
    tl_ms: Optional[int] = None      # 覆盖题目默认时限
    check_stderr: bool = False       # 是否连 stderr 一起比对（默认忽略）
    err_path: Optional[Path] = None
    note: str = ""


@dataclass
class CaseResult:
    name: str
    verdict: str
    time_ms: float = 0.0
    exit_code: Optional[int] = None
    message: str = ""
    weight: float = 1.0
    stdout_len: int = 0
    stderr_len: int = 0
    stdout_raw: bytes = b""   # 仅 gen 复用，不进 JSON 报告


@dataclass
class BuildResult:
    ok: bool
    exe: Optional[Path]
    time_ms: float = 0.0
    log: str = ""
    message: str = ""


# --------------------------------------------------------------------------
# 配置与用例发现
# --------------------------------------------------------------------------

def load_problem_config(problem_dir: Path, overrides: Optional[Dict] = None) -> Dict:
    """读取 problem.json 并与默认值 / 命令行覆盖项合并。"""
    cfg = dict(DEFAULT_CONFIG)
    cfg_path = problem_dir / "problem.json"
    if cfg_path.exists():
        with open(cfg_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        cfg.update(data)
    else:
        cfg["title"] = problem_dir.name
    if overrides:
        for k, v in overrides.items():
            if v is not None:
                cfg[k] = v
    cfg["problem_dir"] = problem_dir
    return cfg


def resolve_problem(arg: str, problems_root: Path = PROBLEMS_ROOT) -> Path:
    """把 'P1' / 'P1-shipping' / 目录路径 解析为 problem 目录。"""
    p = Path(arg)
    if p.is_dir():
        return p.resolve()
    for cand in (problems_root / arg, problems_root / arg.upper()):
        if cand.is_dir():
            return cand.resolve()
    # 允许按前缀模糊匹配 P1 -> P1-shipping
    if problems_root.is_dir():
        for d in sorted(problems_root.iterdir()):
            if d.is_dir() and d.name.split("-")[0].upper() == arg.upper():
                return d.resolve()
    raise FileNotFoundError(f"找不到题目：{arg}（期望 {problems_root}/{arg}）")


def discover_tests(problem_dir: Path, tests_dir: Optional[Path] = None,
                   name_filter: Optional[str] = None) -> List[TestCase]:
    """扫描 tests/ 目录，配对 xx.in / xx.out，读取可选的 xx.json 元数据。"""
    tdir = tests_dir or (problem_dir / "tests")
    if not tdir.is_dir():
        raise FileNotFoundError(f"找不到测试目录：{tdir}")
    cases: List[TestCase] = []
    for in_path in sorted(tdir.glob("*.in")):
        name = in_path.stem
        if name_filter and name_filter not in name:
            continue
        out_path = in_path.with_suffix(".out")
        if not out_path.exists():
            print(f"[警告] {in_path.name} 没有对应的 .out，已跳过", file=sys.stderr)
            continue
        tc = TestCase(name=name, in_path=in_path, out_path=out_path)
        meta_path = in_path.with_suffix(".json")
        if meta_path.exists():
            with open(meta_path, "r", encoding="utf-8") as f:
                meta = json.load(f)
            tc.weight = float(meta.get("weight", 1.0))
            tc.tl_ms = meta.get("tl_ms")
            tc.check_stderr = bool(meta.get("check_stderr", False))
            tc.note = str(meta.get("note", ""))
            err = meta.get("err_file")
            if err:
                tc.err_path = tdir / err
        err_guess = in_path.with_suffix(".err")
        if tc.check_stderr and tc.err_path is None and err_guess.exists():
            tc.err_path = err_guess
        cases.append(tc)
    return cases


# --------------------------------------------------------------------------
# 构建（编译 / make）
# --------------------------------------------------------------------------

def _run_build_cmd(cmd: List[str], cwd: Path, timeout_ms: int) -> Tuple[int, str]:
    """执行构建命令，返回 (退出码, 合并输出)。"""
    try:
        proc = subprocess.run(
            cmd, cwd=str(cwd),
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=timeout_ms / 1000.0,
        )
        out = proc.stdout.decode("utf-8", errors="replace")
        return proc.returncode, out
    except FileNotFoundError:
        return 127, f"找不到命令：{cmd[0]}（请确认 g++ / make 已在 PATH 中）"
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or b"").decode("utf-8", errors="replace")
        return 124, out + f"\n[构建超时 > {timeout_ms} ms]"


def _copy_submission(submission: Path, workdir: Path, source_name: str) -> Path:
    """把提交内容复制进构建目录，返回源文件/目录位置。"""
    if submission.is_file():
        dst = workdir / source_name
        shutil.copy2(submission, dst)
        return dst
    if submission.is_dir():
        # 整目录复制（P6 工程题）；顺带允许单文件题传目录（自动找 source_name）
        for item in submission.iterdir():
            dst = workdir / item.name
            if item.is_dir():
                shutil.copytree(item, dst)
            else:
                shutil.copy2(item, dst)
        return workdir
    raise FileNotFoundError(f"提交不存在：{submission}")


def build_submission(submission: Path, cfg: Dict, workdir: Path) -> BuildResult:
    """在 workdir 中构建提交，返回可执行文件路径或失败日志。"""
    t0 = time.perf_counter()
    _copy_submission(submission, workdir, cfg["source_name"])
    timeout_ms = int(cfg["build_timeout_ms"])

    if cfg["build"] == "make":
        src_dir = workdir
        rc, log = _run_build_cmd(list(cfg["make_cmd"]), src_dir, timeout_ms)
        if rc != 0:
            return BuildResult(False, None, (time.perf_counter() - t0) * 1000,
                               log, "make 构建失败（等价于编译错误 CE）")
        exe = _find_target(workdir, cfg["target"])
        if exe is None:
            return BuildResult(False, None, (time.perf_counter() - t0) * 1000,
                               log, f"make 成功但找不到可执行文件 {cfg['target']}")
        return BuildResult(True, exe, (time.perf_counter() - t0) * 1000, log)

    # single 模式：固定 OJ 编译命令
    src = workdir / cfg["source_name"]
    if not src.exists():
        return BuildResult(False, None, 0.0, "",
                           f"提交目录里找不到 {cfg['source_name']}")
    exe_name = "program.exe" if os.name == "nt" else "program"
    exe = workdir / exe_name
    cmd = [str(x).format(src=cfg["source_name"], exe=exe_name, dir=str(workdir))
           for x in cfg["compile_cmd"]]
    rc, log = _run_build_cmd(cmd, workdir, timeout_ms)
    if rc != 0 or not exe.exists():
        return BuildResult(False, None, (time.perf_counter() - t0) * 1000,
                           log, "编译失败（CE）")
    return BuildResult(True, exe, (time.perf_counter() - t0) * 1000, log)


def _find_target(workdir: Path, target: str) -> Optional[Path]:
    if not target:
        return None
    for cand in (workdir / target, workdir / (target + ".exe")):
        if cand.exists():
            return cand
    return None


# --------------------------------------------------------------------------
# 运行单个用例
# --------------------------------------------------------------------------

def _posix_limit(mem_mb: int):
    """返回 preexec_fn：在 POSIX 上限制地址空间，帮助揪出内存失控。"""
    if os.name != "posix" or mem_mb <= 0:
        return None

    def _fn():
        try:
            import resource
            limit = mem_mb * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (limit, limit))
        except Exception:
            pass
    return _fn


def _decode_exit(code: Optional[int]) -> str:
    """把退出码翻译成人话（覆盖常见崩溃信号）。"""
    if code is None:
        return ""
    if code == 0:
        return ""
    # Windows NTSTATUS
    if code == 3221225477:
        return "访问违例（段错误）：数组越界 / 空指针 / 栈溢出"
    if code == 3221225495:
        return "栈缓冲区溢出（__stack_chk_fail）"
    if code == 3221225725:
        return "被系统终止（可能是内存耗尽）"
    # POSIX 信号（subprocess 返回负值）
    sig_map = {
        -4: "SIGILL 非法指令",
        -6: "SIGABRT 中止：可能 double free / assert 失败",
        -8: "SIGFPE 除零",
        -9: "SIGKILL 被杀死（可能超时或内存耗尽）",
        -11: "SIGSEGV 段错误：数组越界 / 空指针 / 栈溢出",
    }
    if code in sig_map:
        return sig_map[code]
    return f"非零退出码 {code}"


def run_case(exe: Path, case: TestCase, cfg: Dict, workdir: Path) -> CaseResult:
    tl_ms = int(case.tl_ms or cfg["time_limit_ms"])
    mem_mb = int(cfg.get("memory_limit_mb", 0))
    result = CaseResult(name=case.name, verdict=VERDICT_RE, weight=case.weight)

    t0 = time.perf_counter()
    try:
        with open(case.in_path, "rb") as fin:
            proc = subprocess.Popen(
                [str(exe)],
                stdin=fin,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=str(workdir),
                preexec_fn=_posix_limit(mem_mb),
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0,
            )
            try:
                out, err = proc.communicate(timeout=tl_ms / 1000.0)
                result.exit_code = proc.returncode
            except subprocess.TimeoutExpired:
                proc.kill()
                out, err = proc.communicate()
                result.time_ms = (time.perf_counter() - t0) * 1000
                result.verdict = VERDICT_TLE
                result.message = (f"超过时限 {tl_ms} ms（可能死循环 / 算法过慢 / "
                                  f"仍在等待输入）")
                result.stdout_len = len(out)
                result.stderr_len = len(err)
                return result
    except OSError as e:
        result.time_ms = (time.perf_counter() - t0) * 1000
        result.verdict = VERDICT_RE
        result.message = f"无法启动程序：{e}"
        return result

    result.time_ms = (time.perf_counter() - t0) * 1000
    result.stdout_len = len(out)
    result.stderr_len = len(err)
    result.stdout_raw = out

    # 1) 运行时错误：非零退出 / 崩溃信号
    if result.exit_code != 0:
        result.verdict = VERDICT_RE
        hint = _decode_exit(result.exit_code)
        tail = err.decode("utf-8", errors="replace").strip()[-400:]
        result.message = hint or "程序异常退出"
        if tail:
            result.message += f"；stderr 末尾：{tail!r}"
        return result

    # 2) 答案比对（没有期望输出时跳过，供 gen 使用）
    if not case.out_path.exists():
        result.verdict = VERDICT_SKIP
        result.message = "无期望输出，跳过比对"
        return result

    expected = case.out_path.read_bytes()
    mode = cfg["compare"]
    verdict, message = compare_output(expected, out, mode, cfg["strict_newline"])
    result.verdict = verdict
    result.message = message

    # 3) 可选：stderr 也比对（题目默认忽略 stderr）
    if case.check_stderr and case.err_path is not None and verdict == VERDICT_AC:
        exp_err = case.err_path.read_bytes()
        v2, m2 = compare_output(exp_err, err, COMPARE_TOKEN, False)
        if v2 != VERDICT_AC:
            result.verdict = VERDICT_WA
            result.message = "stderr 不符：" + m2
    return result


# --------------------------------------------------------------------------
# 输出比对
# --------------------------------------------------------------------------

def _norm_newlines(b: bytes) -> bytes:
    return b.replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def _strip_one_newline(b: bytes) -> bytes:
    return b[:-1] if b.endswith(b"\n") else b


def _tokenize(b: bytes) -> List[str]:
    return b.split()


def _is_number(tok: str) -> bool:
    try:
        float(tok)
        return True
    except ValueError:
        return False


def _diff_message(expected: bytes, actual: bytes) -> str:
    """给出第一条差异行的人话提示。"""
    exp_lines = expected.split(b"\n")
    act_lines = actual.split(b"\n")
    n = min(len(exp_lines), len(act_lines))
    for i in range(n):
        if exp_lines[i] != act_lines[i]:
            e = exp_lines[i].decode("utf-8", errors="replace")
            a = act_lines[i].decode("utf-8", errors="replace")
            e_show = e if len(e) <= 100 else e[:100] + "…"
            a_show = a if len(a) <= 100 else a[:100] + "…"
            msg = f"第 {i + 1} 行不符：期望 {e_show!r}，实际 {a_show!r}"
            if len(e) > 100 or len(a) > 100:
                msg += "（长行已截断，可用 -v 查看完整 diff）"
            return msg
    if len(exp_lines) != len(act_lines):
        more = len(act_lines) - len(exp_lines)
        if more > 0:
            return f"输出比期望多 {more} 行（行数 {len(act_lines)} vs {len(exp_lines)}）"
        return f"输出比期望少 {-more} 行（行数 {len(act_lines)} vs {len(exp_lines)}）"
    return "输出与期望存在不可见字符差异"


def compare_output(expected: bytes, actual: bytes, mode: str,
                   strict_newline: bool) -> Tuple[str, str]:
    """返回 (判定, 提示信息)。"""
    exp = _norm_newlines(expected)
    act = _norm_newlines(actual)

    if mode == COMPARE_TOKEN or mode.startswith(COMPARE_FLOAT_PREFIX):
        exp_toks, act_toks = _tokenize(exp), _tokenize(act)
        tol = 0.0
        if mode.startswith(COMPARE_FLOAT_PREFIX):
            try:
                tol = float(mode.split(":", 1)[1])
            except (IndexError, ValueError):
                tol = 1e-6
        if len(exp_toks) != len(act_toks):
            return (VERDICT_WA,
                    f"token 数量不符：期望 {len(exp_toks)} 个，实际 {len(act_toks)} 个")
        for i, (e, a) in enumerate(zip(exp_toks, act_toks)):
            if e == a:
                continue
            if tol > 0 and _is_number(e) and _is_number(a):
                try:
                    if abs(float(e) - float(a)) <= tol * max(1.0, abs(float(e))):
                        continue
                except ValueError:
                    pass
            return (VERDICT_WA,
                    f"第 {i + 1} 个 token 不符：期望 {e!r}，实际 {a!r}")
        return (VERDICT_AC, "")

    # exact：逐字符
    if not strict_newline:
        exp_c, act_c = _strip_one_newline(exp), _strip_one_newline(act)
    else:
        exp_c, act_c = exp, act

    if exp_c == act_c:
        return (VERDICT_AC, "")

    # 仅空白差异？给 PE 提示（仍按不通过计）
    if b"".join(exp_c.split()) == b"".join(act_c.split()) or _tokenize(exp_c) == _tokenize(act_c):
        return (VERDICT_PE,
                "内容相同但空白/换行/行末空格不同（Presentation Error，按不通过计）")

    return (VERDICT_WA, _diff_message(exp_c, act_c))


# --------------------------------------------------------------------------
# 评分汇总
# --------------------------------------------------------------------------

def summarize(results: List[CaseResult], total_score: float) -> Dict:
    total_weight = sum(r.weight for r in results) or 1.0
    passed_weight = sum(r.weight for r in results if r.verdict == VERDICT_AC)
    passed = sum(1 for r in results if r.verdict == VERDICT_AC)
    verdicts: Dict[str, int] = {}
    for r in results:
        verdicts[r.verdict] = verdicts.get(r.verdict, 0) + 1
    score = round(total_score * passed_weight / total_weight, 2)
    pass_rate = round(passed / len(results), 4) if results else 0.0
    return {
        "total": len(results),
        "passed": passed,
        "pass_rate": pass_rate,
        "score": score,
        "total_score": total_score,
        "verdicts": verdicts,
    }


# --------------------------------------------------------------------------
# 报告渲染
# --------------------------------------------------------------------------

def render_report(cfg: Dict, submission: Path, build: BuildResult,
                  results: List[CaseResult], summary: Dict, verbose: bool) -> str:
    line = "─" * 62
    bar = "═" * 62
    out: List[str] = [bar]
    title = cfg.get("title") or cfg["problem_dir"].name
    out.append(f" {cfg['problem_dir'].name} · {title} — 评测报告")
    out.append(f" 提交: {submission}")
    if build.ok:
        out.append(f" 构建: 成功（{build.time_ms:.0f} ms）")
    else:
        out.append(f" 构建: 失败 — {build.message}")
    out.append(line)
    out.append(f" {'#':>2}  {'用例':<16}{'结果':>5}  {'耗时':>9}  {'退出码':>6}  说明")
    out.append(line)

    for i, r in enumerate(results, 1):
        time_s = f"{r.time_ms:.1f} ms"
        code_s = "" if r.exit_code is None else str(r.exit_code)
        msg = r.message
        if not verbose and len(msg) > 60:
            msg = msg[:60] + "…"
        out.append(f" {i:>2}  {r.name:<16}{paint_verdict(r.verdict)}  {time_s:>9}  {code_s:>6}  {msg}")

    out.append(line)
    verdict_str = "  ".join(f"{k}={v}" for k, v in sorted(summary["verdicts"].items()))
    rate_pct = summary["pass_rate"] * 100
    out.append(f" 通过 {summary['passed']}/{summary['total']} · "
               f"通过率 {rate_pct:.1f}% · "
               f"得分 {summary['score']}/{summary['total_score']}")
    if verdict_str:
        out.append(f" 判定分布: {verdict_str}")

    # 非 AC 用例的详细提示（verbose 时展开全部）
    failures = [r for r in results if r.verdict != VERDICT_AC]
    if failures:
        out.append(line)
        out.append(" 未通过用例详情：")
        show = failures if verbose else failures[:5]
        for r in show:
            out.append(f"  · [{r.verdict}] {r.name}: {r.message}")
            if verbose:
                out.append(f"    输出字节数 {r.stdout_len}，stderr 字节数 {r.stderr_len}")
        if not verbose and len(failures) > 5:
            out.append(f"  … 其余 {len(failures) - 5} 条用 -v 查看")

    if not build.ok and build.log:
        out.append(line)
        out.append(" 构建输出（末尾 15 行，完整内容见 JSON 报告）：")
        log_lines = build.log.rstrip().splitlines()
        for ln in log_lines[-15:]:
            out.append("   | " + ln)

    out.append(bar)
    return "\n".join(out)


def write_json_report(path: Path, cfg: Dict, submission: Path,
                      build: BuildResult, results: List[CaseResult],
                      summary: Dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    data = {
        "problem": cfg["problem_dir"].name,
        "title": cfg.get("title", ""),
        "submission": str(submission),
        "config": {k: str(v) if isinstance(v, Path) else v
                   for k, v in cfg.items() if k != "problem_dir"},
        "build": {
            "ok": build.ok,
            "time_ms": round(build.time_ms, 2),
            "message": build.message,
            "log": build.log,
        },
        "cases": [
            {
                "name": r.name,
                "verdict": r.verdict,
                "time_ms": round(r.time_ms, 2),
                "exit_code": r.exit_code,
                "message": r.message,
                "weight": r.weight,
                "stdout_bytes": r.stdout_len,
                "stderr_bytes": r.stderr_len,
            }
            for r in results
        ],
        "summary": summary,
        "generated_at": time.strftime("%Y-%m-%d %H:%M:%S"),
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


# --------------------------------------------------------------------------
# 命令：run
# --------------------------------------------------------------------------

def cmd_run(args: argparse.Namespace) -> int:
    problem_dir = resolve_problem(args.problem)
    submission = Path(args.submission).resolve()
    overrides = {
        "build": args.build,
        "target": args.target,
        "source_name": args.source_name,
        "time_limit_ms": args.tl,
        "compare": args.compare,
        "total_score": args.score,
    }
    cfg = load_problem_config(problem_dir, overrides)
    cases = discover_tests(problem_dir,
                           tests_dir=Path(args.tests).resolve() if args.tests else None,
                           name_filter=args.filter)
    if not cases:
        print("没有可运行的测试用例", file=sys.stderr)
        return 2

    keep = args.keep_build
    workdir = Path(tempfile.mkdtemp(prefix="judge_build_"))
    report_path: Optional[Path] = None
    try:
        build = build_submission(submission, cfg, workdir)
        results: List[CaseResult] = []
        if build.ok:
            for case in cases:
                r = run_case(build.exe, case, cfg, workdir)
                results.append(r)
                if not args.quiet:
                    print(f"  {case.name:<16} {paint_verdict(r.verdict)}  {r.time_ms:7.1f} ms")
        summary = summarize(results, cfg["total_score"]) if results else {
            "total": 0, "passed": 0, "pass_rate": 0.0,
            "score": 0.0 if not build.ok else cfg["total_score"],
            "total_score": cfg["total_score"],
            "verdicts": {VERDICT_CE: 0} if not build.ok else {},
        }
        if not build.ok:
            summary["score"] = 0.0
            summary["verdicts"] = {VERDICT_CE: 1}

        print()
        print(render_report(cfg, submission, build, results, summary, args.verbose))

        if not args.no_report:
            stamp = time.strftime("%Y%m%d-%H%M%S")
            report_path = Path(args.report) if args.report else (
                REPORTS_DIR / f"{problem_dir.name}_{stamp}.json")
            write_json_report(report_path, cfg, submission, build, results, summary)
            print(f"\n JSON 报告: {report_path}")

        if keep:
            print(f" 构建目录已保留: {workdir}")
        # 退出码：全 AC 为 0，否则 1（方便脚本化批改）
        return 0 if (build.ok and summary["passed"] == summary["total"]) else 1
    finally:
        if not keep:
            shutil.rmtree(workdir, ignore_errors=True)


# --------------------------------------------------------------------------
# 命令：gen（用参考实现生成期望输出）
# --------------------------------------------------------------------------

def cmd_gen(args: argparse.Namespace) -> int:
    problem_dir = resolve_problem(args.problem)
    ref = Path(args.ref).resolve()
    if not ref.exists():
        print(f"参考实现不存在：{ref}", file=sys.stderr)
        return 2
    cfg = load_problem_config(problem_dir, {"build": args.build, "target": args.target})
    tests_dir = Path(args.tests).resolve() if args.tests else (problem_dir / "tests")
    tests_dir.mkdir(parents=True, exist_ok=True)

    # 收集要生成的输入
    inputs = [Path(x).resolve() for x in args.inputs] if args.inputs else []
    if not inputs:
        inputs = sorted(tests_dir.glob("*.in"))
    if not inputs:
        print(f"{tests_dir} 下没有 .in 文件", file=sys.stderr)
        return 2

    workdir = Path(tempfile.mkdtemp(prefix="judge_gen_"))
    try:
        build = build_submission(ref, cfg, workdir)
        if not build.ok:
            print(f"参考实现构建失败：{build.message}\n{build.log}", file=sys.stderr)
            return 1
        ok = 0
        for in_path in inputs:
            out_path = tests_dir / (in_path.stem + ".out")
            case = TestCase(name=in_path.stem, in_path=in_path, out_path=out_path)
            r = run_case(build.exe, case, cfg, workdir)
            if r.verdict in (VERDICT_TLE, VERDICT_RE):
                print(f"  [跳过] {in_path.name}: {r.verdict} {r.message}", file=sys.stderr)
                continue
            out_path.write_bytes(_norm_newlines(r.stdout_raw))
            print(f"  [生成] {out_path.name}  ({in_path.name})")
            ok += 1
        print(f"\n 完成：{ok}/{len(inputs)} 个期望输出已写入 {tests_dir}")
        return 0
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


# --------------------------------------------------------------------------
# 命令：import-samples（从题面 markdown 提取样例）
# --------------------------------------------------------------------------

RE_SAMPLE_IN = re.compile(r"\*\*【样例\s*(\d+)\s*输入】\*\*")
RE_SAMPLE_OUT = re.compile(r"\*\*【样例\s*(\d+)\s*输出】\*\*")
RE_FENCE = re.compile(r"^```(\w*)\s*$")


def _extract_fence_blocks(lines: List[str], start: int) -> Tuple[Optional[str], int]:
    """从 lines[start:] 找下一个 ``` 围栏代码块，返回 (内容, 结束行号)。"""
    i = start
    n = len(lines)
    while i < n and lines[i].strip() == "":
        i += 1
    if i >= n:
        return None, start
    m = RE_FENCE.match(lines[i].rstrip("\n"))
    if not m:
        return None, start
    body: List[str] = []
    i += 1
    while i < n:
        if RE_FENCE.match(lines[i].rstrip("\n")):
            return "\n".join(body), i + 1
        body.append(lines[i].rstrip("\n"))
        i += 1
    return "\n".join(body), i


def import_samples(md_path: Path, problem_dir: Path, tests_dir: Optional[Path] = None,
                   force: bool = False) -> int:
    """解析题面 markdown 中的样例，生成 tests/sampleNN.in / .out。"""
    text = md_path.read_text(encoding="utf-8")
    lines = text.splitlines()
    samples: Dict[int, Dict[str, str]] = {}
    i = 0
    while i < len(lines):
        line = lines[i]
        m_in = RE_SAMPLE_IN.search(line)
        m_out = RE_SAMPLE_OUT.search(line)
        if m_in:
            idx = int(m_in.group(1))
            body, nxt = _extract_fence_blocks(lines, i + 1)
            if body is not None:
                samples.setdefault(idx, {})["in"] = body
            i = nxt if nxt > i else i + 1
            continue
        if m_out:
            idx = int(m_out.group(1))
            body, nxt = _extract_fence_blocks(lines, i + 1)
            if body is not None:
                samples.setdefault(idx, {})["out"] = body
            i = nxt if nxt > i else i + 1
            continue
        i += 1

    if not samples:
        print(f"{md_path.name} 里没有解析到样例", file=sys.stderr)
        return 0

    tdir = tests_dir or (problem_dir / "tests")
    tdir.mkdir(parents=True, exist_ok=True)
    written = 0
    for idx in sorted(samples):
        pair = samples[idx]
        if "in" not in pair or "out" not in pair:
            print(f"  [警告] 样例 {idx} 缺少输入或输出，已跳过", file=sys.stderr)
            continue
        name = f"sample{idx:02d}"
        in_path = tdir / f"{name}.in"
        out_path = tdir / f"{name}.out"
        if not force and (in_path.exists() or out_path.exists()):
            print(f"  [保留] {name}（已存在，--force 覆盖）")
            continue
        in_path.write_text(pair["in"] + "\n", encoding="utf-8", newline="\n")
        out_path.write_text(pair["out"] + "\n", encoding="utf-8", newline="\n")
        print(f"  [写入] {name}.in / {name}.out")
        written += 1
    print(f"\n 完成：{written} 组样例 → {tdir}")
    return written


def cmd_import_samples(args: argparse.Namespace) -> int:
    md_path = Path(args.md).resolve()
    if not md_path.exists():
        print(f"题面不存在：{md_path}", file=sys.stderr)
        return 2
    if args.problem:
        problem_dir = resolve_problem(args.problem)
    else:
        # 从文件名推断，如 P1-shipping.md -> P1
        m = re.match(r"(P\d+)", md_path.stem, re.IGNORECASE)
        if not m:
            print("无法从文件名推断题号，请用 --problem 指定", file=sys.stderr)
            return 2
        problem_dir = PROBLEMS_ROOT / m.group(1).upper()
        problem_dir.mkdir(parents=True, exist_ok=True)
    tests_dir = Path(args.tests).resolve() if args.tests else None
    import_samples(md_path, problem_dir, tests_dir, force=args.force)
    return 0


# --------------------------------------------------------------------------
# 命令：list
# --------------------------------------------------------------------------

def cmd_list(args: argparse.Namespace) -> int:
    problem_dir = resolve_problem(args.problem)
    cfg = load_problem_config(problem_dir)
    print(f"题目: {problem_dir.name} · {cfg.get('title', '')}")
    print(f"构建: {cfg['build']}    时限: {cfg['time_limit_ms']} ms    "
          f"总分: {cfg['total_score']}    比对: {cfg['compare']}")
    cases = discover_tests(problem_dir)
    if not cases:
        print("（暂无用例）")
        return 0
    print(f"{'用例':<18}{'输入':>8}{'期望输出':>10}  权重")
    for c in cases:
        print(f"{c.name:<18}{c.in_path.stat().st_size:>8}{c.out_path.stat().st_size:>10}"
              f"  {c.weight:g}{'  ' + c.note if c.note else ''}")
    print(f"共 {len(cases)} 组")
    return 0


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="judge.py",
        description="FCAA101-02 练习 Project 自动评分器（Online-Judge 风格）",
    )
    sub = p.add_subparsers(dest="command", required=True)

    # run
    pr = sub.add_parser("run", help="编译并评测一份提交")
    pr.add_argument("problem", help="题号或题目目录（P1 / P6 / problems/P1）")
    pr.add_argument("submission", help="提交文件（单文件题）或目录（P6 工程题）")
    pr.add_argument("--tests", help="测试目录（默认 <problem>/tests）")
    pr.add_argument("--filter", help="只跑用例名包含该子串的用例")
    pr.add_argument("--tl", type=int, help="覆盖时间限制（毫秒）")
    pr.add_argument("--build", choices=["single", "make"], help="覆盖构建方式")
    pr.add_argument("--target", help="make 模式产物名（如 inspector）")
    pr.add_argument("--source-name", help="single 模式期望的源文件名")
    pr.add_argument("--compare", help="比对方式：exact | token | float:1e-6")
    pr.add_argument("--score", type=float, help="总分（默认 100）")
    pr.add_argument("--report", help="JSON 报告输出路径")
    pr.add_argument("--no-report", action="store_true", help="不写 JSON 报告")
    pr.add_argument("--keep-build", action="store_true", help="保留构建目录便于调试")
    pr.add_argument("--quiet", action="store_true", help="不逐条打印用例进度")
    pr.add_argument("-v", "--verbose", action="store_true", help="显示完整失败详情")
    pr.set_defaults(func=cmd_run)

    # gen
    pg = sub.add_parser("gen", help="用参考实现生成/更新期望输出 .out")
    pg.add_argument("problem", help="题号或题目目录")
    pg.add_argument("--ref", required=True, help="参考实现：.cpp 文件或工程目录")
    pg.add_argument("--inputs", nargs="*", help="指定 .in 文件（默认全部）")
    pg.add_argument("--tests", help="测试目录（默认 <problem>/tests）")
    pg.add_argument("--build", choices=["single", "make"], help="覆盖构建方式")
    pg.add_argument("--target", help="make 模式产物名")
    pg.set_defaults(func=cmd_gen)

    # import-samples
    pi = sub.add_parser("import-samples", help="从题面 markdown 提取样例生成用例")
    pi.add_argument("md", help="题面 markdown 文件")
    pi.add_argument("--problem", help="题号（默认按文件名 P?-xxx.md 推断）")
    pi.add_argument("--tests", help="测试目录（默认 <problem>/tests）")
    pi.add_argument("--force", action="store_true", help="覆盖已存在的用例")
    pi.set_defaults(func=cmd_import_samples)

    # list
    pl = sub.add_parser("list", help="列出某题的测试用例")
    pl.add_argument("problem", help="题号或题目目录")
    pl.set_defaults(func=cmd_list)

    return p


def main(argv: Optional[List[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

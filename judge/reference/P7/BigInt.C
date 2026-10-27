#include "BigInt.h"
#include <cstddef>
#include <cstring>      // strlen / memcpy
#include <iostream>

// ---------- 内部辅助 ----------

void BigInt::alloc(size_t cap) {
    if (cap < 1) cap = 1;                  // 至少容纳 '\0'
    char* old = s;
    const size_t oldCap = size;
    s = new char[cap];
    s[0] = '\0';
    if (old != nullptr) {
        // 最多保留 cap-1 个字符，给 '\0' 留位置
        const size_t keep = cap - 1;
        const size_t n = (oldCap < keep) ? oldCap : keep;
        for (size_t i = 0; i < n; ++i) {
            s[i] = old[i];
        }
        s[n] = '\0';
        delete[] old;
    }
    size = cap;
    if (len > size - 1) {
        len = size - 1;
    }
}

// 用 n 个数字字符（d[0] 最高位）把 *this 重置为该值；跳过前导零。
void BigInt::setFromDigits(const char* d, size_t n) {
    while (n > 1 && d[0] == '0') {         // 跳过前导零，但保留至少 1 位
        ++d;
        --n;
    }
    if (n == 0 || (n == 1 && d[0] == '0')) {   // 规范 0：isZero + len==0 + 空串
        alloc(2);
        len = 0;
        isZero = true;
        s[0] = '\0';
        return;
    }
    alloc(n + 1);
    for (size_t i = 0; i < n; ++i) {
        s[i] = d[i];
    }
    s[n] = '\0';
    len = n;
    isZero = false;
}

void BigInt::fromSizeT(size_t v) {
    if (v == 0) {
        alloc(2);
        len = 0;
        isZero = true;
        s[0] = '\0';
        return;
    }
    // 先数位数，再从后往前填
    size_t t = v, n = 0;
    while (t) {
        t /= 10;
        ++n;
    }
    alloc(n + 1);
    len = n;
    isZero = false;
    for (size_t i = n; i-- > 0; ) {
        s[i] = char('0' + v % 10);
        v /= 10;
    }
    s[len] = '\0';
}

void BigInt::fromCStr(const char* str) {
    if (str == nullptr) {
        alloc(2);
        len = 0;
        isZero = true;
        s[0] = '\0';
        return;
    }
    size_t n = 0;
    while (str[n] >= '0' && str[n] <= '9') {   // 数字前缀的长度
        ++n;
    }
    if (str[n] != '\0') {
        // 遇到非数字字符：cerr 诊断（评测忽略），取数字前缀
        std::cerr << "invalid: " << str << std::endl;
    }
    if (n == 0) {                               // 无任何数字 -> 规范 0
        alloc(2);
        len = 0;
        isZero = true;
        s[0] = '\0';
        return;
    }
    setFromDigits(str, n);
}

int BigInt::cmp(const BigInt& o) const {
    const bool az = isZero || (len == 0);     // isZero 是权威标志，len==0 双保险
    const bool bz = o.isZero || (o.len == 0);
    if (az || bz) {
        if (az && bz) return 0;
        return az ? -1 : 1;                  // 0 小于任何正数
    }
    if (len != o.len) {
        return (len < o.len) ? -1 : 1;       // 高位在前，位数多者更大
    }
    for (size_t i = 0; i < len; ++i) {
        if (s[i] != o.s[i]) {
            return (s[i] < o.s[i]) ? -1 : 1; // 首个不同位决定大小
        }
    }
    return 0;
}

// ---------- 构造 / 析构 / 赋值 ----------

BigInt::BigInt(size_t v) : isZero(true), len(0), size(0), s(nullptr) {
    fromSizeT(v);
}

BigInt::BigInt(const char* str) : isZero(true), len(0), size(0), s(nullptr) {
    fromCStr(str);
}

BigInt::BigInt(const BigInt& o) : isZero(true), len(0), size(0), s(nullptr) {
    alloc(o.size);
    len = o.len;
    isZero = o.isZero;
    if (o.s != nullptr) {
        for (size_t i = 0; i <= o.len; ++i) {   // 连 '\0' 一起拷
            s[i] = o.s[i];
        }
    } else {
        len = 0;
        isZero = true;
        s[0] = '\0';
    }
}

BigInt& BigInt::operator=(const BigInt& o) {
    if (this == &o) {
        return *this;                          // 自赋值安全
    }
    // 先分配新缓冲，再删旧的（alloc 内部完成），最后写入内容
    alloc(o.size);
    if (o.s != nullptr) {
        for (size_t i = 0; i <= o.len; ++i) {
            s[i] = o.s[i];
        }
        len = o.len;
        isZero = o.isZero;
    } else {
        len = 0;
        isZero = true;
        s[0] = '\0';
    }
    return *this;
}

BigInt::~BigInt() {
    delete[] s;
    s = nullptr;
}

// ---------- 算术：+ += * *= ----------

BigInt BigInt::operator+(const BigInt& o) const {
    // 从低位到高位竖式相加（索引越大位越低），结果最多 max(len)+1 位
    const size_t n = (len > o.len) ? len : o.len;
    char* d = new char[n + 2];               // +1 进位 +1 '\0'
    for (size_t t = 0; t < n + 2; ++t) d[t] = '0';
    size_t i = len, j = o.len, k = n + 1;    // 从结果末尾往前写
    int carry = 0;
    while (i > 0 || j > 0 || carry > 0) {
        int sum = carry;
        if (i > 0) { sum += s[i - 1] - '0'; --i; }
        if (j > 0) { sum += o.s[j - 1] - '0'; --j; }
        d[k] = char('0' + sum % 10);
        --k;
        carry = sum / 10;
    }
    // 进位最多使有效位数达到 n+2（即 k 恰好停在 0），从 k+1 到 n+1 共 n+1-k 位
    const size_t m = (n + 1) - k;
    BigInt r;
    r.setFromDigits(d + k + 1, m);
    delete[] d;
    return r;
}

BigInt BigInt::operator*(const BigInt& o) const {
    if (isZero || o.isZero || len == 0 || o.len == 0) {
        return BigInt(size_t(0));            // 任何数乘 0 得 0
    }
    // 竖式乘法：i（this 的低位索引）+ j（o 的低位索引）的结果写入低位索引 i+j
    const size_t m = len + o.len;
    int* acc = new int[m + 1];               // 用 int 累加中间量，避免 char 溢出
    for (size_t t = 0; t <= m; ++t) acc[t] = 0;
    for (size_t i = 0; i < len; ++i) {
        const int a = s[len - 1 - i] - '0';  // this 从低到高
        for (size_t j = 0; j < o.len; ++j) {
            const int b = o.s[o.len - 1 - j] - '0';
            acc[i + j] += a * b;             // 低位索引 i+j
        }
    }
    int carry = 0;
    char* d = new char[m + 1];
    for (size_t t = 0; t < m; ++t) {
        const int v = acc[t] + carry;
        d[m - 1 - t] = char('0' + v % 10);   // 写回时反转成 MSD 在前
        carry = v / 10;
    }
    delete[] acc;
    // 最高位进位（m 位时 carry 必为 0，但保险起见处理）
    size_t total = m;
    if (carry > 0) {
        char* d2 = new char[m + 2];
        d2[0] = char('0' + carry);
        for (size_t t = 0; t < m; ++t) d2[t + 1] = d[t];
        d2[m + 1] = '\0';
        delete[] d;
        d = d2;
        total = m + 1;
    }
    d[total] = '\0';
    BigInt r;
    r.setFromDigits(d, total);
    delete[] d;
    return r;
}

BigInt& BigInt::operator+=(const BigInt& o) {
    // 用 operator+ 得到新值再整体替换；o 可能就是 *this（别名）也没问题
    BigInt sum = (*this) + o;
    *this = sum;
    return *this;
}

BigInt& BigInt::operator*=(const BigInt& o) {
    BigInt prod = (*this) * o;
    *this = prod;
    return *this;
}

// ---------- 比较 ----------

bool BigInt::operator==(const BigInt& o) const { return cmp(o) == 0; }
bool BigInt::operator!=(const BigInt& o) const { return cmp(o) != 0; }
bool BigInt::operator< (const BigInt& o) const { return cmp(o) <  0; }
bool BigInt::operator<=(const BigInt& o) const { return cmp(o) <= 0; }
bool BigInt::operator> (const BigInt& o) const { return cmp(o) >  0; }
bool BigInt::operator>=(const BigInt& o) const { return cmp(o) >= 0; }

// ---------- 输出 ----------

std::ostream& operator<<(std::ostream& os, const BigInt& b) {
    if (b.isZero || b.len == 0) {
        os << '0';
        return os;
    }
    for (size_t i = 0; i < b.len; ++i) {
        os << b.s[i];                        // 已是 MSD 在前，不反转
    }
    return os;
}

#ifndef __BIGINT_H__
#define __BIGINT_H__
#include <cstddef>
#include <iostream>

class BigInt {
    bool   isZero;   // true 表示值为 0（与 len==0 严格同步）
    size_t len;      // 存储的位数（isZero 时为 0）
    size_t size;     // s 的分配容量（>= len + 1）
    char*  s;        // 数字串：s[0] 为最高位，s[len-1] 为最低位，s[len] = '\0'

    void alloc(size_t cap);            // 分配/扩容缓冲区（保留已有内容）
    void fromSizeT(size_t v);          // BigInt(size_t)
    void fromCStr(const char* str);    // BigInt(const char*)，含非法字符诊断
    int  cmp(const BigInt& o) const;   // -1/0/1，驱动六个比较运算符
    void setFromDigits(const char* d, size_t n);  // 用 n 个数字字符（MSD 在前）重置值
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

#endif

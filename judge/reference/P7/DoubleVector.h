#ifndef __DOUBLEVECTOR_H__
#define __DOUBLEVECTOR_H__
#include <cstddef>
#include <iostream>

class DoubleVector {
    size_t  size;   // 当前容量（可存元素个数），始终 >= count
    size_t  count;  // 当前元素个数
    double* data;   // 堆上缓冲区，长度 == size

    void grow();    // 容量不足时按 2 倍扩容（count == size 时调用）
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

#endif

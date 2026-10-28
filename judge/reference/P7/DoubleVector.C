#include "DoubleVector.h"
using namespace std;

DoubleVector::DoubleVector(size_t sz) : size(sz), count(0) {
    data = new double[size];   // new double[0] 是合法的，扩容时再补
}

DoubleVector::DoubleVector(const DoubleVector &v) : size(v.size), count(v.count) {
    data = new double[size];
    for (size_t i = 0; i < count; ++i) {
        data[i] = v.data[i];
    }
}

DoubleVector::~DoubleVector() {
    delete[] data;
}

void DoubleVector::grow() {
    size_t newSize = (size == 0) ? 1 : size * 2;
    double *newData = new double[newSize];
    for (size_t i = 0; i < count; ++i) {
        newData[i] = data[i];
    }
    delete[] data;
    data = newData;
    size = newSize;
}

void DoubleVector::push_back(double x) {
    if (count == size) {
        grow();
    }
    data[count++] = x;
}

DoubleVector &DoubleVector::operator=(const DoubleVector &v) {
    if (this != &v) {
        double *newData = new double[v.size];          // 先分配新的
        for (size_t i = 0; i < v.count; ++i) {
            newData[i] = v.data[i];
        }
        delete[] data;                                 // 再释放旧的
        data = newData;
        size = v.size;
        count = v.count;
    }
    return *this;
}

DoubleVector &DoubleVector::operator+=(const DoubleVector &right) {
    // 先把 right 的元素抄到临时缓冲：若 right 就是 *this，
    // 边遍历边 push_back 会在扩容后让 right.data 悬空（use-after-free）。
    size_t n = right.count;
    double *tmp = new double[(n == 0) ? 1 : n];
    for (size_t i = 0; i < n; ++i) {
        tmp[i] = right.data[i];
    }
    for (size_t i = 0; i < n; ++i) {
        push_back(tmp[i]);
    }
    delete[] tmp;
    return *this;
}

double DoubleVector::operator[](int i) const {
    if (i < 0 || static_cast<size_t>(i) >= count) {
        return 0.0;   // 越界返回 0.0，绝不访问内存
    }
    return data[i];
}

size_t DoubleVector::getCount() const {
    return count;
}

ostream &operator<<(ostream &os, const DoubleVector &v) {
    for (size_t i = 0; i < v.count; ++i) {
        if (i > 0) {
            os << ",";
        }
        os << v.data[i];
    }
    return os;
}

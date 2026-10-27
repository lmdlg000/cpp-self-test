// 演示：RE —— 空指针解引用，运行时崩溃
#include <iostream>
using namespace std;

int main() {
    int *p = NULL;
    cout << *p << endl;   // 访问违例 / 段错误
    return 0;
}

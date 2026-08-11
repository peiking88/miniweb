# Con: Constants and immutability

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **默认 const**：变量/成员函数/参数只要不需要修改就标 `const`。
- **能 constexpr 就 constexpr**：编译期常量与纯函数优先 constexpr（更早发现错误）。
- **接口承诺要真实**：`const` 成员函数不得通过 `mutable`/全局状态偷偷修改可观察行为（除非是缓存且线程安全）。

### 建议（SHOULD）
- 共享数据优先不可变（尤其并发模块），可变状态集中管理并明确同步策略。
- 使用 `string_view/span` 做只读视图，但必须保证底层对象生命周期。

### 禁止（MUST NOT）
- 禁止滥用 `mutable` 绕过 const-correctness。
- 禁止返回指向内部可变状态的非 const 引用/指针破坏封装。

## 2. 走查清单（Checklist）

- P0：`string_view/span` 是否悬垂？
- P1：是否存在应为 const 但没标导致误修改？
- P1：const 成员函数是否在多线程下产生数据竞争（缓存/惰性初始化）？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
constexpr int kMaxRetry = 3;

class Cache {
public:
 int get(int key) const; // 只读语义
};
```

### 不推荐（Bad）
```cpp
int MAX_RETRY = 3; // 可变全局，且命名像宏
```

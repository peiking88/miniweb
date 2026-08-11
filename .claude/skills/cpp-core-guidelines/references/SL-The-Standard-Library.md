# SL: The Standard Library

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **默认优先标准库容器与算法**：`vector/string/unordered_map/algorithm` 等。
- **不要手写容易出错的循环**：能用算法就用算法（可读性更高、也更容易优化）。
- **时间/时长用 `std::chrono`**，避免 int 表示毫秒/秒的“单位地狱”。

### 建议（SHOULD）
- 用 `optional/variant/expected(若可用)` 表达可选/多态返回。
- 用 `string_view/span` 表达只读视图（但需保证生命周期）。

### 禁止（MUST NOT）
- 禁止重复造轮子（自写 vector/string/shared_ptr），除非有明确平台/性能约束且通过架构评审。

## 2. 走查清单（Checklist）

- P0：是否自研容器/字符串导致内存与异常安全问题？
- P1：是否用裸 int 表示时间/大小单位？
- P2：是否存在可用算法简化的手写循环？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
std::sort(v.begin(), v.end());

using namespace std::chrono_literals;
auto timeout = 200ms;
```

### 不推荐（Bad）
```cpp
// 手写排序/手写字符串拼接，易错且难维护
```

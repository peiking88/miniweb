# GSL: Guidelines support library

## 1. 推荐使用的 GSL 组件（团队落地优先级）

### 必须（MUST）
- `not_null<T*>`：表达不可为空指针（替代隐式约定）。
- `span<T>`：表达连续区间/数组参数（替代 `T* + n`）。
- `owner<T*>`：仅用于遗留接口过渡（表达 owning 裸指针），新代码优先智能指针。
- `finally`/`final_action`：当没有合适 RAII handle 时表达清理动作。

### 建议（SHOULD）
- `zstring/czstring`：与 C 字符串交互时表达 0 结尾语义。
- `Expects/Ensures`：表达契约（前置/后置条件），提升可检查性。

## 2. 走查清单（Checklist）

- P0：是否可以用 `span/not_null` 让接口语义更明确并减少 UB？
- P1：是否仍在用 `(T*, int)` 表达范围？
- P1：是否仍在用裸 owning 指针但没标注 owner？

## 3. 示例

```cpp
void f(gsl::span<int> xs);
void g(gsl::not_null<Foo*> p);

Expects(!xs.empty());
Ensures(result >= 0);
```

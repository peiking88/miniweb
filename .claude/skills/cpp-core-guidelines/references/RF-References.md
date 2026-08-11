# RF: References

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **引用（`T&`/`const T&`）默认表示“必须存在且生命周期覆盖调用方使用期”**。
- **可选语义不要用引用**：
 - 可空用 `T*` 或 `std::optional<std::reference_wrapper<T>>`（慎用）。
- **禁止返回悬垂引用**：不得返回指向局部对象/临时对象/已释放存储的引用。

### 建议（SHOULD）
- 需要表达“借用只读视图”时，优先 `const T&`，但要避免把引用存到异步任务/成员里。
- 对跨线程/异步场景，默认按值传递或用共享所有权（shared_ptr）并显式约束。

### 禁止（MUST NOT）
- 禁止用引用作为协程参数跨越挂起点（若使用协程）。

## 2. 走查清单（Checklist）

- P0：是否返回/保存了悬垂引用？
- P1：是否把“可选”做成引用导致不得不传假对象？
- P1：lambda/线程/协程是否捕获引用导致悬垂？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
void use(const Config& cfg);

Config* try_get(); // optional semantics
```

### 不推荐（Bad）
```cpp
Config& try_get(); // bad: 可选语义却用引用
```

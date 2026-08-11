# T: Templates and generic programming

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **模板接口必须可约束**：
 - C++20 优先 `requires`/concepts；
 - 否则用 `static_assert` + type traits 给出清晰错误信息。
- **模板代码必须可读**：避免深层 SFINAE 黑魔法；能用普通重载就不用模板。
- **泛型代码不得隐藏昂贵操作**：例如在模板中隐式拷贝大对象，需显式用 `const&`/`span`/完美转发。

### 建议（SHOULD）
- 为泛型提供最小可用示例与约束说明（在头文件注释中写清楚）。
- 使用 `auto`/范围 for/算法提升可读性，但避免过度类型擦除导致语义不清。

### 禁止（MUST NOT）
- 禁止让模板参与公共 ABI 边界而不加控制（会导致编译依赖爆炸/ABI 不稳定）。

## 2. 走查清单（Checklist）

- P0：模板参数是否缺少约束，导致误用在编译期报出“天书”？
- P1：模板中是否存在不必要拷贝、隐藏分配、隐藏异常？
- P2：是否过度使用模板导致编译时间/二进制膨胀？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
template <class Iter>
requires std::input_iterator<Iter>
auto sum(Iter first, Iter last) {
 using V = typename std::iterator_traits<Iter>::value_type;
 V v{};
 for (; first != last; ++first) v += *first;
 return v;
}
```

### 不推荐（Bad）
```cpp
// 无约束模板：任何类型都能进来，报错难读
template <class T>
void f(T x) { x.nonexistent(); }
```

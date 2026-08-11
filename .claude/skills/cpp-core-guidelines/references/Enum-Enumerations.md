# Enum: Enumerations

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **默认使用 `enum class`**（作用域枚举），禁止无作用域 `enum` 在新代码中扩散。
- **跨模块/序列化/ABI 边界必须明确底层类型**：`enum class X : std::uint8_t`。
- **switch 必须覆盖所有枚举值**：
 - 对“封闭枚举”（不应出现未知值）不写 `default`，让编译器告警遗漏；
 - 对“可扩展/外部输入”枚举必须有 `default` 并做 fail-fast 或错误返回。

### 建议（SHOULD）
- 枚举值命名统一前缀/语义一致，避免与宏/常量冲突。
- 需要位运算时，明确这是 bitmask enum，并提供安全的按位操作封装。

### 禁止（MUST NOT）
- 禁止把枚举当 int 用：随意与整数混算/比较/序列化。

## 2. 走查清单（Checklist）

- P0：是否把枚举当 int 使用导致非法值流入？
- P1：switch 是否遗漏分支？是否依赖 default 吞掉新枚举值？
- P2：是否存在无作用域 enum 污染命名空间？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
enum class Color : std::uint8_t { red, green, blue };

std::string_view to_string(Color c) {
 switch (c) {
  case Color::red: return "red";
  case Color::green: return "green";
  case Color::blue: return "blue";
 }
 // 对封闭枚举：不写 default，让编译器帮助你
 std::terminate();
}
```

### 不推荐（Bad）
```cpp
enum Color { red, green, blue }; // 污染作用域

int x = red; // 变成 int，丢失语义
```

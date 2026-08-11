# CPL: C-style programming

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **避免 C 风格 API 直接泄漏到业务代码**：与 C API 交互必须做 RAII 封装与边界隔离。
- **使用现代替代方案**：
 - 裸数组 -> `std::array/std::vector/std::span`
 - C字符串 -> `std::string/std::string_view`（或 GSL zstring）
 - 宏常量 -> `constexpr`
 - 宏函数 -> `inline`/模板/`constexpr` 函数
 - 变参 -> 可变模板/重载/容器参数

### 建议（SHOULD）
- 用 `<cxxx>` 头（`<cstdio>`）替代 `<stdio.h>`，减少命名污染。
- 对 legacy 的 `malloc/free`、`fopen/fclose`、`pthread_*` 统一封装。

### 禁止（MUST NOT）
- 禁止 C cast（`(T)x`）：必须用命名 cast 并在评审中说明。
- 禁止宏做文本替换（影响可读性/可调试性/命名冲突）。

## 2. 走查清单（Checklist）

- P0：是否存在 C cast、裸数组越界、printf/scanf 格式化风险？
- P1：是否用宏实现常量/函数？是否引入命名冲突？
- P1：是否出现变参函数、手工内存管理？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
constexpr int kBufSize = 4096;

void f(std::span<const std::byte> buf);
```

### 不推荐（Bad）
```cpp
#define BUF_SIZE 4096

void f(unsigned char* buf, int n);
```

# SF: Source files

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **头文件最小化依赖**：
 - 头文件只包含它真正需要的头（include-what-you-use）；
 - 尽量用前置声明减少耦合（但注意完整类型需求）。
- **头文件禁止 `using namespace ...`**。
- **每个头文件必须可独立编译**（能被单独 include 而不依赖顺序）。
- **接口与实现分离**：对稳定 ABI/编译时间敏感模块用 Pimpl。

### 建议（SHOULD）
- 统一 include 顺序：本项目头 -> 第三方 -> 标准库。
- 限制 header-only 的扩散（编译时间与 ODR 风险）。

### 禁止（MUST NOT）
- 禁止在头文件里定义非 inline 的全局对象（ODR/初始化顺序风险）。

## 2. 走查清单（Checklist）

- P0：是否存在 ODR 问题（头里定义变量/非 inline 函数）？
- P1：是否依赖 include 顺序才能编译？
- P1：是否把实现细节暴露到头导致编译依赖爆炸？

## 3. 推荐/不推荐

### 推荐（Good）
```cpp
// header: 声明
class widget {
 struct impl;
 std::unique_ptr<impl> p;
public:
 widget();
 ~widget();
};
```

### 不推荐（Bad）
```cpp
// header: 定义全局对象
int g_counter = 0; // bad
```

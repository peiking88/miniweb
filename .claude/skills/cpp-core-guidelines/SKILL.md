---
name: cpp-core-guidelines
description: "C++ 代码审查与质量改进技能，基于 C++ Core Guidelines 提供结构化走查流程。当用户需要：(1) 审查 C++ 代码质量、安全性、性能、可维护性；(2) 查找特定规则编号（如 F.16、R.11、C.21）；(3) 制定重构或整改计划；(4) 解决资源泄漏、内存安全、并发问题；(5) 设计符合现代 C++ 的接口与类层次；(6) 了解 RAII、智能指针、异常安全等最佳实践时触发此技能。"
---

# C++ Core Guidelines 代码审查技能

基于 C++ Core Guidelines 的结构化代码走查与审阅流程，输出问题清单、风险等级与修复建议。

## 快速参考：规则章节

| 章节 | 主题 | 何时查阅 |
|------|------|----------|
| P | Philosophy | 审查设计哲学、代码意图表达 |
| I | Interfaces | 审查 API 设计、参数传递、接口契约 |
| F | Functions | 审查函数设计、参数、返回值 |
| C | Classes | 审查类设计、继承、多态、对象生命周期 |
| Enum | Enumerations | 审查枚举使用 |
| R | Resource | 审查资源管理、内存、智能指针、RAII |
| ES | Expressions | 审查表达式、语句、控制流 |
| Per | Performance | 审查性能问题、优化建议 |
| CP | Concurrency | 审查并发、线程安全、数据竞争 |
| E | Error handling | 审查异常处理、错误传播 |
| Con | Constants | 审查常量、不可变性、constexpr |
| T | Templates | 审查模板、泛型编程、概念 |
| CPL | C-style | 审查 C 风格代码、与现代 C++ 的互操作 |
| SF | Source files | 审查源文件组织、头文件 |
| SL | Standard Library | 审查标准库使用 |
| NR | Non-Rules | 识别常见误区与伪规则 |
| GSL | Guidelines Support Library | 审查 GSL 类型使用 |
| NL | Naming/Layout | 审查命名与代码布局 |

详细规则请查阅 `references/` 目录下对应文件。

## 走查流程

### 1. 明确范围与约束

收集以下信息：
- 走查范围：文件/模块/函数列表
- 编译标准：C++17/20/23
- 重点关注：性能 / 并发 / 安全 / 可靠性 / API 设计
- 已有分析结果：clang-tidy、sanitizers 等

### 2. 按优先级执行检查

**P0 - 必须修复（安全/正确性）**
- 资源泄漏（R）：裸指针所有权、RAII 违规
- 未定义行为（ES）：空指针解引用、越界访问
- 数据竞争（CP）：并发访问共享状态
- 异常安全（E）：资源泄漏路径

**P1 - 应该修复（可维护性/性能）**
- 接口设计（I/F）：参数传递方式、返回值
- 类设计（C）：不变量、继承层次
- 性能问题（Per）：不必要的拷贝、分配

**P2 - 建议改进（代码质量）**
- 命名与布局（NL）
- 标准库使用（SL）
- 代码风格一致性

### 3. 查阅参考文件

根据发现的问题类型，查阅对应的 reference 文件：

```
发现资源管理问题 → 查阅 references/R-Resource-management.md
发现接口设计问题 → 查阅 references/I-Interfaces.md
发现并发问题     → 查阅 references/CP-Concurrency-and-parallelism.md
```

### 4. 输出审查报告

## 输出格式

### 审查摘要

```markdown
## 审查摘要

- 文件：[文件名]
- 标准：C++[版本]
- 发现问题：[数量] 个（P0: x, P1: y, P2: z）
- 主要风险：[2-4 条要点]
```

### 问题清单

```markdown
| ID | 严重性 | 位置 | 规则 | 证据 | 修复建议 |
|----|--------|------|------|------|----------|
| 1 | P0 | foo.cpp:42 | R.3 | `T* p = new T();` 无 RAII | 改用 `auto p = std::make_unique<T>();` |
| 2 | P1 | bar.h:15 | F.16 | `void f(const std::string& s)` 对小类型 | 改为 `void f(std::string_view s)` |
```

### 快速收益项

列出可在一次迭代内完成的改进：
- [ ] 替换裸指针为智能指针
- [ ] 添加 `[[nodiscard]]` 属性
- [ ] 使用 `std::span` 替代指针+长度

## 常用规则速查

### 资源管理（R）
- **R.3**: 裸指针不表示所有权
- **R.11**: 避免显式 new/delete，使用 make_unique/make_shared
- **R.20**: 所有权用 unique_ptr 表达
- **R.21**: 共享所有权用 shared_ptr 表达

### 函数（F）
- **F.15**: 优先简单、常规的参数传递方式
- **F.16**: 输入参数用 `const T&` 或 `T*`
- **F.17**: 输出参数用非 const 指针或引用
- **F.20**: 输出值优先用返回值而非输出参数

### 类（C）
- **C.20**: 尽可能避免定义默认操作
- **C.21**: 定义或删除所有默认操作
- **C.43**: 确保类有默认构造函数（如可行）
- **C.44**: 尽量让构造函数简单、非虚

### 并发（CP）
- **CP.2**: 避免数据竞争
- **CP.3**: 最小化共享可写数据
- **CP.20**: 使用 RAII 管理锁

## 资源文件

- `docs/CppCoreGuidelines.md`：完整指南原文（权威来源）
- `references/`：章节化指南文件，按需查阅

### references/ 文件清单

| 文件 | 行数 | 内容概述 |
|------|------|----------|
| C-Classes-and-class-hierarchies.md | 273 | 类设计、继承、多态 |
| ES-Expressions-and-statements.md | 169 | 表达式、语句、控制流 |
| CP-Concurrency-and-parallelism.md | 159 | 并发、线程安全 |
| E-Error-handling.md | 146 | 异常处理、错误传播 |
| I-Interfaces.md | 143 | 接口设计、参数传递 |
| F-Functions.md | 141 | 函数设计、返回值 |
| R-Resource-management.md | 126 | RAII、智能指针 |
| T-Templates-and-generic-programming.md | 44 | 模板、泛型 |
| SF-Source-files.md | 44 | 源文件组织 |
| CPL-C-style-programming.md | 42 | C 风格代码 |
| P-Philosophy.md | 41 | 设计哲学 |
| Per-Performance.md | 41 | 性能优化 |
| Con-Constants-and-immutability.md | 39 | 常量、不可变性 |
| SL-The-Standard-Library.md | 36 | 标准库使用 |
| RF-References.md | 36 | 引用规则 |
| Enum-Enumerations.md | 47 | 枚举使用 |
| GSL-Guidelines-support-library.md | 29 | GSL 类型 |
| NL-Naming-and-layout-suggestions.md | 26 | 命名、布局 |
| Pro-Profiles.md | 23 | 配置文件 |
| NR-Non-Rules-and-myths.md | 22 | 常见误区 |
| C++-Core-Guidelines.md | 64 | 总览 |

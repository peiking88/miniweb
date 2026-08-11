---
name: cpp-code-review
description: |
  组合式 C++ 代码评审方案 — 整合静态分析、安全编码规范、C++ 编程规范（含现代 C++）、
  C 语言编程规范、经验与教训、高风险函数检查、多轮迭代评审。覆盖 14 个评审维度：内存安全、
  未定义行为、RAII/所有权、性能、安全编码、高风险函数、并发安全、现代 C++、代码规范、
  函数设计、类与对象、输入验证、日志与错误处理、C语言编程经验与教训。
  触发词：review cpp、cpp 代码评审、C++ review、代码审查、安全编码审查、cpp-code-review。
license: MIT
---

# C++ Code Review

组合式 C++ 代码评审方案，整合静态分析、C++专项检查、安全编码规范检查、C++编程规范检查、高风险函数检查、C 语言编程规范检查、经验与教训、多轮迭代评审。

---

## 架构

```
用户发起 C++ 代码评审
         │
         ▼
┌──────────────────────────────┐
│  1. 规范驱动的专项预检          │
│     ├── C++ 专项检查          │  ← cpp skill + regex 预检
│     ├── 安全编码规范检查        │  ← references/secure-coding.md
│     ├── C++编程规范检查        │  ← references/cpp-programming-guide.md
│     ├── 高风险函数扫描         │  ← references/secure-coding.md (高风险函数章节)
│     └── C语言编程规范检查       │  ← references/c-programming-guide.md
└────────────┬─────────────────┘
             │
             ▼
┌──────────────────────────────┐
│  2. 三路并行 AI 评审           │
│   Reviewer-1 → 功能正确性      │  逻辑/边界/异常/断言
│   Reviewer-2 → 性能与内存      │  分配/拷贝/算法/缓存
│   Reviewer-3 → 安全与最佳实践  │  注入/溢出/并发/规范
└────────────┬─────────────────┘
             │
             ▼
┌──────────────────────────────┐
│  3. 汇总 + 评分报告           │  13 维度评分 + 修复优先级
└────────────┬─────────────────┘
             │
             ▼
┌──────────────────────────────┐
│  4. 可选自动修复              │  自动修复 bug/安全/风格
└──────────────────────────────┘
```

---

## 评审维度（13 维度全覆盖）

| # | 维度 | 关注点 | 规范来源 |
|---|------|--------|---------|
| 1 | **内存安全** | 泄漏、悬垂引用、野指针、double-free、use-after-free、初始化 | checklist + 编程规范 |
| 2 | **未定义行为** | 未初始化、数据竞争、整数溢出、类型双关、精度溢出 | checklist |
| 3 | **RAII/所有权** | 构造/析构顺序、智能指针、初始化列表、虚析构 | checklist + 编程规范 |
| 4 | **性能** | 不必要拷贝、临时对象、虚函数开销、容器选择 | checklist |
| 5 | **安全编码** | 注入防护、溢出、路径遍历、加密、随机数、防重放 | 安全编码规范 |
| 6 | **高风险函数** | 线程不安全函数、缓冲区溢出函数、信号不安全函数 | 安全编码规范 |
| 7 | **并发安全** | 锁粒度、死锁、atomic、线程不安全全局状态 | checklist + 编程规范 |
| 8 | **现代C++** | constexpr、string_view、format、Concepts、override | checklist |
| 9 | **代码规范** | 命名、头文件组织、格式、include guard | 编程规范 |
| 10 | **函数设计** | 参数、返回值、const正确性、assert | 编程规范 |
| 11 | **类与对象** | 成员顺序、初始化列表、虚析构、职责单一 | 编程规范 |
| 12 | **输入验证** | SQL注入、XSS、CSRF、路径遍历、文件上传 | 安全编码规范 |
| 13 | **日志与错误处理** | 敏感信息泄露、审计日志、统一错误处理 | 安全编码规范 |

---

## 资源文件

### 参考规范文件
评审时按需加载以下参考文件获取详细规则和示例：

| 文件 | 内容 | 何时加载 |
|------|------|---------|
| [references/cpp-checklist.md](references/cpp-checklist.md) | 14 维度完整检查清单（主索引） | 每次评审必读 |
| [references/secure-coding.md](references/secure-coding.md) | 安全编码规范 + 高风险函数完整清单 | 安全/高风险函数检查时 |
| [references/cpp-programming-guide.md](references/cpp-programming-guide.md) | C++编程规范 + 代码审查表 + 现代C++ | 代码规范/函数设计/类设计/现代C++特性检查时 |
| [references/c-programming-guide.md](references/c-programming-guide.md) | C 语言编程规范、经验与教训 | 指针安全/内存管理/表达式陷阱/预处理检查时 |
| [references/severity.md](references/severity.md) | 问题严重级别定义 + 评分公式 | 生成评分报告时 |
| [references/workflow.md](references/workflow.md) | 完整工作流程 | 首次使用或复杂评审时 |

### 脚本工具
- [scripts/analyze_cpp.py](scripts/analyze_cpp.py) — C++ 代码快速分析脚本，检测常见问题模式

### 示例代码
- [assets/examples/bad_example.cpp](assets/examples/bad_example.cpp) — 包含常见问题的C++/c代码示例
- [assets/examples/good_example.cpp](assets/examples/good_example.cpp) — 修复后的安全C++/c代码示例

---

## 工作流程

### Step 1 — 收集代码上下文

- 用户指定文件、目录或 git diff
- 检测是否 PR 场景（`gh pr status` / `git log`）
- 判断增量审查还是全量审查
- 识别 C++ 版本和编译标准

### Step 2 — 规范驱动的专项预检

1. **加载检查清单**：读取 [references/cpp-checklist.md](references/cpp-checklist.md) 作为主索引
2. **执行静态分析**：运行 [scripts/analyze_cpp.py](scripts/analyze_cpp.py) 进行快速问题检测
3. **按需加载详细规范**：根据代码特点加载相应的参考文件：
   - 安全相关问题 → [references/secure-coding.md](references/secure-coding.md)
   - cpp代码规范问题 → [references/cpp-programming-guide.md](references/cpp-programming-guide.md)
   - C代码规范问题 → [references/c-programming-guide.md](references/c-programming-guide.md)

### Step 3 — 三路并行 AI 评审

启动 3 个独立 reviewer，每个关注不同维度：

- **Reviewer-1（功能正确性）**：逻辑错误、边界条件、断言、异常处理、UB
- **Reviewer-2（性能与内存）**：内存分配、拷贝语义、算法复杂度、缓存友好性、RAII
- **Reviewer-3（安全与最佳实践）**：注入防护、缓冲区安全、高风险函数、并发安全、编程规范、Modern C++

### Step 4 — 汇总报告

综合所有 reviewer 意见，按 [references/severity.md](references/severity.md) 定级，输出统一报告：

```markdown
## C++ Code Review Report

### 项目概览
- 评审范围：<文件/目录/diff>
- 评审模式：<全量/增量/PR>
- 评审时间：<timestamp>

### 问题汇总（按严重性排序）
| 严重性 | 维度 | 位置 | 问题描述 | 规范依据 |
|--------|------|------|----------|---------|
| 🔴 P0 | Security | foo.cpp:42 | gets() — 缓冲区溢出 | 安全编码规范 |
| � P0 | Memory | bar.cpp:87 | delete后未置nullptr | 编程规范-内存管理 |
| � P1 | Concurrency | baz.cpp:23 | strtok多线程不安全 | 高风险函数清单 |
| � P2 | Standards | utils.cpp:15 | 缺少include guard | 编程规范-文件结构 |
| 🟢 P3 | Modern C++ | main.cpp:8 | 建议用constexpr替代宏 | 现代C++ |

### 代码评分（百分制）
| 维度 | 得分 |
|------|------|
| 内存安全 | XX/100 |
| 逻辑正确性 | XX/100 |
| 性能 | XX/100 |
| 安全 | XX/100 |
| 可读性 | XX/100 |
| **总分** | **XX/100** |

### 修复优先级
1. [P0 - 必须修复] ...
2. [P1 - 强烈建议] ...
3. [P2 - 建议优化] ...
4. [P3 - 可选改进] ...
```

### Step 5 — 自动修复（可选）

用户确认后执行修复：

```bash
/cpp-review --fix          # 审查并自动修复
/cpp-review --security     # 仅安全检查
/cpp-review --explain      # 学习模式（带详细解释）
```

---

## 触发方式

| 命令 | 场景 |
|------|------|
| `/cpp-review` | 评审当前打开的 C++ 文件 |
| `/cpp-review <file>` | 评审指定文件 |
| `/cpp-review --diff` | 评审当前 git diff |
| `/cpp-review --pr` | 评审当前 PR |
| `/cpp-review --fix` | 评审并自动修复 |
| `/cpp-review --full` | 全量项目评审 |
| `/cpp-review --security` | 仅安全编码检查 |
| `/cpp-review --explain` | 学习模式（带详细解释） |

---

## 依赖的 Skills

| Skill | 用途 |
|-------|------|
| `cpp` | C++ 专项规则（内存、UB、所有权） |
| `code-review-sr` | 本地静态分析 + AI 深度评审 |
| `iterative-code-review` | 多轮迭代 + 多 reviewer 并行 |
| `modified-code-review` | 评分报告 + 性价比分析 |
| `code-review-fix` | 自动修复 |

---

## 限制与注意

- **并发限制**：同时最多 3 个 subagent 并行评审
- **最大轮次**：10 轮迭代（防止无限循环）
- **安全模式**：默认每步需用户确认，不会自动修改代码
- **代码不外传**：不向外部 API 发送时使用本地分析模式

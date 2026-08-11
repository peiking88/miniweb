# C++ Core Guidelines

## 1. 团队落地原则（行业经验版）

### 1.1 必须（MUST）
- **以“风险驱动”落地**：优先解决 P0 问题（UB/越界/并发数据竞争/资源泄漏/悬垂/线程生命周期）。
- **将规则变成门禁**：
 - 新增代码必须满足核心规则集；
 - 存量代码通过“触碰即治理（boy-scout rule）”与专项重构逐步清理。
- **约定统一的 Review 输出**：每条问题必须包含：位置、严重性（P0/P1/P2）、规则映射、修复建议、是否可自动化。

### 1.2 建议（SHOULD）
- **先选一个 Profile** 再扩展：例如先用 Lifetime/Bounds/Type safety 相关规则形成“安全剖面”。
- **用工具替代口头约束**：clang-tidy + sanitizer + CI 才能规模化执行。
- **建立“例外机制”**：少量规则在性能热点/兼容约束下允许例外，但必须：说明理由 + 单测 + sanitizer 覆盖 + 注释标记。

## 2. 走查流程（Checklist）

- P0：是否存在 UB、越界、悬垂、资源泄漏、数据竞争、线程 detach 生命周期失控？
- P1：接口语义是否清晰（所有权/可空性/范围/异常保证）？类是否维持不变量？
- P2：可读性与一致性（命名/布局/重复代码/标准库优先）。

## 3. 工具化建议（建议作为 CI 必选项）

- **clang-tidy**：启用 `cppcoreguidelines-*`、`modernize-*`、`bugprone-*`、`performance-*`（按模块逐步加严）。
- **Sanitizers**：
 - ASan/UBSan：默认在 debug/CI nightly 运行；
 - TSan：并发模块/核心链路定期跑。
- **静态分析**：clang-analyzer/coverity（如有）对资源与路径分析更强。

## 4. 例外（Exception）治理模板

> 适用于确实需要违反某条指南的情况。

- 规则：例如 `R.11`（避免显式 new/delete）
- 例外原因：性能热点/ABI/C API/平台限制
- 风险评估：泄漏/异常安全/并发影响
- 防护措施：RAII 封装、单元测试、sanitizer 覆盖、注释标记
- 退出计划：未来可替换方案与时间窗口

## 5. 指南章节走查项（按章节）

> 建议在每次 Review 中按风险优先级执行：R/ES/CP/E/C 优先。

- [ ] P: Philosophy（P-Philosophy.md）
- [ ] I: Interfaces（I-Interfaces.md）
- [ ] F: Functions（F-Functions.md）
- [ ] C: Classes and class hierarchies（C-Classes-and-class-hierarchies.md）
- [ ] Enum: Enumerations（Enum-Enumerations.md）
- [ ] R: Resource management（R-Resource-management.md）
- [ ] ES: Expressions and statements（ES-Expressions-and-statements.md）
- [ ] Per: Performance（Per-Performance.md）
- [ ] CP: Concurrency and parallelism（CP-Concurrency-and-parallelism.md）
- [ ] E: Error handling（E-Error-handling.md）
- [ ] Con: Constants and immutability（Con-Constants-and-immutability.md）
- [ ] T: Templates and generic programming（T-Templates-and-generic-programming.md）
- [ ] CPL: C-style programming（CPL-C-style-programming.md）
- [ ] SF: Source files（SF-Source-files.md）
- [ ] SL: The Standard Library（SL-The-Standard-Library.md）
- [ ] NR: Non-Rules and myths（NR-Non-Rules-and-myths.md）
- [ ] RF: References（RF-References.md）
- [ ] Pro: Profiles（Pro-Profiles.md）
- [ ] GSL: Guidelines support library（GSL-Guidelines-support-library.md）
- [ ] NL: Naming and layout suggestions（NL-Naming-and-layout-suggestions.md）

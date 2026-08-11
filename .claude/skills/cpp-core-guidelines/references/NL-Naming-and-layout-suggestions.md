# NL: Naming and layout suggestions

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **一致性优先**：项目内遵循同一套命名/格式化规则（由 clang-format/代码规范定义）。
- **一个声明一行**（变量/using/typedef/枚举项按需例外）。
- **命名表达语义与单位**：例如 `timeout_ms`、`size_bytes`。

### 建议（SHOULD）
- 本地变量短、类型/全局/跨模块名称更长更明确。
- 避免形似命名（`l/I/1/O/0`），减少视觉误判。

### 禁止（MUST NOT）
- 禁止滥用 `ALL_CAPS`（除宏名）；宏必须唯一且有前缀。

## 2. 走查清单（Checklist）

- 命名是否表达意图？是否含单位？
- 是否存在缩写过度导致不可读？
- 是否存在宏名冲突风险？

## 3. 工具化建议

- clang-format 统一格式化（作为 CI 门禁）。
- clang-tidy/自定义脚本检查命名（可选）。

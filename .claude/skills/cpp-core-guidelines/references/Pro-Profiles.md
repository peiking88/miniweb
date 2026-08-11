# Pro: Profiles

## 1. 编码规范（行业经验版）

### 必须（MUST）
- **团队必须选定并维护一个最小 Profile 集合**（例如：Lifetime + Bounds + Type safety），并作为新增代码门禁。
- **Profile 必须可工具化**：能被 clang-tidy/静态分析规则集表达，避免纯口头规则。
- **Profile 必须有例外流程**：违反规则需要记录理由、风险与验证手段。

### 建议（SHOULD）
- 对存量代码采用“分层治理”：核心链路最先达标，边缘模块逐步推进。
- 将 Profile 指标可视化（lint 数量趋势、P0 清零率、sanitizer 覆盖率）。

## 2. 走查清单（Checklist）

- 是否明确了本模块适用的 Profile？
- 是否把 Profile 规则落到了 CI（而不是写在文档里）？
- 例外是否有退出计划？

## 3. 工具化建议

- 为每个 Profile 维护 clang-tidy 配置（`.clang-tidy`）与 baseline。
- 对关键 Profile 配套 sanitizer 运行（ASan/UBSan/TSan）。

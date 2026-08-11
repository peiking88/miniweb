# I: Interfaces

## 1. 编码规范（行业经验版）

### 1.1 必须（MUST）

- **接口必须表达“所有权、生命周期、可空性、范围”**：
 - 所有权：`std::unique_ptr` / `std::shared_ptr`（必要时）/ `gsl::owner<T*>`（遗留接口过渡）
 - 可空性：可空用 `T*`，不可空用 `gsl::not_null<T*>` 或 `T&`
 - 范围：用 `gsl::span<T>` / `std::span<T>` 表达数组区间，不用 `(T*, int)`
 - C 字符串语义：用 `gsl::zstring/czstring`（或 `std::string_view` + 明确 0 结尾约束）
- **接口契约必须可验证**：对外暴露的 API 至少要有“前置条件/后置条件/错误语义”三件事之一：
 - 代码层面：`Expects()` / `Ensures()`（GSL），或断言/静态断言
 - 文档层面：在声明处注释清楚（尤其是线程安全、异常保证、所有权）
- **接口不得依赖隐式全局状态**（典型反例：`errno`、隐式单例、全局开关影响函数语义）。
- **接口不得通过“含义不明的基本类型组合”传参**：避免多个 `int/double/bool` 组合表达业务含义。

### 1.2 建议（SHOULD）

- **参数数量控制**：同一函数参数过多（经验阈值：>4）时优先：
 - 用 `struct Params { ... }` 打包；或
 - 拆分为更小、更明确的 API
- **避免相邻同类型参数**（易交换导致隐蔽 bug），优先用强类型/封装类型。
- **稳定 ABI 场景使用 Pimpl**（或 C ABI 边界），在 ABI/跨库边界减少模板、内联、异常穿透。
- **接口命名表达意图与单位**：时间/长度等必须带单位（`ms`/`s`/`bytes`/`count`）。

### 1.3 禁止（MUST NOT）

- **禁止返回“拥有所有权”的裸指针**（例如 `T* make()`），除非是明确的 legacy 约束且在接口上强制约束（例如 `owner<T*>` + 文档 + 统一释放方）。
- **禁止“布尔旗标参数”堆叠**（`set(true,false,...)`），应使用枚举/配置结构体/策略对象。
- **禁止跨翻译单元（TU）的复杂全局初始化依赖**（静态初始化顺序灾难）。

## 2. 走查清单（Checklist）

> 建议在走查报告中用 P0/P1/P2 标注严重性：P0=可能崩溃/安全漏洞/资源泄漏；P1=高概率逻辑错误/可维护性灾难；P2=风格/可读性/一致性。

- P0：是否存在**返回局部对象地址/引用**、悬垂引用、越界 span/指针算术？
- P0：是否存在**所有权不清晰**（裸指针到底谁释放？shared_ptr 是否形成环？）
- P1：是否存在**隐式全局依赖**（全局变量、单例、`errno`、全局开关影响行为）
- P1：是否存在**参数语义不清**（单位不明、多个 bool、多个同类型参数）
- P1：是否把“范围”错误表达成 `(T*, int)` 或 `T*` 并下标？
- P2：接口是否把实现细节泄漏到 API（暴露内部容器类型、暴露可变全局）？

## 3. 常见工程坑（经验补充）

- **shared_ptr 入参滥用**：仅为“方便共享”而在接口层到处传 `shared_ptr`，会导致生命周期耦合、循环引用、性能抖动（原子 refcount）。
- **string_view 误用**：跨线程/异步保存 `string_view`，但底层字符串已释放。
- **默认参数 + 虚函数**：默认参数是静态绑定，覆写后易产生行为不一致。

## 4. 推荐（Good）

```cpp
// 用强类型表达意图
Month month() const;
```

```cpp
// 用契约表达前置条件/后置条件
Expects(height > 0 && width > 0);
Ensures(buffer[0] == 0);
```

```cpp
// 用 span 表示区间，不用 (T*, int)
void f(gsl::span<int> xs);
```

```cpp
// 用 not_null 表示不可为空
void process(gsl::not_null<Record*> r);
```

```cpp
// 用 unique_ptr 表达所有权转移
std::unique_ptr<Shape> make_shape(...);
```

```cpp
// 参数过多时打包，降低歧义
struct SystemParams { int retry; std::chrono::milliseconds timeout; /*...*/ };
void start(SystemParams p);
```

```cpp
// 稳定 ABI: Pimpl
class widget {
 struct impl;
 std::unique_ptr<impl> p;
public:
 widget();
 ~widget();
 widget(widget&&) noexcept;
 widget& operator=(widget&&) noexcept;
};
```

## 5. 不推荐（Bad）

```cpp
// 隐式全局状态影响语义（示意）
extern bool round_up;
double round(double x); // 行为取决于 round_up
```

```cpp
// 语义不清：多个基础类型参数
void draw_rect(int, int, int, int);
draw_rect(100, 200, 100, 500);
```

```cpp
// 布尔旗标参数堆叠
set_settings(true, false, 42);
```

```cpp
// 范围表达错误
void f(int* p, int n);
// 之后在实现里做 p[i] 下标访问 => 走查时应要求改为 span
```

```cpp
// 相邻同类型参数易交换
void copy_n(T* src, T* dst, int n);
```

```cpp
// 跨 TU 复杂全局初始化依赖（静态初始化顺序灾难）
extern const X x;
const Y y = f(x);
const X x = g(y);
```

```cpp
// 返回裸指针表达所有权（禁止）
X* compute();
```

## 6. 工具化建议

- clang-tidy：关注接口层的所有权/生命周期告警（如 clang-analyzer、cppcoreguidelines-*、modernize-*）。
- Sanitizers：ASan/UBSan 对悬垂引用/越界/未定义行为覆盖效果显著。
- 代码评审规则：新增/修改公共 API 时，必须在 PR 描述中说明“所有权/异常保证/线程安全”。

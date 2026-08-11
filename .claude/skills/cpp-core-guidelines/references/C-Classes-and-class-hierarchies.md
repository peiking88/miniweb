# C: Classes and class hierarchies

## 1. 编码规范（行业经验版）

### 1.1 建模与封装（struct vs class）

#### 必须（MUST）

- **有不变量（invariant）的类型必须用 `class` + 私有数据**，并在构造函数中建立不变量。
- **对外提供的类必须最小化暴露面**：
 - 默认 `private` 数据成员；
 - 优先提供“行为”而不是“裸 get/set”；
 - 不把“调用方需要维护的不变量”丢给调用方。

#### 建议（SHOULD）

- **无不变量、仅承载数据的聚合/DTO** 才使用 `struct` 且允许 public data（例如配置、返回包）。
- **把 helper 函数放在同一命名空间**（便于 ADL、可读性、避免污染全局）。
- **类内接口在前、实现细节在后**（读代码更快）。

#### 禁止（MUST NOT）

- 禁止“定义类型 + 同语句声明变量”（可读性差，且影响前置声明/工具检查）。

---

### 1.2 资源与特殊成员函数（Rule of Zero / Three / Five）

#### 必须（MUST）

- **默认遵循 Rule of Zero**：
 - 类自己不手写析构/拷贝/移动；
 - 资源交给 RAII 成员（`std::vector`、`std::unique_ptr`、`std::string`、锁守卫、文件句柄封装等）。
- **一旦你定义/删除了任何一个特殊成员函数（dtor/copy/move），就必须显式审计并定义/删除其余相关函数（Rule of Five）**。
- **可移动类型的 move 操作应 `noexcept`**（否则容器退化为拷贝，且异常安全更难保证）。

#### 建议（SHOULD）

- 用 `=default` 明确“我需要默认语义”；用 `=delete` 明确“禁止某种语义”。
- **可拷贝/可移动类型不应有 `const` 或引用数据成员**（几乎必然导致不可赋值/不可移动或语义怪异）。

#### 禁止（MUST NOT）

- 禁止 `memset/memcpy` 复制/清零非平凡对象（会破坏不变量、指针、vptr、资源句柄）。

---

### 1.3 构造与不变量（Constructors）

#### 必须（MUST）

- **构造函数必须构造“完全有效”的对象**：构造后对象应满足类不变量。
- **构造失败要么抛异常，要么使用工厂函数返回 `expected/optional`**（团队需统一策略）。
- **成员初始化必须用 member initializer list**，避免“默认构造 + 赋值”的两阶段初始化。
- **单参数构造函数默认 `explicit`**（除非你明确要允许隐式转换）。
- **初始化顺序必须与成员声明顺序一致**（并避免误导性写法）。

#### 禁止（MUST NOT）

- 禁止在构造函数/析构函数中调用虚函数（虚派发不符合直觉，易触发未实现调用/半初始化状态）。

---

### 1.4 析构（Destructors）

#### 必须（MUST）

- **析构函数不得失败**：不得抛异常；如果确实无法释放资源只能 `terminate` 或转移为显式 `close()`/`dispose()` 返回错误码。
- **析构函数应 `noexcept`**（默认也建议保持不抛）。
- **多态基类：析构函数必须 public virtual**（或 protected non-virtual 且禁止通过基类指针删除）。

---

### 1.5 继承与多态（Class hierarchies / OOP）

#### 必须（MUST）

- **优先组合（composition）而不是继承**；只有当概念具有天然层次结构、并且需要替换/扩展行为时才建立继承层次。
- **接口与实现要分离**：接口类保持“纯抽象、无数据”；实现细节留在实现类。
- **虚函数覆写必须使用 `override`**；类层次终止点可用 `final`。
- **通过指针/引用使用多态对象**，避免对象切片（slicing）。

#### 建议（SHOULD）

- **多态类应抑制 public copy/move**：避免切片与语义不一致；若需要“拷贝多态对象”，提供 `virtual clone()`。
- **避免 `dynamic_cast`**：优先通过虚函数表达；确实需要类型导航时才使用，并明确失败语义（指针返回 nullptr / 引用抛异常）。

#### 禁止（MUST NOT）

- **禁止 `protected` 数据成员**（会破坏不变量、形成脆弱基类）；如果必须共享状态，用 `protected` 成员函数或 Pimpl/组合替代。
- **禁止给虚函数与其 override 提供不同默认参数**（默认参数是静态绑定，行为会分裂）。

---

### 1.6 运算符与转换（Overloading）

#### 必须（MUST）

- 运算符重载必须符合常规语义（例如 `+` 就是加，不得做减）。
- 对称运算符（如 `==`）优先做成非成员函数（必要时声明为 friend）。

#### 禁止（MUST NOT）

- 禁止隐式转换运算符导致“惊讶行为”；需要时用 `explicit operator T()`。

---

### 1.7 union（Tagged union）

- 优先 `std::variant`；避免裸 `union`。
- 禁止用 `union` 做 type punning（严格别名/UB 风险）。

## 2. 走查清单（Checklist）

> P0=可能崩溃/安全漏洞/资源泄漏/UB；P1=高概率逻辑错误/可维护性灾难；P2=一致性/可读性。

### P0

- 多态基类是否缺少 **virtual 析构**，却被 `unique_ptr<Base>`/`delete Base*` 删除？
- 是否存在 **对象切片**（按值传递/返回基类对象，或把派生赋给基类值对象）？
- 是否存在 **手写资源管理**（`new/delete`、裸 owning 指针成员）且没有完整 Rule of Five？
- 析构函数是否可能抛异常（显式 throw 或隐式 noexcept(false)）？
- 是否在 ctor/dtor 调用虚函数（含间接调用）？
- 是否用 `memcpy/memset` 操作非平凡对象？

### P1

- 是否存在“构造后仍需调用 init() 才可用”的半初始化对象？
- 单参构造是否缺少 `explicit` 导致隐式转换？
- 成员初始化顺序是否与声明顺序不一致，存在隐藏 bug？
- move 是否 `noexcept`，否则容器扩容可能退化为拷贝？
- 是否存在 `protected` 数据，导致派生类绕过基类校验？

### P2

- 是否过度使用 getter/setter，类只有数据没有行为？
- helper 是否放在错误命名空间，导致 ADL/可发现性差？

## 3. 推荐（Good）

```cpp
// 有不变量 -> class + 私有数据 + 构造时校验
class Date {
public:
 Date(int yy, Month mm, int dd) {
  if (!is_valid(yy, mm, dd)) throw Bad_date{};
  y_ = yy; m_ = mm; d_ = dd;
 }
 int day() const noexcept { return d_; }
 Month month() const noexcept { return m_; }
private:
 int y_{};
 Month m_{};
 int d_{};
};
```

```cpp
// Rule of Zero：用 RAII 成员管理资源，不手写特殊成员
struct FileReader {
 std::ifstream f;
 explicit FileReader(const std::string& path) : f(path) {
  if (!f) throw std::runtime_error("open failed");
 }
};
```

```cpp
// 多态接口：纯抽象 + virtual 析构 + override
struct Device {
 virtual ~Device() = default;
 virtual void write(std::span<const char>) = 0;
 virtual void read(std::span<char>) = 0;
};

struct FileDevice final : Device {
 void write(std::span<const char>) override;
 void read(std::span<char>) override;
};
```

```cpp
// 多态拷贝：clone
struct Shape {
 virtual ~Shape() = default;
 virtual std::unique_ptr<Shape> clone() const = 0;
};
```

```cpp
// 值类型一致性：=default / noexcept swap
struct Bundle {
 std::string name;
 std::vector<Record> vr;
};
inline bool operator==(const Bundle& a, const Bundle& b) noexcept {
 return a.name == b.name && a.vr == b.vr;
}
```

## 4. 不推荐（Bad）

```cpp
// P0：多态基类无 virtual 析构 -> 删除派生对象 UB/泄漏
struct Base { virtual void f(); /* no virtual dtor */ };
struct D : Base { std::string s{"need cleanup"}; ~D(){} };

void use() {
 std::unique_ptr<Base> p = std::make_unique<D>();
} // 只调用 ~Base()，~D() 不执行
```

```cpp
// P0：构造/析构调用虚函数
struct B {
 B() { f(); }     // bad
 virtual void f() = 0;
};
```

```cpp
// P1：单参构造不 explicit -> 隐式转换惊讶
struct String { String(int); /* ... */ }; // BAD
String s = 10; // surprise
```

```cpp
// P1：成员初始化顺序误导（实际按声明顺序）
class Foo {
 int m1;
 int m2;
public:
 Foo(int x) : m2{x}, m1{++x} {} // BAD
};
```

```cpp
// P1：protected data 破坏不变量
class Shape {
protected:
 Color fill_color; // bad
};
```

```cpp
// P0：memcpy/memset 操作对象
void copy(derived& a, derived& b) {
 memcpy(&a, &b, sizeof(derived)); // bad
}
```

```cpp
// P1：虚函数默认参数差异（静态绑定）
struct Base {
 virtual int multiply(int v, int factor = 2) = 0;
};
struct Derived : Base {
 int multiply(int v, int factor = 10) override;
};
```

## 5. 工具化建议

- clang-tidy（强烈建议启用并作为 PR 门禁的一部分）：
 - `cppcoreguidelines-special-member-functions`（Rule of Five/Zero）
 - `cppcoreguidelines-virtual-class-destructor`（虚析构）
 - `cppcoreguidelines-pro-type-member-init`（成员初始化）
 - `modernize-use-override`、`modernize-use-default-member-init`
 - `bugprone-virtual-near-miss`（虚函数签名近似导致未 override）
- Sanitizers：ASan/UBSan 对切片相关 UB、错误 delete、未定义行为非常敏感。
- 评审门禁建议：
 - 新增/修改“可多态删除的基类”必须在声明处写明析构策略（virtual/protected）。
 - 出现 `protected` 数据成员必须给出设计理由与替代方案评估。

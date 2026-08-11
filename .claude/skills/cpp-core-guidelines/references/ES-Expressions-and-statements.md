# ES: Expressions and statements

## 1. 编码规范（行业经验版）

### 1.1 作用域与命名

#### 必须（MUST）
- **作用域尽可能小**：变量声明放在最靠近使用处（减少误用、减少维护成本）。
- **for/if/switch 的初始化子句里声明临时变量**，避免变量泄漏到外层。

#### 禁止（MUST NOT）
- 禁止在嵌套作用域复用同名变量（shadowing），除非团队明确允许并有 lint 兜底。

---

### 1.2 初始化与对象状态

#### 必须（MUST）
- **所有对象必须初始化**（尤其是 POD/内置类型、成员变量）。
- **优先使用 `{}` 初始化**（避免窄化、避免 most-vexing-parse）。
- **默认 `const/constexpr`**：除非确实需要修改。

#### 建议（SHOULD）
- 不要“先声明后赋值”，能在声明处得到值就直接初始化（减少未初始化窗口）。

---

### 1.3 宏、常量与重复代码

#### 必须（MUST）
- **常量使用 `constexpr`/`const`/`enum class`**，不用宏。
- **避免重复代码**：重复逻辑抽函数/抽 lambda/抽工具函数。

#### 禁止（MUST NOT）
- 禁止用宏做“程序文本操作”（例如把 `!=` 定义成宏）。

---

### 1.4 表达式复杂度与求值顺序（UB 高发区）

#### 必须（MUST）
- **表达式要简单**：复杂表达式拆分成中间变量（尤其是包含副作用时）。
- **不确定优先级就加括号**（宁可啰嗦，也不要读错）。
- **禁止依赖未规定的求值顺序**（尤其是同一对象多次修改/读取的表达式）。

---

### 1.5 转换、指针与范围

#### 必须（MUST）
- **用 `nullptr` 替代 `0/NULL`**。
- **避免强转**：能不用 cast 就不用；必须 cast 时用命名 cast（`static_cast`/`reinterpret_cast`/`const_cast`）。
- **禁止窄化转换**：需要显式转换并在代码评审中说明。
- **数组/区间用 `std::span`（或 `gsl::span`）表达**，不要用裸指针做下标。

#### 禁止（MUST NOT）
- 禁止 `const_cast` 去 const（除非与遗留 API 互操作且有严格封装）。
- 禁止对无效指针解引用；禁止比较不同数组的内部指针。

---

### 1.6 控制流（if/for/switch）

#### 必须（MUST）
- **优先 range-for**（当你只需要遍历时）。
- **switch 不允许隐式 fallthrough**：必须显式标注（`[[fallthrough]]`）或加注释并有 lint。
- **避免 do-while / goto**：通常可用更清晰结构替代。

---

### 1.7 整数与下标（线上 bug 高发区）

#### 必须（MUST）
- **不得混用 signed/unsigned 做算术**（隐式提升导致 bug）。
- **下标优先使用 `gsl::index` 或 `std::ptrdiff_t`**，不要用 `unsigned` 只是为了“避免负数”。
- **禁止溢出/下溢**：对边界值做显式检查，或使用更大类型/饱和运算。

## 2. 走查清单（Checklist）

> P0=UB/越界/崩溃；P1=高概率逻辑错误；P2=可读性/一致性。

### P0
- 是否存在未初始化变量/成员？
- 是否存在窄化转换、整数溢出/下溢、除以 0？
- 是否存在依赖求值顺序的表达式（同一变量在一个语句中多次修改/读取）？
- 是否存在无效指针解引用、跨数组指针比较、裸指针下标？

### P1
- 是否存在宏替代常量/函数？是否引入命名冲突？
- 是否混用 signed/unsigned，或把 size 用 unsigned 导致负数变大正数？
- switch 是否可能 fallthrough？是否遗漏 default（或 default 用错场景）？

### P2
- 表达式是否过于复杂？是否应拆分？
- 变量作用域是否过大？是否应在 for/if 初始化处声明？

## 3. 推荐（Good）

```cpp
// 作用域小：for 初始化里声明
for (int i = 0; i < 20; ++i) {
 // ...
}

if (auto* pc = dynamic_cast<Circle*>(ps)) {
 // ...
}
```

```cpp
// 使用标准库算法
auto sum = std::accumulate(begin(a), end(a), 0.0);
```

```cpp
// 避免重复代码：先做公共动作，再分支
void func(bool flag) {
 x();
 if (flag) y();
 else z();
}
```

```cpp
// 下标使用 gsl::index（或 ptrdiff_t）
template <class T>
void print(std::ostream& os, const std::vector<T>& v) {
 for (gsl::index i = 0; i < v.size(); ++i) os << v[i] << '\n';
}
```

```cpp
// nullptr
int* p = nullptr;
```

## 4. 不推荐（Bad）

```cpp
// 宏做文本操作：灾难（示例）
#define NE !=
// ... 与 enum 值 NE 冲突
```

```cpp
// 变量作用域过大
int i;
for (i = 0; i < 20; ++i) { /* ... */ }
```

```cpp
// 裸指针做范围 + 下标（应改 span）
void f(int* p, int n) {
 p[2] = 7;
}
```

```cpp
// 不清晰的类型转换/窄化
int x = 3.14; // bad
```

## 5. 工具化建议

- clang-tidy：
 - `cppcoreguidelines-*`（初始化、转换、宏、指针等）
 - `bugprone-*`（求值顺序、可疑表达式）
 - `hicpp-*`（窄化/整数问题）
- Sanitizers：UBSan 对未定义行为/整数问题很有效，ASan 对越界与 UAF 有效。

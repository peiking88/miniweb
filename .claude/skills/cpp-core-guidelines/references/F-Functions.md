# F: Functions

## 1. 编码规范（行业经验版）

### 1.1 必须（MUST）

- **函数职责单一**：一个函数只做一件“可命名的事”。当函数同时做 IO + 解析 + 业务决策 + 错误上报，必须拆分。
- **参数/返回值语义清晰**：
 - 输入：小对象按值，大对象用 `const T&`；
 - 输出：优先返回值；多个输出用聚合结构体；
 - 可选参数：用指针或 `std::optional<T>`；
 - 不可选参数：用引用或 `not_null<T*>`。
- **禁止返回悬垂引用/指针**：不得返回局部变量地址/引用；不得返回指向临时对象的引用。
- **禁止返回 `T&&`**：返回右值引用几乎总是错误/令人困惑。
- **异常语义必须一致**：
 - 不抛异常的底层函数才标 `noexcept`；
 - `noexcept` 一旦承诺就必须长期遵守（否则 ABI/行为灾难）。
- **禁止 `va_arg`/C 风格变参**：用重载、可变模板或容器参数。

### 1.2 建议（SHOULD）

- **能 constexpr 就 constexpr**：纯计算、无副作用函数优先 `constexpr`，让更多错误在编译期暴露。
- **避免“输出参数”**：除非有明确性能/互操作约束，否则输出参数降低可读性与可组合性。
- **避免 `return std::move(local)`**：会抑制 NRVO/RVO；直接 `return local;`。
- **lambda 捕获要显式**：
 - 本地、同步使用：可按引用捕获（`[&]` 也可，但更推荐列出关键变量）；
 - 跨线程/异步/保存到堆：必须按值捕获，严禁引用捕获。
 - 成员函数内默认 `[=]` 易误捕获 `this`，建议显式 `[this]` 或 `[=, this]`（取决于标准/规则）。

### 1.3 禁止（MUST NOT）

- **禁止 `void main()`**：必须 `int main()`。
- **禁止用 `const T` 作为返回类型**：会阻止移动语义并制造困惑。

## 2. 走查清单（Checklist）

- P0：是否存在返回局部引用/指针、返回 `T&&`、返回指向临时对象的引用？
- P0：是否存在跨线程 lambda 引用捕获导致悬垂？
- P1：参数传递是否符合“值/const&/span/optional/not_null”语义？
- P1：是否存在输出参数滥用导致调用方必须先构造空容器？
- P1：是否存在错误的 `noexcept` 承诺（内部可能抛异常）？
- P2：函数是否过长/嵌套过深，是否可提取子函数（guard clause/早返回）？

## 3. 推荐（Good）

```cpp
// 单一职责拆分
int read(std::istream& in);
void print(std::ostream& out, int v);
```

```cpp
// constexpr 纯计算
constexpr int fac(int n) {
 return (n <= 1) ? 1 : n * fac(n - 1);
}
```

```cpp
// 输出用返回值（更易组合、可测试）
std::vector<const int*> find_all(const std::vector<int>& v, int x);
```

```cpp
// 多输出用结构体
struct f_result { int status; std::string data; };
f_result fetch(...);
```

```cpp
// 可选参数用指针/optional，不可选用引用/not_null
void use(Record& r);
void maybe_use(Record* r);
// or: void maybe_use(std::optional<Record> r);
```

```cpp
// 本地同步 lambda：引用捕获
auto handler = [&message](auto& socket) { socket.send(message); };
```

```cpp
// 跨线程/异步 lambda：值捕获
thread_pool.queue_work([message] { process(message); });
```

## 4. 不推荐（Bad）

```cpp
// 过长 + 多职责
void read_and_print();
```

```cpp
// 大对象按值传参（不必要拷贝）
void f2(std::string s);
```

```cpp
// 小对象反而 const&（啰嗦且可能阻碍优化）
void f4(const int& x);
```

```cpp
// 输出参数（不推荐）
void find_all(const std::vector<int>& v, std::vector<const int*>& out, int x);
```

```cpp
// 返回局部地址/引用（禁止）
int* f() {
 int x = 0;
 return &x;
}
```

```cpp
// 返回 T&&（几乎总是错误）
auto&& wrapper(F f){ return f(); }
```

```cpp
// 抑制 RVO
std::vector<int> g(){
 std::vector<int> v;
 return std::move(v); // bad
}
```

```cpp
// C 风格变参
int sum(int n, ...);
```

## 5. 工具化建议

- clang-tidy：
 - `cppcoreguidelines-avoid-c-arrays` / `cppcoreguidelines-pro-type-vararg`（变参）
 - `performance-*`（不必要拷贝）
 - `bugprone-*`（返回悬垂、捕获问题等）
- 走查门禁：公共 API 修改必须同步更新声明处注释（异常保证/线程安全/所有权）。

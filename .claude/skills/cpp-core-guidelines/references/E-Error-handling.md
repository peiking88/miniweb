# E: Error handling

## 1. 编码规范（行业经验版）

### 1.1 统一策略（先定规矩再写代码）

#### 必须（MUST）
- **在设计阶段就确定错误处理策略**：
 - 模块内是否允许异常？
 - 边界（跨库/跨线程/回调/C API）如何传递错误？
 - 哪些错误“可恢复”，哪些错误“失败即终止（fail-fast）”？
- **围绕不变量设计错误处理**：对象不变量由构造函数建立；建立不了就失败（抛异常或返回错误）。

---

### 1.2 异常的使用边界

#### 必须（MUST）
- **异常只用于错误处理**，不要用异常做正常控制流（例如找到元素就 throw）。
- **抛异常用自定义异常类型**（包含上下文信息），不要 `throw int` / `throw "..."`。
- **throw by value，catch by reference**：`catch(const MyError& e)`。
- **析构/释放/swap/异常对象的拷贝移动必须不失败**（不得抛）。

#### 建议（SHOULD）
- 在模块边界（例如线程入口、任务调度框架边界、C ABI）统一捕获并转换成错误码/日志/上报。
- 最小化 try/catch：优先依赖 RAII 自动清理。

#### 禁止（MUST NOT）
- 禁止抛出“信息为空”的 `std::exception{}` 作为业务错误。
- 禁止在持有资源所有权（裸 owning 指针、未封装句柄）时抛异常。

---

### 1.3 noexcept 的工程化规则

#### 必须（MUST）
- 只有当“抛异常不可能或不可接受”时才标 `noexcept`。
- `noexcept` 函数内部如果调用可能抛的代码，必须处理或让程序明确终止（而不是漏出异常）。

---

### 1.4 不用异常的场景（约束环境）

#### 必须（MUST）
- 在禁用异常的代码里，错误码必须系统化（统一类型/统一约定），并且依然要 RAII（或模拟 RAII）保证资源清理。
- 不要依赖全局状态（如 `errno`）作为主要错误传递方式。

## 2. 走查清单（Checklist）

> P0=泄漏/终止/未捕获异常/双重释放；P1=错误语义不一致；P2=可维护性。

### P0
- 析构函数是否可能抛异常？是否隐式 noexcept(false)？
- 是否存在 `new`/`malloc` 后在异常路径泄漏？
- 是否在持有锁/文件句柄/内存所有权时抛异常但未 RAII？
- 线程入口/回调边界是否可能让异常逃逸导致 `std::terminate`？

### P1
- 是否用异常做控制流？
- 异常类型是否贫乏（无上下文信息），或用 built-in 类型？
- catch 顺序是否正确（先派生后基类）？是否存在 catch(...) 吞异常？
- `noexcept` 承诺是否可靠？是否与实现矛盾？

### P2
- try/catch 是否过多（可用 RAII 代替）？
- 是否明确写了前置/后置条件（契约）？

## 3. 推荐（Good）

```cpp
// 构造函数建立不变量，失败就抛
class Gadget {
public:
 explicit Gadget(std::string_view arg) {
  if (!valid(arg)) throw Bad_arg{};
 }
};
```

```cpp
// RAII 防泄漏（示意）
struct Foo {
 std::vector<Thing> v;
 File_handle f;
 std::string s;
};
```

```cpp
// 无合适 RAII handle 时，用 finally 表达清理
void f(int n) {
 void* p = std::malloc(n);
 auto _ = gsl::finally([&] { std::free(p); });
 // ...
}
```

```cpp
// throw by value, catch by reference
throw My_error{"read config failed", path};

try {
 // ...
} catch (const My_error& e) {
 log(e);
}
```

## 4. 不推荐（Bad）

```cpp
// 可能泄漏：手动 new + 异常
void f1(int i) {
 int* p = new int[12];
 if (i < 17) throw Bad{"in f()", i};
 delete[] p;
}
```

```cpp
// 用异常做控制流
int find_index(std::vector<std::string>& vec, const std::string& x) {
 try {
  for (gsl::index i = 0; i < vec.size(); ++i)
   if (vec[i] == x) throw i;
 } catch (int i) {
  return i;
 }
 return -1;
}
```

```cpp
// 抛 built-in / 字符串 / 空信息异常
throw 7;
throw "something bad";
throw std::exception{};
```

## 5. 工具化建议

- clang-tidy：`bugprone-exception-escape`、`cppcoreguidelines-noexcept-*`、`cert-err*`。
- Sanitizers：LSan 对泄漏有效；ASan 对异常路径下的 UAF/越界同样有效。
- 评审门禁：
 - 在模块边界处必须明确“异常是否允许跨越边界”。
 - `noexcept` 需要说明理由，并配套测试用例覆盖异常路径。

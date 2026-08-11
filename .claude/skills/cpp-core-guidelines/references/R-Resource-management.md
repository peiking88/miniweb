# R: Resource management

## 1. 编码规范（行业经验版）

### 1.1 必须（MUST）

- **所有资源必须用 RAII 管理**：资源获取与释放绑定到对象生命周期（栈对象/成员对象）。
- **明确所有权模型**：
 - 拥有：`unique_ptr`（默认） / `shared_ptr`（确实需要共享时）
 - 观察：裸指针 `T*` / 引用 `T&`（都视为 non-owning）
- **默认禁止显式 `new/delete`**：
 - 创建对象：用 `make_unique/make_shared` 或工厂函数返回智能指针
 - 例外：
  - 性能热点且有严格审计（如对象池），需要配套单元测试 + sanitizer
  - 与 C API 互操作（必须封装到 RAII handle）
- **禁止 `malloc/free` 管理 C++ 对象**（对象生命周期/构造析构不匹配）。
- **同一表达式最多一次资源分配**：避免“分配 + 另一次分配 + 绑定”混在一行，保证异常安全。

### 1.2 建议（SHOULD）

- **优先栈对象**，避免不必要的堆分配（性能、碎片、异常安全、复杂度）。
- **接口层用 smart pointer 仅用于表达语义**：
 - `sink(unique_ptr<T>)` 表示接管所有权
 - `shared_ptr<T>` 仅在确实共享生命周期时使用
 - 普通“借用访问”用 `T*`/`T&`/`span`/`string_view`
- **避免 shared_ptr 环**：出现双向引用必须引入 `weak_ptr` 打断。

### 1.3 禁止（MUST NOT）

- **禁止 raw pointer 下标**（`p[i]`）：表示范围就用 `span`。
- **禁止“裸 owning pointer 成员”不自解释**：类中出现 `T*` 成员必须说明 owning/observing（否则默认视为 bug）。

## 2. 走查清单（Checklist）

- P0：是否存在 `new/delete`、`malloc/free`、`fopen/fclose`、`lock/unlock` 这类**成对调用**但缺乏 RAII 保护？
- P0：异常路径/早返回路径是否可能泄漏资源（内存、锁、句柄）？
- P0：是否存在 raw pointer owning（返回 owning 裸指针、类成员 owning 裸指针、把 `new` 结果交给多个地方）？
- P1：是否在接口层滥用 `shared_ptr`（本可用借用语义），导致 refcount 开销与生命周期耦合？
- P1：是否存在 shared_ptr 循环引用（无法释放）？
- P1：是否把范围错误表达成 `T* + n` 组合并做下标？

## 3. 推荐（Good）

```cpp
// RAII：锁、端口、所有权一目了然
void send(std::unique_ptr<X> x, std::string_view destination) {
 Port port{destination};
 std::lock_guard<std::mutex> guard{my_mutex};
 send(port, x);
}
```

```cpp
// 不需要共享时：unique_ptr + make_unique
auto p = std::make_unique<Foo>(7);
```

```cpp
// 需要共享时：make_shared
auto p = std::make_shared<X>(2);
```

```cpp
// 打断 shared_ptr 环
struct foo { std::shared_ptr<bar> fwd; };
struct bar { std::weak_ptr<foo> back; };
```

```cpp
// 接口语义：接管所有权 vs 借用
void sink(std::unique_ptr<widget>); // takes ownership
void uses(widget*);         // borrows
```

```cpp
// 用 span 表示数组/缓冲区
void f(gsl::span<int> xs);
```

## 4. 不推荐（Bad）

```cpp
// 手动管理资源：异常/早返回容易泄漏
void send(X* x, std::string_view destination) {
 auto port = open_port(destination);
 my_mutex.lock();
 send(port, x);
 my_mutex.unlock();
 close_port(port);
 delete x;
}
```

```cpp
// raw pointer 下标：范围语义不清
void f(int* p, int n) {
 p[2] = 7; // bad
}
```

```cpp
// 返回 owning 裸指针：调用方必须记得 delete
Gadget* make_gadget(int n) {
 return new Gadget{n};
}
```

```cpp
// malloc/free 管理 C++ 对象（禁止）
Record* p = static_cast<Record*>(malloc(sizeof(Record)));
// ...
free(p);
```

```cpp
// 只在本地使用却用 shared_ptr：refcount 开销 + 语义误导
void f() {
 std::shared_ptr<Base> base = std::make_shared<Derived>();
}
```

## 5. 工具化建议

- clang-tidy：`cppcoreguidelines-owning-memory`、`cppcoreguidelines-no-malloc`、`modernize-make-unique`、`modernize-make-shared`。
- Sanitizers：ASan/LSan 对泄漏与 use-after-free 非常有效；TSan 对锁/数据竞争。
- 评审门禁：出现 `new/delete/malloc/free` 必须在 PR 里写清“为何例外 + 如何验证（测试/sanitizer）”。

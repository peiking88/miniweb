# CP: Concurrency and parallelism

## 1. 编码规范（行业经验版）

### 1.1 基本原则

#### 必须（MUST）
- **假设你的代码会在多线程程序中运行**：默认要考虑可重入、共享状态与并发访问。
- **避免数据竞争（data race）**：共享可写数据必须用互斥或原子同步；否则就是 UB。
- **最小化共享可写数据**：优先不可变数据、消息传递、任务返回值。

---

### 1.2 锁与临界区（Lock discipline）

#### 必须（MUST）
- **只用 RAII 加锁**：`std::lock_guard` / `std::unique_lock` / `std::scoped_lock`，禁止裸 `lock()/unlock()`。
- **多把锁按统一顺序获取**，或用 `std::scoped_lock`/`std::lock` 一次性获取以避免死锁。
- **持锁期间不得调用未知代码**（回调、虚函数、可重入函数、用户提供的 functor）。
- **临界区要短**：临界区内不做 IO/阻塞等待/长时间计算。

#### 禁止（MUST NOT）
- 禁止用 `volatile` 做同步（它不提供线程间可见性与原子性保证）。

---

### 1.3 线程生命周期（Thread lifetime）

#### 必须（MUST）
- **线程必须可 join**：把 joining-thread 看作“作用域容器”。
- **禁止 `detach()`**（除非极少数后台线程且有完整生命周期管理与进程退出策略）。
- **跨线程传参默认按值传递**：避免引用捕获导致悬垂。

---

### 1.4 任务化（Tasks over threads）

#### 必须（MUST）
- **优先任务（`std::async`/线程池）而不是直接 `std::thread`**：减少线程创建销毁、降低上下文切换。
- 并发任务返回值用 `future`（或框架提供的 task handle），明确 join/等待点。

---

### 1.5 条件变量与等待

#### 必须（MUST）
- **等待必须带条件**：`cv.wait(lock, pred)`，禁止无条件 wait。

---

### 1.6 无锁编程与双重检查锁

#### 必须（MUST）
- **除非迫不得已，不写 lock-free**（复杂度与可验证性成本极高）。
- **禁止自写 double-checked locking**；需要延迟初始化时用 `std::call_once`/静态局部变量等成熟模式。

---

### 1.7 协程（如使用）

#### 必须（MUST）
- **禁止在 suspension point 跨越锁/同步原语**（挂起时锁无法正确表达生命周期，易死锁/阻塞）。
- 协程参数不要按引用跨越挂起点（避免悬垂）。

## 2. 走查清单（Checklist）

> P0=数据竞争/死锁/悬垂/terminate；P1=性能与可维护性风险；P2=一致性。

### P0
- 是否存在共享可写数据未加锁/未用原子（data race）？
- 是否存在 `lock()/unlock()` 手动配对（异常/早返回导致未解锁）？
- 是否存在多锁顺序不一致导致潜在死锁？
- 是否在持锁调用回调/虚函数/用户函数（未知代码）？
- 是否 `detach()` 导致线程访问已销毁对象（悬垂）？
- 线程入口/任务体是否可能抛异常导致 `std::terminate`（未捕获异常逃逸）？

### P1
- 是否过度创建线程、上下文切换过多？是否应改任务/线程池？
- 临界区是否过长（IO/阻塞/重计算）？
- `condition_variable` 是否使用了无条件 wait？是否存在丢信号/虚假唤醒处理不当？

### P2
- 锁是否与数据“就地绑定”（mutex 与受保护数据放一起）？
- 锁 guard 是否命名清晰，便于审阅？

## 3. 推荐（Good）

```cpp
// 任务化并发：async + future
void process_readings(const std::vector<Reading>& rs) {
 auto h1 = std::async([&] { if (!validate(rs)) throw Invalid_data{}; });
 auto h2 = std::async([&] { return temperature_gradients(rs); });
 auto h3 = std::async([&] { return altitude_map(rs); });

 h1.get();
 auto v2 = h2.get();
 auto v3 = h3.get();
}
```

```cpp
// 多锁：scoped_lock 避免死锁
std::scoped_lock lk(m1, m2);
```

```cpp
// RAII 锁：禁止手动 lock/unlock
void do_this(Foo* p) {
 std::lock_guard<std::mutex> lck{my_mutex};
 // ...
}
```

```cpp
// joining_thread：线程生命周期绑定作用域（GSL）
joining_thread t0(f, &x);
```

## 4. 不推荐（Bad）

```cpp
// 多锁顺序不一致 => 死锁风险
// thread 1
lock_guard<mutex> lck1(m1);
lock_guard<mutex> lck2(m2);

// thread 2
lock_guard<mutex> lck2(m2);
lock_guard<mutex> lck1(m1);
```

```cpp
// 持锁调用未知代码（回调）=> 死锁/重入/性能问题
void do_something(Action f) {
 std::unique_lock<std::recursive_mutex> lck{my_mutex};
 f(this); // bad
}
```

```cpp
// detach：生命周期失控
void use() {
 std::thread t(heartbeat);
 t.detach(); // bad
}
```

```cpp
// 用 volatile 做同步（错误）
volatile bool ready = false; // bad
```

## 5. 工具化建议

- 静态/动态工具：TSan（线程数据竞争）、helgrind（如使用 Valgrind）。
- clang-tidy：`cppcoreguidelines-avoid-const-or-ref-data-members`（跨线程对象语义）、并发相关检查项。
- 评审门禁：
 - 新增共享状态必须写清“谁拥有、谁修改、用什么同步”。
 - 出现 `detach()` 必须有架构级理由与退出策略。

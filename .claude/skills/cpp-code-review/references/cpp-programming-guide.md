# C++ 编程规范与实践经验

> 来源：C++ 编程规范、*Effective Modern C++*（Scott Meyers 著）

## 目录

- [第一部分：C++ 编程规范](#第一部分c-编程规范)
  - [文件结构](#文件结构)
  - [命名规范](#命名规范)
  - [代码格式](#代码格式)
  - [表达式与语句](#表达式与语句)
  - [常量与宏](#常量与宏)
  - [函数设计](#函数设计)
  - [内存管理](#内存管理)
  - [类与对象](#类与对象)
  - [并发编程](#并发编程)
  - [代码审查表](#代码审查表)
- [第二部分：现代C++](#第二部分现代-c++)
  - [类型推导](#类型推导)
  - [auto](#auto)
  - [移步现代C++](#移步现代c)
  - [智能指针](#智能指针)
  - [右值引用、移动语义与完美转发](#右值引用移动语义与完美转发)
  - [Lambda表达式](#lambda表达式)
  - [并发API](#并发api)
  - [微调](#微调)
  - [开发经验（最佳实践）](#开发经验最佳实践)
  - [开发教训（常见陷阱）](#开发教训常见陷阱)

---

## 第一部分：C++ 编程规范

## 文件结构

### 头文件
- 头文件使用 `#ifndef`/`#define`/`#endif` 预处理块或 `#pragma once`
- 头文件和定义文件名称合理，目录结构清晰
- 包含版权和版本声明
- 只声明接口，不暴露实现细节

### 代码组织
```cpp
// 头文件标准结构
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

// 1. 系统头文件
#include <string>
#include <vector>

// 2. 项目头文件
#include "my_module.h"

// 3. 常量定义
constexpr int MAX_SIZE = 1024;

// 4. 类型定义/前置声明
class MyClass;

// 5. 类声明
class MyClass {
public:
    // 构造/析构
    // 接口方法
protected:
    // 保护成员
private:
    // 私有成员
};

#endif
```

### 检查要求
- [ ] 头文件是否有include guard
- [ ] 文件命名是否合理
- [ ] 目录结构是否清晰
- [ ] 版权和版本声明是否完整

---

## 命名规范

### 基本规则
- 标识符直观且可拼读
- 长度遵循 `min-length && max-information`
- 不出现同名局部变量和全局变量
- 类名、函数名、变量、参数、常量遵循统一格式

### 推荐风格
| 类型 | 风格 | 示例 |
|------|------|------|
| 类名 | PascalCase | `MyClass` |
| 函数名 | camelCase / snake_case | `getData()` / `get_data()` |
| 变量名 | camelCase / snake_case | `userName` / `user_name` |
| 常量 | UPPER_SNAKE_CASE | `MAX_BUFFER_SIZE` |
| 宏 | UPPER_SNAKE_CASE | `DEBUG_MODE` |
| 成员变量 | 带前缀 `m_` | `m_data` |
| 静态变量 | 带前缀 `s_` | `s_instance` |
| 全局变量 | 带前缀 `g_` | `g_config` |

### 检查要求
- [ ] 命名风格是否一致
- [ ] 是否使用有意义的名称
- [ ] 静态/全局/成员变量是否有前缀区分

---

## 代码格式

### 基本规则
- 一行代码只做一件事（只定义一个变量，只写一条语句）
- `if`/`for`/`while`/`do` 等语句自占一行，不论执行语句多少都加大括号
- 代码行内空格得体
- 修饰符 `*` 和 `&` 紧靠变量名

### 示例
```cpp
// 推荐
int* ptr = &value;
const std::string& name = getName();

// 不推荐
int *ptr = &value;
const std::string & name = getName();
```

---

## 表达式与语句

### 运算符与优先级
- 运算符较多时用括号明确操作顺序
- 不编写过于复杂或多用途的复合表达式

### 浮点比较
- 浮点变量不直接与 TRUE/FALSE 或 1/0 比较
- 使用精度容差比较

### 指针比较
- 指针变量不直接与 TRUE/FALSE 比较
- 使用 `nullptr` 而非 `NULL` 或 `0`

### 循环优化
- 循环体内有逻辑判断且循环次数大时，将判断移到循环外
- `case` 语句结尾必须加 `break`
- `switch` 必须有 `default` 分支
- 不随意使用 `goto` 语句

### 检查要求
- [ ] 复杂表达式是否有括号
- [ ] 浮点比较是否正确
- [ ] switch是否有default
- [ ] 是否有goto语句

---

## 常量与宏

### 规则
- 使用含义直观的常量表示多次出现的数字或字符串
- C++中用 `const`/`constexpr` 常量取代宏常量
- 关联常量在定义中体现关系
- 注意 `const` 数据成员只在对象生存期内是常量，对整个类是可变的

### 禁止
```cpp
// 禁止
#define BUFFER_SIZE 1024
#define PI 3.14159
```

### 推荐
```cpp
// 推荐
constexpr size_t BUFFER_SIZE = 1024;
constexpr double PI = 3.14159;
```

### 检查要求
- [ ] 是否用constexpr/const替代宏
- [ ] 是否有magic number
- [ ] 关联常量是否体现关系

---

## 函数设计

### 参数规则
- 参数书写完整（不省略参数名）
- 参数命名和顺序合理
- 参数个数不宜过多（建议不超过5个）
- 避免使用类型和数目不确定的参数

### 返回值
- 不省略返回值类型
- 函数名与返回值语义不冲突
- 正常值用输出参数，错误标志用return
- 使用 `assert` 检查参数有效性（不滥用）
- 不返回指向栈内存的指针

### const正确性
- 使用 `const` 提高函数健壮性
- `const` 保护参数、返回值、函数体
- 原则：Use const whenever you need

### 检查要求
- [ ] 参数是否完整且合理
- [ ] 返回值类型是否正确
- [ ] 是否使用const保护
- [ ] 是否有assert检查

---

## 内存管理

### 初始化
- 数组和动态内存必须赋初值
- 防止将未初始化的内存作为右值使用

### 边界安全
- 数组或指针下标不越界
- 动态内存申请与释放配对（防内存泄漏）
- `free`/`delete` 后将指针置为 `NULL`

### 分配规则
- 不混用 `malloc`/`free` 与 `new`/`delete`
- `malloc` 语句字节数和类型转换正确
- `new`/`delete` 数组形式正确（`new[]` 配 `delete[]`）

### RAII原则
- 使用RAII管理资源
- 智能指针优先：`std::unique_ptr`、`std::shared_ptr`
- 使用 `std::vector`、`std::string` 替代手动内存管理

### 检查要求
- [ ] 是否有未初始化内存
- [ ] 是否有数组越界
- [ ] new/delete是否配对
- [ ] delete后是否置空
- [ ] 是否混用malloc/free和new/delete
- [ ] 是否使用RAII

---

## 类与对象

### 结构顺序
- `public` → `protected` → `private` 顺序一致
- 构造函数使用初始化列表
- 成员初始化顺序与声明顺序一致
- 虚析构函数（有多态时）

### 构造与析构
- 构造函数只做初始化，不做复杂逻辑
- 析构函数释放所有资源
- 禁止在构造/析构中调用虚函数

### 检查要求
- [ ] 类成员顺序是否一致
- [ ] 是否使用初始化列表
- [ ] 多态类是否有虚析构
- [ ] 构造/析构是否调用虚函数

---

## 并发编程

### 互斥与锁
- 使用 `std::lock_guard` / `std::unique_lock` 管理锁
- 锁粒度最小化：只锁必要代码
- 不持锁做耗时操作（IO、sleep）
- 避免嵌套锁（防死锁）
- 如需多锁，使用 `std::lock()` 一次获取

### 线程安全
- 共享数据必须保护
- 注意 `mutable` 成员的线程安全
- `recursive_mutex` 谨慎使用（通常说明设计有问题）
- 使用 `std::atomic` 进行原子操作

### 示例
```cpp
// 推荐：最小锁粒度
void process() {
    Data data;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        data = m_sharedData;
    }
    doWork(data);
}

// 禁止：持锁做IO
void bad_process() {
    std::lock_guard<std::mutex> lock(m_mutex);
    doWork(m_sharedData);
    writeFile();  // 持锁IO，危险
}
```

### 检查要求
- [ ] 共享数据是否有锁保护
- [ ] 锁粒度是否最小
- [ ] 是否有死锁风险
- [ ] 是否持锁做IO

---

## 代码审查表

### 文件与格式
- [ ] 头文件名称是否合理？
- [ ] 目录结构是否合理？
- [ ] 版权和版本声明是否完整？
- [ ] 是否使用 `#ifndef`/`#define`/`#endif` 预处理块？
- [ ] 代码行内空格是否得体？
- [ ] 一行代码是否只做一件事？
- [ ] `if`/`for`/`while`/`do` 是否自占一行且加大括号？
- [ ] 修饰符 `*` 和 `&` 是否紧靠变量名？

### 注释
- [ ] 注释是否清晰并且必要？
- [ ] 注释是否有错误或可能导致误解？

### 命名
- [ ] 标识符是否直观且可以拼读？
- [ ] 标识符长度是否符合 `min-length && max-information`？
- [ ] 是否出现同名局部变量和全局变量？
- [ ] 类名、函数名、变量和参数、常量书写格式是否统一？
- [ ] 静态变量、全局变量、类的成员变量是否加前缀？

### 表达式
- [ ] 运算符较多时是否用括号明确顺序？
- [ ] 是否有过于复杂的复合表达式？
- [ ] 浮点比较是否正确？
- [ ] `switch` 是否有 `default`？
- [ ] `case` 结尾是否有 `break`？
- [ ] 是否有 `goto` 语句？

### 常量
- [ ] 是否用 `const`/`constexpr` 替代宏常量？
- [ ] 是否有未定义的 magic number？

### 函数
- [ ] 参数书写是否完整？
- [ ] 参数个数是否太多？
- [ ] 返回值类型是否正确？
- [ ] 是否使用 `const` 提高健壮性？
- [ ] 是否使用 `assert` 检查参数有效性？

### 内存
- [ ] 数组和动态内存是否赋初值？
- [ ] 数组或指针下标是否越界？
- [ ] 动态内存申请与释放是否配对？
- [ ] `delete` 后是否置 `nullptr`？
- [ ] 是否混用 `malloc`/`free` 和 `new`/`delete`？

### 类
- [ ] `public`/`protected`/`private` 顺序是否一致？
- [ ] 是否使用初始化列表？
- [ ] 多态类是否有虚析构函数？

### 错误处理
- [ ] 错误处理程序块本身是否正确？
- [ ] 是否正确关闭文件和释放资源？
- [ ] 是否有误差累积问题？
- [ ] 变量精度是否足够？

### 参考资源
- [360 Safe Rules](https://github.com/Qihoo360/safe-rules/blob/main/c-cpp-rules.md)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- *C++ Concurrency In Action - Second Edition*

---

## 第二部分：现代C++

---

## 第一章 类型推导

### Item 1: 理解模板类型推导

**核心知识点：**
模板类型推导有三种情况：
1. **ParamType是指针或引用（非通用引用）**：忽略expr的引用部分，剩余部分决定T，然后与ParamType匹配
2. **ParamType是通用引用（T&&）**：左值推导为左值引用，右值使用情景一的规则
3. **ParamType既不是指针也不是引用（传值）**：忽略引用和const/volatile

**关键要点：**
- 数组实参会退化为指针，除非用于初始化引用
- 函数实参也会退化为函数指针
- 可以利用模板推导数组大小：`template<typename T, std::size_t N> constexpr std::size_t arraySize(T (&)[N]) noexcept`

### Item 2: 理解auto类型推导

**核心知识点：**
- auto类型推导基本与模板类型推导相同，有一个例外
- **例外**：auto使用花括号初始化时推导为`std::initializer_list`，而模板不会
- `auto x = {27}` → `std::initializer_list<int>`
- C++14中auto用于函数返回值和lambda形参时，使用模板类型推导规则（非auto规则）

**开发教训：**
```cpp
auto x3 = {27};  // std::initializer_list<int>，不是int！
auto x4{27};     // 同上
```

### Item 3: 理解decltype

**核心知识点：**
- decltype通常不加修改地产出变量或表达式的类型
- 对于T类型的左值表达式，decltype产出T&（引用）
- C++14支持`decltype(auto)`，使用decltype规则推导

**关键用法：**
```cpp
// C++11 尾置返回类型
template<typename Container, typename Index>
auto authAndAccess(Container& c, Index i) -> decltype(c[i]);

// C++14 decltype(auto)
template<typename Container, typename Index>
decltype(auto) authAndAccess(Container&& c, Index i);
```

**开发教训：**
```cpp
decltype(auto) f2() {
    int x = 0;
    return (x);  // 返回int&，引用局部变量！未定义行为
}
```

### Item 4: 学会查看类型推导结果

**四种方法：**
1. IDE编辑器：鼠标悬停查看类型
2. 编译器诊断：声明但不定义的模板类`TD<T>`触发错误
3. 运行时输出：`typeid(T).name()`（不可靠）
4. Boost.TypeIndex：`boost::typeindex::type_id_with_cvr<T>()`（最可靠）

**开发教训：**
- `std::type_info::name()`不保证准确，会忽略引用和const
- IDE显示的类型信息也不总是可靠的

---

## 第二章 auto

### Item 5: 优先考虑auto而非显式类型声明

**核心知识点：**
- auto变量必须初始化，避免未初始化变量
- auto可以表示只有编译器才知道的类型（如lambda闭包）
- auto避免类型不匹配导致的性能问题和隐蔽bug
- auto避免`std::unordered_map`遍历时的类型不匹配（key是const的）

**关键要点：**
```cpp
// auto存储闭包比std::function更高效
auto derefUPLess = [](const auto& p1, const auto& p2) { return *p1 < *p2; };

// 避免类型不匹配
std::unordered_map<std::string, int> m;
for (const auto& p : m) { /* p类型正确 */ }
// vs
for (const std::pair<std::string, int>& p : m) { /* 类型不匹配！产生临时对象 */ }
```

### Item 6: auto推导若非己愿，使用显式类型初始化惯用法

**核心知识点：**
- 代理类（proxy class）可能使auto推导出"错误"类型
- 显式类型初始化惯用法：`auto var = static_cast<desired_type>(expression)`

**开发教训：**
```cpp
std::vector<bool> features(const Widget& w);
auto val = features(w)[5];  // val类型是std::vector<bool>::reference，不是bool！
// 悬空指针风险

auto val = static_cast<bool>(features(w)[5]);  // 正确
```

---

## 第三章 移步现代C++

### Item 7: 区别使用()和{}创建对象

**核心知识点：**
- 括号初始化（花括号）是最广泛使用的初始化语法
- 花括号防止隐式变窄转换
- 花括号免疫C++最令人头疼的解析（most vexing parse）
- **陷阱**：花括号初始化与`std::initializer_list`构造函数的优先匹配

**开发教训：**
```cpp
std::vector<int> v1(10, 20);  // 10个元素，值为20
std::vector<int> v2{10, 20};  // 2个元素，值为10和20
// 完全不同的结果！
```

### Item 8: 优先考虑nullptr而非0和NULL

**核心知识点：**
- 0和NULL都不是指针类型，在重载决议中可能匹配int而非指针
- nullptr的类型是`std::nullptr_t`，可隐式转换为所有原始指针类型
- nullptr不会导致模板类型推导错误

### Item 9: 优先考虑别名声明而非typedefs

**核心知识点：**
- 别名声明（using）支持模板化（alias templates），typedef不支持
- 别名模板避免`::type`后缀和`typename`前缀
- C++14提供`_t`后缀的类型转换别名：`std::remove_const_t<T>`

### Item 10: 优先考虑限域枚举而非未限域枚举

**核心知识点：**
- 限域枚举（enum class/struct）避免命名空间污染
- 限域枚举的枚举名是强类型，不接受隐式转换
- 限域枚举可以前置声明，基础类型默认为int
- 非限域枚举适用于tuple字段访问

### Item 11: 优先考虑使用deleted函数而非使用未定义的私有声明

**核心知识点：**
- delete函数应为public（编译器先检查访问权限再检查delete状态）
- 任何函数都可以delete，包括非成员函数和模板实例
- 可以delete特定模板实例化来限制接受的指针类型

```cpp
template<typename T>
void processPointer(T* ptr);

template<>
void processPointer<void>(void*) = delete;  // 禁止void*
```

### Item 12: 使用override声明重载函数

**核心知识点：**
- 重写条件：基类virtual、函数名相同、参数相同、const相同、引用限定符相同、返回值兼容
- override关键字让编译器检查是否真正重写
- 成员函数引用限定（&/&&）可以区分左值和右值对象的调用

### Item 13: 优先考虑const_iterator而非iterator

**核心知识点：**
- C++11提供cbegin/cend成员函数
- C++11的insert/emplace等接受const_iterator
- 最通用的代码应使用非成员函数cbegin

### Item 14: 如果函数不抛出异常请使用noexcept

**核心知识点：**
- noexcept是函数接口的一部分
- noexcept函数更容易被优化
- noexcept对移动语义、swap、内存释放和析构函数非常有用
- 大多数函数是异常中立的（可能抛异常），不应盲目加noexcept
- 条件noexcept：`noexcept(noexcept(expr))`

### Item 15: 尽可能的使用constexpr

**核心知识点：**
- constexpr对象是const且编译期可知
- constexpr函数：编译期可知的实参产编译期结果，运行时值产运行时结果
- C++11限制：constexpr函数只能有一行return（可用三元运算符和递归）
- C++14放宽限制：允许局部变量、循环等
- constexpr是接口的一部分，后续移除会破坏客户端代码

```cpp
constexpr int pow(int base, int exp) noexcept {  // C++14
    auto result = 1;
    for (int i = 0; i < exp; ++i) result *= base;
    return result;
}
std::array<int, pow(3, 5)> results;  // 编译期计算
```

### Item 16: 让const成员函数线程安全

**核心知识点：**
- const成员函数如果修改mutable成员（缓存），需要线程安全
- 单变量同步：`std::atomic`
- 多变量同步：必须使用`std::mutex`
- `std::atomic`和`std::mutex`都是move-only类型

**开发教训：**
```cpp
// 错误：两个atomic变量不能保证一致性
mutable std::atomic<bool> cacheValid{false};
mutable std::atomic<int> cachedValue;
// cacheValid = true; cachedValue = val1 + val2; // 中间状态可能被其他线程看到

// 正确：使用mutex保护多个变量
mutable std::mutex m;
mutable int cachedValue;
mutable bool cacheValid{false};
```

### Item 17: 理解特殊成员函数的生成

**核心知识点：**
- 移动操作仅在没有用户声明的拷贝操作、移动操作、析构时自动生成
- 拷贝构造仅在没有用户声明的拷贝构造时生成；如果声明了移动操作，拷贝构造=delete
- 声明析构函数阻止移动操作生成，但不阻止拷贝操作
- 成员函数模板不阻止特殊成员函数生成

**开发经验：**
- 声明析构函数（如添加日志）会阻止移动操作生成→性能回退
- 应显式声明`= default`以明确意图

---

## 第四章 智能指针

### Item 18: 对于独占资源使用std::unique_ptr

**核心知识点：**
- 默认与原始指针大小相同，零开销
- 支持自定义删除器（lambda最佳，无状态lambda不增加大小）
- 可以转换为`std::shared_ptr`
- 工厂函数返回`std::unique_ptr`是最佳实践

```cpp
auto delInvmt = [](Investment* p) {
    makeLogEntry(p);
    delete p;
};
std::unique_ptr<Investment, decltype(delInvmt)> pInv(nullptr, delInvmt);
```

### Item 19: 对于共享资源使用std::shared_ptr

**核心知识点：**
- 大小是原始指针的两倍（原始指针+控制块指针）
- 控制块包含引用计数（原子操作）、弱引用计数、自定义删除器、分配器
- 引用计数的原子操作有性能开销
- 避免从原始指针变量创建`std::shared_ptr`（会导致多个控制块）

**开发教训：**
```cpp
auto pw = new Widget;  // 不要这样做
std::shared_ptr<Widget> spw1(pw);
std::shared_ptr<Widget> spw2(pw);  // 两个控制块！double-free！
```

### Item 20: 当std::shared_ptr可能悬空时使用std::weak_ptr

**核心知识点：**
- `std::weak_ptr`不增加引用计数，可检测悬空
- 访问方式：`lock()`返回`std::shared_ptr`（过期时为空）或构造`std::shared_ptr`（过期时抛异常）
- 典型用途：缓存、观察者列表、打破`std::shared_ptr`循环引用

### Item 21: 优先考虑使用std::make_unique和std::make_shared而非new

**核心知识点：**
- 避免重复写类型
- 异常安全：防止new和智能指针构造之间的异常导致泄漏
- `std::make_shared`一次分配同时容纳对象和控制块（性能优化）

**限制（不能使用make函数的场景）：**
- 需要自定义删除器
- 需要使用花括号初始化
- 自定义内存对齐的类

### Item 22: 当使用Pimpl惯用法，请在实现文件中定义特殊成员函数

**核心知识点：**
- Pimpl通过隐藏实现减少编译依赖
- 使用`std::unique_ptr`时，析构函数必须在实现文件中定义（Impl为完成类型时）
- 移动操作也需要在实现文件中定义
- 拷贝操作需要手动实现（`std::unique_ptr`不可拷贝）
- 使用`std::shared_ptr`不需要这些约束

```cpp
// widget.h
class Widget {
public:
    Widget();
    ~Widget();  // 仅声明
    Widget(Widget&& rhs);  // 仅声明
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

// widget.cpp
struct Widget::Impl { /* ... */ };
Widget::Widget() : pImpl(std::make_unique<Impl>()) {}
Widget::~Widget() = default;  // 在Impl完成类型后定义
Widget::Widget(Widget&& rhs) = default;
```

---

## 右值引用、移动语义与完美转发

### Item 23: 理解std::move和std::forward

**核心知识点：**
- `std::move`不移动任何东西，只做无条件到右值的转换
- `std::forward`只在特定条件下转换（参数绑定到右值时）
- 两者在运行期不产生任何可执行代码

### Item 24: 区别通用引用和右值引用

**核心知识点：**
- T&&有两种含义：右值引用或通用引用
- 通用引用条件：类型推导发生 + 精确的T&&格式
- `auto&&`也是通用引用
- const修饰会排除通用引用资格：`const T&&`是右值引用

**判断方法：**
```cpp
void f(Widget&& param);         // 右值引用
template<typename T> void f(T&& param);  // 通用引用
auto&& var2 = var1;             // 通用引用
template<typename T> void f(std::vector<T>&& param);  // 右值引用
```

### Item 25: 对右值引用使用std::move，对通用引用使用std::forward

**核心知识点：**
- 右值引用→`std::move`（无条件转换）
- 通用引用→`std::forward`（有条件转换）
- 仅在最后一次使用时转换
- 按值返回时对右值引用/通用引用使用`std::move`或`std::forward`
- 不要对局部变量（RVO候选者）使用`std::move`

**开发教训：**
```cpp
// 错误：对通用引用使用std::move
template<typename T>
void setName(T&& newName) { name = std::move(newName); }  // 可能意外移动左值！

// 正确：
template<typename T>
void setName(T&& newName) { name = std::forward<T>(newName); }
```

### Item 26: 避免重载通用引用

**核心知识点：**
- 通用引用是最贪婪的匹配，会匹配几乎所有类型
- 完美转发构造函数会劫持拷贝构造（non-const左值匹配转发构造而非拷贝构造）
- 派生类的拷贝/移动操作会调用基类的完美转发构造而非拷贝/移动构造

### Item 27: 熟悉通用引用重载的替代方法

**四种替代方案：**
1. **放弃重载**：使用不同的函数名
2. **Pass by const T&**：退回C++98
3. **Pass by value**：按值传递（参考Item 41）
4. **Tag dispatch**：添加标签类型参数控制重载选择
5. **约束模板**：使用`std::enable_if`限制通用引用的匹配条件

### Item 28: 理解引用折叠

**核心知识点：**
- 引用折叠规则：任一为左值引用→左值引用；全为右值引用→右值引用
- 引用折叠发生的四种场景：模板实例化、auto类型推导、typedef/using、decltype
- 通用引用的本质是类型推导+引用折叠

### Item 29: 认识移动操作的缺点

**核心知识点：**
移动操作不一定比复制快，以下情况移动无优势：
- **No move operations**：类没有提供移动操作
- **Move not faster**：移动不比复制快（如`std::array`的移动是线性的）
- **Move not usable**：移动操作未声明noexcept，上下文要求noexcept时退回复制
- **Source object is lvalue**：源对象是左值

**开发教训：**
- `std::array`移动：O(n)，因为数据存储在对象内部而非堆上
- SSO（Small String Optimization）：短字符串移动不比复制快

### Item 30: 熟悉完美转发失败的情况

**完美转发失败的场景：**
1. **花括号初始化器**：`fwd({1,2,3})`无法推导为`std::initializer_list`
2. **0或NULL作为空指针**：推导为int而非指针
3. **仅声明的整数static const数据成员**：需要取地址但无定义
4. **重载函数名或模板名**：无单一函数可匹配
5. **位域**：无法取地址

```cpp
// 花括号解决方案
auto il = {1, 2, 3};  // auto可以推导
fwd(il);              // 转发成功
```

---

## Lambda表达式

### Item 31: 避免使用默认捕获模式

**核心知识点：**
- `[=]`默认按值捕获对this指针是隐式捕获（非成员变量的副本）
- `[&]`默认按引用捕获可能导致悬空引用
- 静态变量不会被捕获，但可以在lambda中使用

**开发教训：**
```cpp
class Widget {
    void addFilter() const {
        // [=] 捕获了this指针，不是divisor的副本！
        filters.emplace_back(
            [=](int value) { return value % divisor == 0; }  // 危险：this可能悬空
        );
    }
    int divisor;
};

// C++14 正确做法：初始化捕获
void addFilter() const {
    filters.emplace_back(
        [divisor = divisor](int value) { return value % divisor == 0; }
    );
}
```

### Item 32: 使用初始化捕获来移动对象到闭包中

**核心知识点：**
- C++14初始化捕获支持移动捕获
- C++11可通过`std::bind`模拟移动捕获

```cpp
// C++14
auto func = [pw = std::move(pw)] { return pw->isValidated(); };

// C++11 模拟
auto func = std::bind([](const std::unique_ptr<Widget>& pw) { /* ... */ },
                       std::move(pw));
```

### Item 33: 对std::forward的auto&&形参使用decltype

**核心知识点：**
- C++14 lambda的auto&&参数是通用引用
- 完美转发lambda参数：`std::forward<decltype(param)>(param)`

```cpp
auto f = [](auto&&... params) {
    return func(std::forward<decltype(params)>(params)...);
};
```

### Item 34: 优先考虑lambda表达式而非std::bind

**核心知识点：**
- lambda更易读、更具表达力、可能更高效（内联）
- std::bind的参数求值时机不直观（在bind时求值，非调用时）
- std::bind与重载函数配合需要函数指针转换
- C++11中std::bind仅在移动捕获和多态函数对象场景有用

---

## 第七章 并发API

### Item 35: 优先考虑基于任务的编程而非基于线程的编程

**核心知识点：**
- `std::async`（基于任务）比`std::thread`（基于线程）更优
- 基于任务可以直接获取返回值和异常
- 基于线程无法直接返回结果
- std::thread可能因线程资源限制抛出异常

### Item 36: 如果有异步的必要请指定std::launch::async

**核心知识点：**
- `std::async`默认启动策略是`async | deferred`（可能异步也可能延迟）
- 延迟任务在`get()`或`wait()`时才执行
- 默认策略的问题：无法预测是否并发、可能不执行、影响wait_for逻辑

### Item 37: 从各个方面使得std::threads unjoinable

**核心知识点：**
- joinable的std::thread析构会终止程序
- 隐式join导致难以调试的性能异常
- 隐式detach导致难以调试的未定义行为
- RAII类`ThreadRAII`确保所有路径上thread最终unjoinable

```cpp
class ThreadRAII {
public:
    enum class DtorAction { join, detach };
    ThreadRAII(std::thread&& t, DtorAction a) : action(a), t(std::move(t)) {}
    ~ThreadRAII() {
        if (t.joinable()) {
            if (action == DtorAction::join) t.join();
            else t.detach();
        }
    }
    ThreadRAII(ThreadRAII&&) = default;
    ThreadRAII& operator=(ThreadRAII&&) = default;
    std::thread& get() { return t; }
private:
    DtorAction action;
    std::thread t;
};
```

### Item 38: 关注不同线程句柄析构行为

**核心知识点：**
- `std::thread`析构：joinable→终止程序
- `std::future`析构通常只销毁future自身（不join不detach）
- **例外**：`std::async`创建的最后一个future会在异步任务完成前block（隐式join）
- `std::packaged_task`的future析构不会block

### Item 39: 考虑对于单次事件通信使用void future

**三种方案对比：**

| 方案 | 互斥锁 | 挂起线程 | 可重用 |
|------|--------|---------|--------|
| 条件变量 | 需要 | 是 | 是 |
| 标志位 | 不需要 | 否 | 是 |
| `std::promise<void>` | 不需要 | 是 | 否（一次通信） |

### Item 40: 对于并发使用std::atomic，volatile用于特殊内存

**核心知识点：**
- `std::atomic`：原子操作、内存顺序保证、用于并发
- `volatile`：禁止编译器优化、用于特殊内存（memory-mapped I/O）
- 两者完全不同，不可混淆
- 可以组合使用：`volatile std::atomic<int>`（原子+禁止优化）

**开发教训：**
```cpp
std::atomic<int> ac(0);  // 并发安全
volatile int vc(0);       // 并发不安全！数据竞争
// ++vc在多线程中结果不可预测
```

---

## 微调

### Item 41: 对于可移动总是被拷贝的形参使用传值方式

**适用条件（全部满足）：**
1. 参数可拷贝
2. 移动开销低
3. 总是会被拷贝（无条件拷贝）
4. 不存在切片问题

**开发教训：**
- 按值传递通过赋值拷贝参数可能比通过构造函数拷贝开销大得多
- 按值传递对于左值会多一次移动构造+析构
- Password类示例：`changeTo(string)`通过赋值时，可能触发额外的内存分配和释放

### Item 42: 考虑就地创建而⾮插⼊

**核心知识点：**
- emplacement函数直接在容器内构造对象，避免临时对象
- `emplace_back` > `push_back`（避免临时对象创建/销毁）
- emplacement优势条件：构造添加、类型不同、容器不拒绝重复

**开发教训：**
- 使用emplacement时注意资源管理类的异常安全
- `emplace_back(new Widget, killWidget)`可能因异常导致资源泄漏
- 正确做法：先用智能指针管理，再插入

```cpp
// 安全的push_back
std::shared_ptr<Widget> spw(new Widget, killWidget);
ptrs.push_back(std::move(spw));

// 不安全的emplace_back
ptrs.emplace_back(new Widget, killWidget);  // 异常时泄漏！
```

---

## 开发经验（最佳实践）

### 1. 类型推导经验
- 理解模板类型推导的三种情况，区分引用、通用引用和传值
- 优先使用auto，但注意花括号初始化和代理类的陷阱
- 使用`decltype(auto)`精确传递返回类型

### 2. 现代C++特性经验
- 使用`nullptr`代替0和NULL
- 使用`enum class`代替`enum`
- 使用`using`代替`typedef`
- 使用`override`声明所有重写函数
- 使用`noexcept`标记不抛异常的函数
- 使用`constexpr`最大化编译期计算

### 3. 智能指针经验
- 独占所有权用`std::unique_ptr`，共享所有权用`std::shared_ptr`
- 工厂函数返回`std::unique_ptr`
- 使用`std::make_unique`/`std::make_shared`而非`new`
- Pimpl惯用法配合`std::unique_ptr`时注意特殊成员函数的定义位置

### 4. 移动语义经验
- `std::move`不移动，`std::forward`只转发
- 区分通用引用（T&&带类型推导）和右值引用（T&&无类型推导）
- 右值引用用`std::move`，通用引用用`std::forward`
- 不要对RVO候选的局部变量使用`std::move`

### 5. Lambda经验
- 避免默认捕获模式`[=]`和`[&]`
- C++14使用初始化捕获实现移动捕获
- 完美转发lambda参数：`std::forward<decltype(param)>(param)`

### 6. 并发经验
- 优先使用`std::async`（基于任务）而非`std::thread`（基于线程）
- 使用RAII确保`std::thread`在所有路径上unjoinable
- `std::atomic`用于并发，`volatile`用于特殊内存

### 7. 容器操作经验
- 对于可拷贝、移动开销低、总是被拷贝的参数考虑按值传递
- 优先使用`emplace_back`代替`push_back`
- 注意emplacement与资源管理类的异常安全

---

## 开发教训（常见陷阱）

### 1. 类型推导陷阱
- auto + 花括号初始化 → `std::initializer_list`
- `decltype(auto)`返回局部变量的引用 → 未定义行为
- 代理类使auto推导出"错误"类型

### 2. 初始化陷阱
- 花括号初始化优先匹配`std::initializer_list`构造函数
- `vector<int> v{10, 20}` vs `vector<int> v(10, 20)` 含义不同
- 默认捕获`[=]`隐式捕获this指针而非成员变量副本

### 3. 移动语义陷阱
- 对const对象调用`std::move`会触发复制而非移动
- 移动操作不一定比复制快（`std::array`、SSO短字符串）
- 移动操作未声明`noexcept`时可能退回复制
- 局部变量`std::move`阻止RVO优化

### 4. 智能指针陷阱
- 从同一个原始指针创建多个`std::shared_ptr` → double-free
- `std::make_shared`与自定义删除器不兼容
- Pimpl + `std::unique_ptr`忘记在实现文件中定义析构函数 → 编译错误

### 5. 完美转发陷阱
- 花括号初始化器无法完美转发
- 0/NULL作为空指针无法完美转发为指针类型
- 仅声明的static const成员无法通过引用转发

### 6. 并发陷阱
- joinable的`std::thread`析构 → 程序终止
- `std::async`默认策略可能不创建新线程（deferred）
- `volatile`不保证原子性，多线程中使用`volatile`是数据竞争
- `std::atomic`的`cout << ai`只保证读取原子，不保证整条语句原子

### 7. Lambda陷阱
- `[=]`捕获this → 悬空指针风险
- `[&]`捕获局部变量 → 悬空引用风险
- 静态变量不被捕获但可在lambda中使用，`[=]`给人"独立"的错觉

// 修复后的C++示例代码
// 展示如何正确编写安全的C++代码

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <stdexcept>

// 解决方案1: 使用安全的C++字符串操作
void safe_string_operations() {
    // 使用std::string代替C字符串
    std::string buffer;
    std::string src = "HelloWorld";
    
    // 安全的字符串操作
    buffer = src;  // 正确: 使用std::string
    
    // 如果需要C字符串接口，使用安全版本
    char c_buffer[20];
    snprintf(c_buffer, sizeof(c_buffer), "%s", src.c_str());  // 正确: 使用snprintf
    
    // 永远不要使用gets
    // 使用fgets代替
    // char input[100];
    // fgets(input, sizeof(input), stdin);
}

// 解决方案2: 正确的内存管理
void proper_memory_management() {
    // 使用智能指针
    auto arr = std::make_unique<int[]>(100);  // 正确: 使用unique_ptr
    
    // 或者使用vector
    std::vector<int> vec(100);  // 正确: 使用vector
    
    // 如果必须使用malloc，检查返回值
    int* raw_arr = static_cast<int*>(malloc(100 * sizeof(int)));
    if (raw_arr == nullptr) {  // 正确: 检查NULL
        throw std::bad_alloc();
    }
    
    // 使用后释放并置NULL
    free(raw_arr);
    raw_arr = nullptr;  // 正确: free后置NULL
}

// 解决方案3: 线程安全操作
void thread_safe_operations() {
    std::string str = "a,b,c";
    size_t pos = 0;
    
    // 手动分割字符串，避免strtok
    while ((pos = str.find(',')) != std::string::npos) {
        std::string token = str.substr(0, pos);
        std::cout << token << std::endl;
        str.erase(0, pos + 1);
    }
    std::cout << str << std::endl;
    
    // 如果需要C接口，使用线程安全版本
    char c_str[] = "a,b,c";
    char* saveptr;
    char* token = strtok_r(c_str, ",", &saveptr);  // 正确: 使用strtok_r
    while (token != nullptr) {
        std::cout << token << std::endl;
        token = strtok_r(nullptr, ",", &saveptr);
    }
}

// 解决方案4: 安全的命令执行
void safe_command_execution(const std::string& user_input) {
    // 验证用户输入
    if (user_input.find(';') != std::string::npos ||
        user_input.find('|') != std::string::npos ||
        user_input.find('&') != std::string::npos) {
        throw std::invalid_argument("Invalid characters in command");
    }
    
    // 使用fork+exec而不是system
    // 这里只是示例，实际实现需要更多代码
    std::cout << "Would execute: " << user_input << std::endl;
}

// 解决方案5: 使用现代C++特性
void modern_cpp_features() {
    // 使用unique_ptr代替auto_ptr
    auto ptr = std::make_unique<int>(42);  // 正确: 使用unique_ptr
    
    // 使用constexpr代替宏
    constexpr int ARRAY_SIZE = 100;  // 正确: 使用constexpr
    int arr[ARRAY_SIZE];
}

// 解决方案6: 总是初始化变量
void always_initialize_variables() {
    int x = 0;  // 正确: 总是初始化
    std::cout << x << std::endl;
}

// 解决方案7: 防止整数溢出
void prevent_integer_overflow() {
    int max_int = 2147483647;
    
    // 检查溢出
    if (max_int > INT_MAX - 1) {
        throw std::overflow_error("Integer overflow");
    }
    
    int result = max_int + 1;
    std::cout << result << std::endl;
}

// 解决方案8: 避免类型双关
void avoid_type_punning() {
    float f = 3.14f;
    
    // 使用memcpy进行安全的类型转换
    int i;
    std::memcpy(&i, &f, sizeof(float));  // 正确: 使用memcpy
    
    // 或者使用union（C++中有限制）
    union {
        float f;
        int i;
    } u;
    u.f = f;
    std::cout << u.i << std::endl;
}

// 解决方案9: 完整的错误处理
void open_file_with_proper_check() {
    std::ifstream file("nonexistent.txt");
    if (!file.is_open()) {  // 正确: 检查文件是否打开
        throw std::runtime_error("Failed to open file");
    }
    
    // 使用文件
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }
}

// 解决方案10: 安全的宏定义
#define SQUARE_SAFE(x) ((x) * (x))  // 正确: 参数和整个表达式都加括号

void safe_macros() {
    int result = SQUARE_SAFE(2 + 3);  // 展开为: ((2 + 3) * (2 + 3)) = 25
    std::cout << "SQUARE_SAFE(2+3) = " << result << std::endl;
    
    // 更好的方案: 使用内联函数
    constexpr auto square = [](auto x) { return x * x; };
    std::cout << "square(2+3) = " << square(2 + 3) << std::endl;
}

// 额外的C语言编程规范应用
void c_deep_dissection_rules() {
    // 规则: 指针定义即初始化
    int* p = nullptr;  // 正确: 定义即初始化为nullptr
    
    // 规则: malloc后必须检查
    p = static_cast<int*>(malloc(sizeof(int)));
    if (p == nullptr) {  // 正确: 检查malloc返回值
        throw std::bad_alloc();
    }
    
    // 规则: free后必须置NULL
    free(p);
    p = nullptr;  // 正确: free后置NULL
    
    // 规则: 结构体成员指针必须初始化
    struct Person {
        char* name;
        int age;
    };
    
    Person person;
    person.name = static_cast<char*>(malloc(50));  // 正确: 为指针成员分配内存
    if (person.name != nullptr) {
        strncpy(person.name, "John", 49);
        person.name[49] = '\0';
    }
    person.age = 30;
    
    free(person.name);
    person.name = nullptr;
}

int main() {
    try {
        safe_string_operations();
        proper_memory_management();
        thread_safe_operations();
        modern_cpp_features();
        always_initialize_variables();
        prevent_integer_overflow();
        avoid_type_punning();
        safe_macros();
        c_deep_dissection_rules();
        
        // 这些可能会抛出异常，放在try块中
        // safe_command_execution("ls");
        // open_file_with_proper_check();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
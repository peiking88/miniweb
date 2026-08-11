// 包含常见问题的C++示例代码
// 用于演示代码评审工具能检测到的问题

#include <cstring>
#include <cstdlib>
#include <iostream>

// 问题1: 使用不安全的C函数
void unsafe_string_operations() {
    char buffer[10];
    char src[] = "HelloWorld"; // 11个字符包括\0
    
    // 缓冲区溢出风险
    strcpy(buffer, src);  // 错误: strcpy不安全
    
    // 同样的问题
    sprintf(buffer, "%s", src);  // 错误: sprintf不安全
    
    // 最危险的函数
    // gets(buffer);  // 严重错误: gets极其危险
}

// 问题2: 内存管理问题
void memory_management_issues() {
    // 未检查malloc返回值
    int* arr = (int*)malloc(100 * sizeof(int));  // 错误: 未检查NULL
    
    // 使用后未释放
    // 缺少: free(arr);
    
    // double free风险
    free(arr);
    // free(arr);  // 错误: double free
    
    // 使用已释放的内存
    // arr[0] = 42;  // 错误: use-after-free
}

// 问题3: 线程不安全函数
void thread_unsafe_operations() {
    char str[] = "a,b,c";
    
    // 线程不安全
    char* token = strtok(str, ",");  // 错误: strtok线程不安全
    
    while (token != NULL) {
        std::cout << token << std::endl;
        token = strtok(NULL, ",");
    }
}

// 问题4: 命令注入风险
void command_injection_risk() {
    char user_input[100];
    // 假设用户输入: "; rm -rf /"
    
    // 命令注入风险
    system(user_input);  // 错误: system()有命令注入风险
}

// 问题5: 已弃用的C++特性
void deprecated_cpp_features() {
    // 已弃用的智能指针
    // std::auto_ptr<int> ptr(new int(42));  // 错误: auto_ptr已弃用
}

// 问题6: 未初始化变量
void uninitialized_variables() {
    int x;  // 错误: 未初始化
    std::cout << x << std::endl;  // 未定义行为
}

// 问题7: 有符号整数溢出
void signed_integer_overflow() {
    int max_int = 2147483647;
    int result = max_int + 1;  // 有符号整数溢出
    std::cout << result << std::endl;
}

// 问题8: 类型双关（违反严格别名规则）
void type_punning() {
    float f = 3.14f;
    int i = *(int*)&f;  // 错误: 类型双关
    std::cout << i << std::endl;
}

// 问题9: 缺少错误处理
void open_file_without_check() {
    FILE* fp = fopen("nonexistent.txt", "r");
    // 错误: 未检查fopen返回值
    fclose(fp);
}

// 问题10: 宏定义陷阱
#define SQUARE(x) x * x  // 错误: 宏参数未加括号

void macro_pitfalls() {
    int result = SQUARE(2 + 3);  // 展开为: 2 + 3 * 2 + 3 = 11, 不是25
    std::cout << "SQUARE(2+3) = " << result << std::endl;
}

int main() {
    unsafe_string_operations();
    memory_management_issues();
    thread_unsafe_operations();
    // command_injection_risk();  // 注释掉以避免实际风险
    deprecated_cpp_features();
    uninitialized_variables();
    signed_integer_overflow();
    type_punning();
    open_file_without_check();
    macro_pitfalls();
    
    return 0;
}
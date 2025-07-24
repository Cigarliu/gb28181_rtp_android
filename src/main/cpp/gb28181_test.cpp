#include "gb28181_jni.h"
#include <iostream>

// 这是一个简单的测试文件，用于在非Android环境下测试GB28181相关功能
// 在实际Android项目中不会使用此文件

// 模拟JNI环境的简单函数
void test_gb28181_functions() {
    std::cout << "Testing GB28181 functions..." << std::endl;
    
    // 这里可以添加GB28181相关功能的测试代码
    // 注意：这些测试需要在实际JNI环境中进行，这里只是示例
    
    std::cout << "GB28181 test completed." << std::endl;
}

// 仅用于独立测试时的主函数
#ifdef STANDALONE_TEST
int main() {
    test_gb28181_functions();
    return 0;
}
#endif
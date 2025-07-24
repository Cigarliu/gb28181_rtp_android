# CMake最低版本要求
cmake_minimum_required(VERSION 3.10.2)

# 项目名称
project(gb28181_jni)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NATS库设置
set(NATS_DIR ${CMAKE_SOURCE_DIR}/3rd/nats.c-3.10.1)
set(NATS_BUILD_WITH_TLS OFF)
set(NATS_BUILD_EXAMPLES OFF)
set(NATS_BUILD_STREAMING OFF)

# 添加源文件
add_library(
    # 指定库名称
    gb28181-lib
    
    # 设置为共享库
    SHARED
    
    # 源文件
    src/main/cpp/gb28181_jni.cpp
    src/main/cpp/nats_client.cpp
    src/main/cpp/nats_test.cpp
)

# 查找并链接日志库
find_library(
    log-lib
    log
)

# 添加包含路径
include_directories(
    ${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include
    ${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/android
    ${CMAKE_ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include
    ${NATS_DIR}/src
    ${NATS_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/main/cpp
)

# 编译NATS静态库
file(GLOB NATS_SOURCES 
    "${NATS_DIR}/src/*.c"
    "${NATS_DIR}/src/unix/*.c"
    "${NATS_DIR}/src/glib/*.c"
)

add_library(nats_static STATIC ${NATS_SOURCES})
target_compile_definitions(nats_static PRIVATE -DNATS_STATIC)

# 链接库
target_link_libraries(
    gb28181-lib
    ${log-lib}
    nats_static
)
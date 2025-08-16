# CMake最低版本要求
cmake_minimum_required(VERSION 3.10.2)

# 项目名称
project(gb28181_jni C CXX)

# 确保这是Android构建
if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists.txt is designed for Android builds only. Use build_en.bat for ndk-build.")
endif()

# 显示编译器信息
message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "ANDROID_ABI: ${ANDROID_ABI}")
message(STATUS "ANDROID_PLATFORM: ${ANDROID_PLATFORM}")

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android 特定设置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID -DNATS_STATIC")

# 确保使用正确的编译器标志
if(ANDROID)
    add_definitions(-DANDROID)
    add_definitions(-DNATS_STATIC)
endif()

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
    src/main/cpp/rtp/RtpSendPs.cpp
    src/main/cpp/rtp/h264_streaming_framer.cpp
)

# 查找并链接日志库
find_library(log-lib log)

# 检查是否找到log库
if(NOT log-lib)
    message(FATAL_ERROR "log library not found")
endif()
message(STATUS "Found log library: ${log-lib}")

# 添加包含路径
target_include_directories(gb28181-lib PRIVATE
    ${NATS_DIR}/src
    ${NATS_DIR}/src/include
    ${NATS_DIR}/src/unix
    ${NATS_DIR}/src/glib
    ${CMAKE_SOURCE_DIR}/src/main/cpp
    ${CMAKE_SOURCE_DIR}/src/main/cpp/rtp
)

# 编译NATS静态库
# 基础源文件
file(GLOB NATS_CORE_SOURCES "${NATS_DIR}/src/*.c")

# 平台特定源文件
if(ANDROID)
    file(GLOB NATS_PLATFORM_SOURCES
        "${NATS_DIR}/src/unix/*.c"
        "${NATS_DIR}/src/glib/*.c"
    )
else()
    # 如果不是Android平台，可以添加其他平台的源文件
    set(NATS_PLATFORM_SOURCES "")
endif()

# 合并所有源文件
set(NATS_SOURCES ${NATS_CORE_SOURCES} ${NATS_PLATFORM_SOURCES})

add_library(nats_static STATIC ${NATS_SOURCES})
target_compile_definitions(nats_static PRIVATE
    NATS_STATIC
    $<$<BOOL:${ANDROID}>:ANDROID>
)
target_include_directories(nats_static PRIVATE
    ${NATS_DIR}/src
    ${NATS_DIR}/src/include
    ${NATS_DIR}/src/unix
    ${NATS_DIR}/src/glib
)

# 为Android设置特殊的编译标志，避免pthread链接
if(ANDROID)
    target_compile_options(nats_static PRIVATE -DANDROID)
    # 不使用-pthread标志，因为Android的pthread是内置在libc中的
endif()

# Android 特定链接库 - pthread在Android中内置在libc中，不需要单独链接
# if(ANDROID)
#     target_link_libraries(nats_static pthread)
# endif()

# 链接库
target_link_libraries(
    gb28181-lib
    ${log-lib}
    nats_static
)

# Android 特定链接库 - pthread在Android中内置在libc中，不需要单独链接
# if(ANDROID)
#     target_link_libraries(gb28181-lib pthread)
# endif()
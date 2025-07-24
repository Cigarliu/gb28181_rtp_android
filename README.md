# GB28181 JNI 库

这是一个用于Android平台的GB28181协议JNI实现库。该库提供了GB28181协议的C++实现，可以通过JNI接口在Android应用中使用。

## 项目结构

```
├── build.bat              # Windows批处理编译脚本 (NDK-Build)
├── build_cmake.bat        # Windows批处理编译脚本 (CMake)
├── build.ps1              # PowerShell编译脚本 (支持多选项)
├── test_env.bat           # Windows批处理环境测试脚本
├── test_env.ps1           # PowerShell环境测试脚本
├── CMakeLists.txt         # CMake构建配置文件
├── jni/                   # NDK编译配置目录
│   ├── Android.mk         # NDK编译主配置文件
│   └── Application.mk     # NDK应用配置文件
└── src/                   # 源代码目录
    └── main/               
        └── cpp/           # C++源代码
            ├── gb28181_jni.cpp     # JNI实现
            ├── gb28181_jni.h       # JNI头文件
            └── gb28181_test.cpp    # 测试文件
```

## 环境配置与测试

项目提供了环境测试脚本，可以帮助检查NDK环境是否正确配置：

```bash
# Windows批处理脚本
test_env.bat

# PowerShell脚本（提供更详细信息）
.\test_env.ps1
```

## 编译方法

### 使用编译脚本（推荐）

项目提供了多种编译脚本，可以根据需要选择：

#### Windows批处理脚本

```bash
# 使用NDK-Build编译
build.bat

# 使用CMake编译
build_cmake.bat
```

#### PowerShell脚本（更多选项）

```powershell
# 默认使用NDK-Build和arm64-v8a架构
.\build.ps1

# 指定构建系统和目标架构
.\build.ps1 -BuildSystem cmake -Abi armeabi-v7a

# 指定NDK版本
.\build.ps1 -NdkVersion 25.1.8937393
```

### 手动编译

#### 使用NDK-Build编译

1. 确保已安装Android NDK
2. 在项目根目录执行：

```bash
ndk-build
```

#### 使用CMake编译

1. 确保已安装CMake和Android NDK
2. 在项目根目录执行：

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21
make
```

## 使用方法

1. 将编译生成的`.so`库文件添加到Android项目的`jniLibs`目录
2. 在Java代码中加载库并调用本地方法：

```java
public class GB28181 {
    static {
        System.loadLibrary("gb28181-lib");
    }
    
    public native String stringFromJNI();
    public native boolean initGB28181(String saveDirectory);
    public native boolean inputData(int dataType, byte[] data);
    public native boolean closeGB28181();
}
```

## 注意事项

- 该库仅实现了GB28181协议的基本功能，可根据实际需求进行扩展
- 使用前请确保了解GB28181协议的相关规范
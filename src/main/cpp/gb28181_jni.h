#ifndef GB28181_JNI_H
#define GB28181_JNI_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

// 从JNI返回字符串
JNIEXPORT jstring JNICALL
Java_com_example_gb28181jni_GB28181_stringFromJNI(JNIEnv *env, jobject thiz);

// 初始化GB28181
JNIEXPORT jboolean JNICALL
Java_com_example_gb28181jni_GB28181_initGB28181(JNIEnv *env, jobject thiz, 
                                              jstring serverIP, jint serverPort);

// JNI_OnLoad函数
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved);

#ifdef __cplusplus
}
#endif

#endif // GB28181_JNI_H
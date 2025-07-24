package com.example.gb28181jni;

/**
 * GB28181 JNI接口类
 */
public class GB28181 {
    // 加载本地库
    static {
        System.loadLibrary("gb28181-lib");
    }
    
    /**
     * 数据类型枚举
     * 用于指定输入数据的类型
     */
    public enum DataType {
        /**
         * 视频流数据
         */
        VIDEO_STREAM(0),
        
        /**
         * SIP配置信息
         */
        SIP_CONFIG(1);
        
        private final int value;
        
        DataType(int value) {
            this.value = value;
        }
        
        public int getValue() {
            return value;
        }
    }

    /**
     * 返回库的版本信息
     * @return 版本信息字符串
     */
    public native String stringFromJNI();

    /**
     * 初始化GB28181客户端
     * @param saveDirectory 数据保存目录，如果为null则不保存文件
     * @return 初始化结果，true表示成功，false表示失败
     */
    public native boolean initGB28181(String saveDirectory);

    /**
     * 输入数据到系统
     * @param dataType 数据类型，使用DataType枚举指定
     * @param data 数据字节数组
     * @return 处理结果，true表示成功，false表示失败
     */
    public native boolean inputData(int dataType, byte[] data);

    /**
     * 释放C++层资源
     * @return 关闭结果，true表示成功，false表示失败
     */
    public native boolean closeGB28181();
}
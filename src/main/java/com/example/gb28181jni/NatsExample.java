package com.example.gb28181jni;

import android.util.Log;

/**
 * NATS集成示例类
 * 演示如何使用GB28181 JNI接口发送各类数据到NATS服务器
 */
public class NatsExample {
    private static final String TAG = "NatsExample";
    private GB28181 gb28181;
    
    /**
     * 初始化示例
     */
    public void initialize() {
        // 创建GB28181实例
        gb28181 = new GB28181();
        
        // 获取版本信息
        String version = gb28181.stringFromJNI();
        Log.d(TAG, "GB28181 JNI版本: " + version);
        
        // 初始化GB28181客户端
        // 参数: 视频流保存目录路径（仅视频流类型数据会保存到此目录，如果为null则不保存文件）
        String saveDirectory = "/storage/emulated/0/DCIM/GB28181";
        boolean result = gb28181.initGB28181(saveDirectory);
        
        if (result) {
            Log.d(TAG, "GB28181客户端初始化成功");
        } else {
            Log.e(TAG, "GB28181客户端初始化失败");
        }
    }
    
    /**
     * 输入数据示例
     * @param dataType 数据类型（GB28181.DataType枚举值）
     * @param data 数据内容
     * @return 操作结果
     */
    public boolean inputData(int dataType, byte[] data) {
        if (gb28181 == null) {
            Log.e(TAG, "GB28181客户端未初始化");
            return false;
        }
        
        boolean result = gb28181.inputData(dataType, data);
        if (result) {
            String dataTypeStr = "";
            switch (dataType) {
                case GB28181.DataType.VIDEO_STREAM:
                    dataTypeStr = "视频流";
                    break;
                case GB28181.DataType.SIP_CONFIG:
                    dataTypeStr = "SIP配置信息";
                    break;
                default:
                    dataTypeStr = "未知类型";
                    break;
            }
            
            Log.d(TAG, dataTypeStr + "处理成功");
            return true;
        } else {
            Log.e(TAG, "数据处理失败");
            return false;
        }
    }
    
    /**
     * 发送视频流示例（兼容旧接口）
     * @param videoData 视频数据
     * @return 发送结果
     */
    public boolean sendVideo(byte[] videoData) {
        return inputData(GB28181.DataType.VIDEO_STREAM, videoData);
    }
    
    /**
     * 关闭连接
     */
    public void close() {
        if (gb28181 != null) {
            boolean result = gb28181.closeGB28181();
            if (result) {
                Log.d(TAG, "GB28181客户端关闭成功");
            } else {
                Log.e(TAG, "GB28181客户端关闭失败");
            }
            gb28181 = null;
        }
    }
}
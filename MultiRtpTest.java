public class MultiRtpTest {
    static {
        System.loadLibrary("gb28181-lib");
    }

    // JNI方法声明
    public static native String stringFromJNI();
    public static native int initGB28181(String natsUrl);
    public static native int inputData(int dataType, byte[] data, int length);
    public static native void closeGB28181();

    // DataType枚举值
    public static final int VIDEO_STREAM1 = 1;
    public static final int VIDEO_STREAM2 = 2;
    public static final int VIDEO_STREAM3 = 3;
    public static final int VIDEO_STREAM4 = 4;
    public static final int VIDEO_STREAM5 = 5;
    public static final int VIDEO_STREAM6 = 6;
    public static final int SIP_CONFIG1 = 7;
    public static final int SIP_CONFIG2 = 8;
    public static final int SIP_CONFIG3 = 9;
    public static final int SIP_CONFIG4 = 10;
    public static final int SIP_CONFIG5 = 11;
    public static final int SIP_CONFIG6 = 12;

    public static void main(String[] args) {
        System.out.println("Testing Multi-RTP functionality...");
        
        // 初始化GB28181
        int result = initGB28181("nats://localhost:4222");
        System.out.println("Init result: " + result);
        
        // 配置第一路RTP
        String sipConfig1 = "192.168.1.100:5060:8000:1001";
        byte[] configData1 = sipConfig1.getBytes();
        result = inputData(SIP_CONFIG1, configData1, configData1.length);
        System.out.println("SIP Config 1 result: " + result);
        
        // 配置第二路RTP
        String sipConfig2 = "192.168.1.100:5060:8002:1002";
        byte[] configData2 = sipConfig2.getBytes();
        result = inputData(SIP_CONFIG2, configData2, configData2.length);
        System.out.println("SIP Config 2 result: " + result);
        
        // 模拟发送H264数据到第一路
        byte[] h264Data1 = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, (byte)0x80, 0x1e}; // 模拟SPS
        result = inputData(VIDEO_STREAM1, h264Data1, h264Data1.length);
        System.out.println("Video Stream 1 result: " + result);
        
        // 模拟发送H264数据到第二路
        byte[] h264Data2 = {0x00, 0x00, 0x00, 0x01, 0x68, (byte)0xce, 0x3c, (byte)0x80}; // 模拟PPS
        result = inputData(VIDEO_STREAM2, h264Data2, h264Data2.length);
        System.out.println("Video Stream 2 result: " + result);
        
        System.out.println("Test completed. Cleaning up...");
        
        // 清理资源
        closeGB28181();
        System.out.println("Cleanup completed.");
    }
}
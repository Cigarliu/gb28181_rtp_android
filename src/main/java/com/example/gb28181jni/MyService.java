package com.rust.sip.GB28181;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import com.aiyu.ironmotor.BaseApplication;
import com.aiyu.ironmotor.R;
import com.aiyu.ironmotor.utils.CommonUtils;
import com.aiyu.ironmotor.utils.Constact;
import com.aiyu.ironmotor.utils.HyperSdkUtils;
import com.aiyu.ironmotor.utils.ScreenUtils;
import com.aiyu.ironmotor.utils.SharedPrefsUtil;
import com.aiyu.ironmotor.view.CameraView;
import com.arthenica.mobileffmpeg.ExecuteCallback;
import com.example.gb28181jni.GB28181;
import com.rust.sip.GB28181.gb28181.GB28181Params;
import com.rust.sip.GB28181.gb28181.XMLUtil;
import com.rust.sip.GB28181.net.IpAddress;
import com.rust.sip.GB28181.sdp.MediaDescriptor;
import com.rust.sip.GB28181.sdp.SessionDescriptor;
import com.rust.sip.GB28181.sip.address.NameAddress;
import com.rust.sip.GB28181.sip.address.SipURL;
import com.rust.sip.GB28181.sip.authentication.DigestAuthentication;
import com.rust.sip.GB28181.sip.header.AuthorizationHeader;
import com.rust.sip.GB28181.sip.header.ExpiresHeader;
import com.rust.sip.GB28181.sip.header.UserAgentHeader;
import com.rust.sip.GB28181.sip.message.Message;
import com.rust.sip.GB28181.sip.message.MessageFactory;
import com.rust.sip.GB28181.sip.message.SipMethods;
import com.rust.sip.GB28181.sip.message.SipResponses;
import com.rust.sip.GB28181.sip.provider.SipProvider;
import com.rust.sip.GB28181.sip.provider.SipProviderListener;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Objects;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.LinkedBlockingDeque;

public class MyService extends Service implements SipProviderListener {

    public static final String TAG_ = "MyService-推流";

    private static final String TAG = "MyService";
    private MyBinder myBinder;
    private SipProvider sipProvider;
    private Timer timerForKeepAlive;
    private long keepAliveSN = 0;
    private int ssrc = 0;
    private static final int VIDEO_MAX_PACKET_SIZE = 10;
    private HashMap<String, DeviceUtils> devices = new HashMap<>(6);
    GB28181 gb28181 = new GB28181();
    //    private LinkedBlockingDeque<VideoData> videoQueue = new LinkedBlockingDeque<>(VIDEO_MAX_PACKET_SIZE);
//    private HashMap<String, LinkedBlockingQueue<VideoData>> videoQueueList = new HashMap<>();
//    private HashMap<String, VideoTread> videoTreads = new HashMap<>();
//    private HashMap<String, DJIStreamerFinal> videoStreams = new HashMap<>();
//    private String localDeviceId;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        myBinder = new MyBinder();
        return myBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Notification notification = new NotificationCompat.Builder(this, "channel_id")
                .setContentTitle("My Foreground Service")
                .setContentText("This is a foreground service")
                .setSmallIcon(R.mipmap.icon_play)
                .build();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        GB28181_Stop();
        sipProvider.stopTrasport();
    }

    public class MyBinder extends Binder {
        public MyService getService() {
            return MyService.this;
        }
    }


    //region GB28181 部分
    /*国标模块参数初始化*/
    public void GB28181Init() {

        String serviceIp = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_SERVICE_ADRESS, Constact.SIP_SERVICE_ADRESS_DEFAULT);
        String servicePort = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_SERVICE_PORT, Constact.SIP_SERVICE_PORT_DEFAULT);
        String serviceNumber = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_SERVICE_NUMBER, Constact.SIP_SERVICE_NUMBER_DEFAULT);
        String serviceRegion = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_SERVICE_REGION, Constact.SIP_SERVICE_REGION_DEFAULT);
        String servicePassword = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_SERVICE_PASSWORD, Constact.SIP_SERVICE_PASSWORD_DEFAULT);
        String localDeviceNumber = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_LOCAL_DEVICE_NUMBER, Constact.SIP_LOCAL_DEVICE_NUMBER_DEFAULT);
        String localMediaChannel = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.SIP_CHANNEL_NUMBER, Constact.SIP_CHANNEL_NUMBER_DEFAULT);
        int localPort = SharedPrefsUtil.getInstance(getApplicationContext()).getInt(Constact.LOCAL_PORT, Constact.LOCAL_PORT_DEFAULT);
        String localAddress = SharedPrefsUtil.getInstance(getApplicationContext()).getString(Constact.LOCAL_IP, NetUtils.getIPAddress(getApplicationContext()));

        GB28181Params.setSIPServerIPAddress(serviceIp);//SIP服务器地址
        GB28181Params.setRemoteSIPServerPort(Integer.parseInt(servicePort));//SIP服务器端口
        GB28181Params.setLocalSIPIPAddress(localAddress);//本机地址
        GB28181Params.setRemoteSIPServerID(serviceNumber);
        GB28181Params.setRemoteSIPServerSerial(serviceRegion);
        GB28181Params.setLocalSIPPort(localPort);//本机端口
        GB28181Params.setCameraHeigth(ScreenUtils.getScreenHeight(getApplicationContext()));
        GB28181Params.setCameraWidth(ScreenUtils.getScreenWidth(getApplicationContext()));
        GB28181Params.setPassword(servicePassword);//密码
        GB28181Params.setLocalSIPDeviceId(localDeviceNumber);
        GB28181Params.setLocalSIPMediaId(localMediaChannel);
        GB28181Params.setCurGBState(0);
        GB28181Params.setCurDeviceDownloadMeidaState(0);
        GB28181Params.setCurDeviceDownloadMeidaState(0);
        GB28181Params.setCurDevicePlayMediaState(0);
        GB28181Params.setCameraState(0);
        Log.d(TAG, "MyService -> GB28181Init()");
        CommonUtils.instance.loge(TAG, "GB28181Params.setSIPServerIPAddress : " + serviceIp + "\n"
                + "GB28181Params.setLocalSIPIPAddress : " + localAddress + "\n"
                + "GB28181Params.setLocalSIPPort : " + localPort + "\n"
                + "GB28181Params.setLocalSIPDeviceId : " + localDeviceNumber + "\n"
                + "GB28181Params.setLocalSIPMediaId : " + localMediaChannel + "\n"
                + "GB28181Params.setPassword : " + servicePassword + "\n"
                + "GB28181Params.setSIPServerPort : " + servicePort + "\n"
                + "GB28181Params.setSIPServerNumber : " + serviceNumber + "\n"
                + "GB28181Params.setSIPServerRegion : " + serviceRegion + "\n");
//        HyperSdkUtils.EncodeH264(file);
    }

    public void GB28181ReStart() {
        GB28181_Stop();
        GB28181_Start();
    }

    public void GB28181_Start() {
        new Thread(() -> {
            IpAddress.setLocalIpAddress(GB28181Params.getLocalSIPIPAddress());
            sipProvider = new SipProvider(GB28181Params.getLocalSIPIPAddress(), GB28181Params.getLocalSIPPort());
            sipProvider.addSipProviderListener(this);
            NameAddress to = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getRemoteSIPServerSerial()));
            NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getRemoteSIPServerSerial()));
            NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
            Message message = MessageFactory.createRequest(sipProvider, SipMethods.REGISTER, new SipURL(GB28181Params.getRemoteSIPServerID(), GB28181Params.getRemoteSIPServerSerial()), to, from, contact, null);
            message.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));

            sipProvider.sendMessage(message, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
            CommonUtils.instance.loge(TAG, "GB28181_Start");
        }).start();
    }

    public void GB28181_Stop() {
        new Thread(() -> {
            if (sipProvider != null && GB28181Params.CurGBState == 1) {
                NameAddress to = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                Message message = MessageFactory.createRequest(sipProvider, SipMethods.REGISTER, new SipURL(GB28181Params.getRemoteSIPServerID(), GB28181Params.getRemoteSIPServerSerial()), to, from, contact, null);
                message.setExpiresHeader(new ExpiresHeader(0));
                message.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                sipProvider.sendMessage(message, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                sipProvider.halt();
                GB28181Params.setCurDeviceDownloadMeidaState(0);
                GB28181Params.setCurDeviceDownloadMeidaState(0);
                GB28181Params.setCurDevicePlayMediaState(0);
            }
        }).start();

    }

    private void GB28181_KeepAlive() {
        if (sipProvider != null && GB28181Params.CurGBState == 1) {
            NameAddress to = new NameAddress(new SipURL(GB28181Params.getSIPServerIPAddress(), GB28181Params.getSIPServerIPAddress()));
            NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
            NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
            String body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
                    "<Notify>\n" +
                    "  <CmdType>Catalog</CmdType>\n" +
                    "  <SN>" + String.valueOf(keepAliveSN++) + "</SN>\n" +
                    "  <DeviceID>" + GB28181Params.getLocalSIPDeviceId() + "</DeviceID>\n" +
                    "  <Status>OK</Status>\n" +
                    "</Notify>";
            Message message = MessageFactory.createMessageRequest(sipProvider, to, from, null, XMLUtil.XML_MANSCDP_TYPE, body);
            sipProvider.sendMessage(message, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
        }
    }

    private String getDeviceList() {
        // 确保标签对齐和正确闭合
        return "    <Item>\n" +
                "      <DeviceID>" + GB28181Params.getBackId() + "</DeviceID>\n" +
                "      <Name>back</Name>\n" +
                "      <Status>ON</Status>\n" +
                "    </Item>\n" +
                "    <Item>\n" +
                "      <DeviceID>" + GB28181Params.getLeftFontId() + "</DeviceID>\n" +
                "      <Name>left_font</Name>\n" +
                "      <Status>ON</Status>\n" +
                "    </Item>\n" +
                "    <Item>\n" +
                "      <DeviceID>" + GB28181Params.getLeftBackId() + "</DeviceID>\n" +
                "      <Name>left_back</Name>\n" +
                "      <Status>ON</Status>\n" +
                "    </Item>\n" +
                "    <Item>\n" +
                "      <DeviceID>" + GB28181Params.getRightFontId() + "</DeviceID>\n" +
                "      <Name>right_font</Name>\n" +
                "      <Status>ON</Status>\n" +
                "    </Item>\n" +
                "    <Item>\n" +
                "      <DeviceID>" + GB28181Params.getRightBackId() + "</DeviceID>\n" +
                "      <Name>right_back</Name>\n" +
                "      <Status>ON</Status>\n" +
                "    </Item>\n";
    }

//    class VideoTread extends Thread {
//        private DeviceUtils device;
//
//        public VideoTread(DeviceUtils device) {
//            this.device = device;
//        }
//
//        @Override
//        public void run() {
//            while (true) {
//                if (device.getState() == 1
//                        && device.getQueue() != null && !device.getQueue().isEmpty()) {
//                    VideoData video = device.getQueue().poll();
//                    if (video != null) {
//                        try {
//                            if (device.getStream() != null) {
//                                device.getStream().pushFrame(video.data);
//                            } else {
//                                if (gb28181 != null && device.getId().equals(GB28181Params.getFontId())) {
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM1.getValue(), video.data);
//                                    CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + video.data.length + " isKeyFrame=" + video.isKeyFrame);
//                                } else if (gb28181 != null && device.getId().equals(GB28181Params.getBackId())) {
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM2.getValue(), video.data);
//                                    CommonUtils.instance.logd(TAG, "onBack: push frame " + isSuccess + " video.data.length =" + video.data.length + " isKeyFrame=" + video.isKeyFrame);
//                                } else if (gb28181 != null && device.getId().equals(GB28181Params.getLeftFontId())) {
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM3.getValue(), video.data);
//                                    CommonUtils.instance.logd(TAG, "onLeftFont: push frame " + isSuccess + " video.data.length =" + video.data.length + " isKeyFrame=" + video.isKeyFrame);
//                                } else if (gb28181 != null && device.getId().equals(GB28181Params.getLeftBackId())) {
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM4.getValue(), video.data);
//                                    CommonUtils.instance.logd(TAG, "onLeftBack: push frame " + isSuccess + " video.data.length =" + video.data.length + " isKeyFrame=" + video.isKeyFrame);
//                                } else if (gb28181 != null && device.getId().equals(GB28181Params.getRightFontId())) {
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM5.getValue(), video.data);
//                                    CommonUtils.instance.logd(TAG, "onRightFont: push frame " + isSuccess + " video.data.length =" + video.data.length + " isKeyFrame=" + video.isKeyFrame);
//                                } else if (gb28181 != null && device.getId().equals(GB28181Params.getRightBackId())) {
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM6.getValue(), video.data);
//                                    CommonUtils.instance.logd(TAG, "onRightBack: push frame " + isSuccess + " video.data.length =" + video.data.length + " isKeyFrame=" + video.isKeyFrame);
//                                }
//                            }
//                        } catch (Exception e) {
//                            if (device.getStream() != null) {
//                                device.getStream().stopStreaming();
//                                device.setStream(null);
//                            }
//                            CommonUtils.instance.logd(TAG, Arrays.toString(e.getStackTrace()));
//                        }
//                    }
//                }
//            }
//        }
//    }


//    long startTime = 0;

    public void setVideoDataCallBack(DeviceUtils device) {
        LinkedBlockingDeque<VideoData> videoQueue = new LinkedBlockingDeque<>(VIDEO_MAX_PACKET_SIZE);
        device.setQueue(videoQueue);

//        VideoTread videoTread = new VideoTread(device);
//        videoTread.start();
//        device.setTread(videoTread);

//        videoQueue.clear();
//        startTime = System.nanoTime();
        if (device.getGb28181() == null) {
            try {
                DJIStreamerFinal streamer = new DJIStreamerFinal(device.getServiceIp(), device.getServicePort(), device.getSsrc());
                device.setStream(streamer);

                streamer.startStreaming();
            } catch (Exception e) {
                CommonUtils.instance.logd(TAG, "packager 创建失败");
            }
        }
    }

    /**
     * 从原始数据中提取特定类型的NALU
     */
    private byte[] extractNalu(byte[] data, int expectedType) {
        int start = findNaluStart(data);
        if (start < 0) return null;

        // 检查类型
        if ((data[start] & 0x1F) == expectedType) {
            return Arrays.copyOfRange(data, start, data.length);
        }

        // 尝试在数据中查找
        for (int i = start; i < data.length; i++) {
            if (i < data.length - 3 &&
                    data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
                int naluStart = i + 3;
                if (naluStart < data.length && (data[naluStart] & 0x1F) == expectedType) {
                    return Arrays.copyOfRange(data, naluStart, data.length);
                }
            }
        }
        return null;
    }

    /**
     * 查找NALU起始位置（简化版）
     */
    private int findNaluStart(byte[] data) {
        for (int i = 0; i < data.length - 4; i++) {
            if (data[i] == 0 && data[i + 1] == 0) {
                if (data[i + 2] == 1) return i + 3; // 3字节起始码
                if (data[i + 2] == 0 && data[i + 3] == 1) return i + 4; // 4字节起始码
            }
        }
        return -1;
    }


    private final CameraView.OnVideoDataListener videoDataListener = new CameraView.OnVideoDataListener() {
        @Override
        public void onFontH264Data(byte[] bytes, boolean isKeyFrame) {
            DeviceUtils device = devices.get(GB28181Params.getFontId());
            boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM1.getValue(), bytes);
            CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + bytes.length + " isKeyFrame=" + isKeyFrame);
//            CommonUtils.instance.logd(TAG, "onFontH264Data: " + device);
            handleVideoData(device, bytes, isKeyFrame);
        }

        @Override
        public void onBackH264Data(byte[] bytes, boolean isKeyFrame) {
            DeviceUtils device = devices.get(GB28181Params.getBackId());
            boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM2.getValue(), bytes);
            CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + bytes.length + " isKeyFrame=" + isKeyFrame);
//            CommonUtils.instance.logd(TAG, "onBackH264Data: " + device);
//            handleVideoData(device, bytes, isKeyFrame);
        }

        @Override
        public void onLeftFontH264Data(byte[] bytes, boolean isKeyFrame) {
            boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM3.getValue(), bytes);
            CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + bytes.length + " isKeyFrame=" + isKeyFrame);
            DeviceUtils device = devices.get(GB28181Params.getLeftFontId());
//            handleVideoData(device, bytes, isKeyFrame);
        }

        @Override
        public void onLeftBackH264Data(byte[] bytes, boolean isKeyFrame) {
            boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM4.getValue(), bytes);
            CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + bytes.length + " isKeyFrame=" + isKeyFrame);
            DeviceUtils device = devices.get(GB28181Params.getLeftBackId());
//            handleVideoData(device, bytes, isKeyFrame);
        }

        @Override
        public void onRightFontH264Data(byte[] bytes, boolean isKeyFrame) {
            boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM5.getValue(), bytes);
            CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + bytes.length + " isKeyFrame=" + isKeyFrame);
            DeviceUtils device = devices.get(GB28181Params.getRightFontId());
//            handleVideoData(device, bytes, isKeyFrame);
        }

        @Override
        public void onRightBackH264Data(byte[] bytes, boolean isKeyFrame) {
            boolean isSuccess = gb28181.inputData(GB28181.DataType.VIDEO_STREAM6.getValue(), bytes);
            CommonUtils.instance.logd(TAG, "onFont: push frame " + isSuccess + " video.data.length =" + bytes.length + " isKeyFrame=" + isKeyFrame);
            DeviceUtils device = devices.get(GB28181Params.getRightBackId());
//            handleVideoData(device, bytes, isKeyFrame);
        }
    };

    private void handleVideoData(DeviceUtils device, byte[] bytes, boolean isKeyFrame) {
        if (device == null) {
            return;
        }

        if (Objects.requireNonNull(BaseApplication.Companion.getApplication()).getDeviceList() == null || BaseApplication.Companion.getApplication().getDeviceList().isEmpty()) {
            return;
        }

//        if (device.getState() == 1 && device.getQueue() != null && device.getTread() != null) {
////                CommonUtils.instance.logd(TAG,"onH264Data: " + bytes.length);
//            if (device.getQueue().size() >= VIDEO_MAX_PACKET_SIZE) {
//                if (Objects.requireNonNull(BaseApplication.Companion.getApplication()).getDeviceList() != null&& !BaseApplication.Companion.getApplication().getDeviceList().isEmpty()) {
//                    VideoData videoDataFrame = device.getQueue().poll();
//                    if (videoDataFrame.isKeyFrame) {
//                        device.getQueue().poll();
//                        try {
//                            device.getQueue().putLast(videoDataFrame);
//                        } catch (Exception e) {
//                            CommonUtils.instance.logd(TAG, "putFirst error: " + e.getMessage());
//                        }
//
//                    }
//                } else {
//                    device.getQueue().poll();
//                }
//            }
//
//            VideoData videoData = new VideoData();
//            videoData.setData(bytes);
//            videoData.setKeyFrame(isKeyFrame);
//            videoData.setDeviceId(device.getId());
////            CommonUtils.instance.logd(TAG, "onH264Data: " + bytes.length + " isKeyFrame: " + isKeyFrame);
//            if (device.getQueue() != null) {
//                device.getQueue().add(videoData);
//            }
//        }

    }

    public CameraView.OnVideoDataListener getVideoDataListener() {
        return videoDataListener;
    }


    private TimerTask keepALiveTask = new TimerTask() {
        @Override
        public void run() {
            GB28181_KeepAlive();
        }
    };

    private String subString = "3402020049131000000";

    @Override
    public void onReceivedMessage(SipProvider sip_provider, Message message) {
        if (message.isResponse()) {
            CommonUtils.instance.loge(TAG, "收到响应消息：" + message.toString());
            switch (message.getCSeqHeader().getMethod()) {
                case SipMethods.REGISTER:
                    if (message.getStatusLine().getCode() == 401) {
                        NameAddress to = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                        NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                        NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                        Message res = MessageFactory.createRegisterRequest(sipProvider, to, from, contact, null, null);
                        res.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                        AuthorizationHeader ah = new AuthorizationHeader("Digest");
                        ah.addUsernameParam(GB28181Params.getLocalSIPDeviceId());
                        ah.addRealmParam(message.getWwwAuthenticateHeader().getRealmParam());
                        ah.addNonceParam(message.getWwwAuthenticateHeader().getNonceParam());
                        ah.addUriParam(res.getRequestLine().getAddress().toString());
                        ah.addQopParam(message.getWwwAuthenticateHeader().getQopParam());
                        String response = (new DigestAuthentication(SipMethods.REGISTER,
                                ah, null, GB28181Params.getPassword())).getResponse();
                        ah.addResponseParam(response);
                        res.setAuthorizationHeader(ah);
                        if (GB28181Params.getCurGBState() == 1) {
                            res.setExpiresHeader(new ExpiresHeader(0));
                        }
                        sipProvider.sendMessage(res, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                    } else if (message.getStatusLine().getCode() == 200) {
                        //注销成功
                        if (GB28181Params.getCurGBState() == 1) {
                            GB28181Params.setCurGBState(0);
                            //取消发送心跳包
                            timerForKeepAlive.cancel();
                        } else {//注册成功
                            GB28181Params.setCurGBState(1);
                            //每隔60秒 发送心跳包
                            timerForKeepAlive = new Timer(true);
                            timerForKeepAlive.schedule(keepALiveTask, 0, 30 * 1000);
                        }
                    }
                    break;
                case SipMethods.MESSAGE:
                    break;
                case SipMethods.ACK:
                    break;
                case SipMethods.BYE:
                    break;
            }
        } else if (message.isRequest()) {
            if (message.isMessage()) {
                if (message.hasBody()) {
                    String body = message.getBody();
                    String sn = body.substring(body.indexOf("<SN>") + 4, body.indexOf("</SN>"));
                    String cmdType = body.substring(body.indexOf("<CmdType>") + 9, body.indexOf("</CmdType>"));
                    if (message.getBodyType().toLowerCase().equals("application/manscdp+xml")) {
                        //发送 200 OK
                        if (cmdType.equals("Catalog")) {
                            Message CatalogResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), null);
                            CatalogResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(CatalogResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);


                            CommonUtils.instance.loge(TAG_, "收到Catalog请求，body=" + body);

                            //region catalogBody
                            String catalogBody = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
                                    "    <Response>\n" +
                                    "      <CmdType>Catalog</CmdType>\n" +
                                    "      <SN>" + sn + "</SN>\n" +
                                    "      <DeviceID>" + GB28181Params.getLocalSIPDeviceId() + "</DeviceID>\n" +
                                    "      <SumNum>6</SumNum>\n" +
                                    "      <DeviceList Num='6'>\n" +
                                    "          <Item>\n" +
                                    "            <DeviceID>" + GB28181Params.getFontId() + "</DeviceID>\n" +
                                    "            <Name>font</Name>\n" +
//                                    "            <Manufacturer>ZBGD</Manufacturer>\n" +
//                                    "            <Model>MODEL</Model>\n" +
//                                    "            <Owner>aiyu</Owner>\n" +
//                                    "            <CivilCode>3402000001</CivilCode>\n" +
//                                    "            <Address>local</Address>\n" +
//                                    "            <Parental>0</Parental>\n" +
//                                    "            <SafetyWay>0</SafetyWay>\n" +
//                                    "            <RegisterWay>1</RegisterWay>\n" +
//                                    "            <Secrecy>0</Secrecy>\n" +
//                                    "            <IPAddress>" + GB28181Params.getLocalSIPIPAddress() + "</IPAddress>\n" +
//                                    "            <Port>" + GB28181Params.getLocalSIPPort() + "</Port>\n" +
//                                    "            <Password>12345678</Password>\n" +
                                    "            <Status>ON</Status>\n" +
                                    "          </Item>\n" +
                                    getDeviceList() + "\n" +
                                    "      </DeviceList>\n" +
                                    "    </Response>";
                            //endregion

                            Message CatalogResponseRequest = MessageFactory.createMessageRequest(sipProvider, message.getFromHeader().getNameAddress(),
                                    message.getToHeader().getNameAddress(), null, XMLUtil.XML_MANSCDP_TYPE, catalogBody);
                            CatalogResponseRequest.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(CatalogResponseRequest, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                        } else if (cmdType.equals("DeviceControl")) {
                            //ToDo 解析控制指令
                            Message DeviceControlResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), null);
                            DeviceControlResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(DeviceControlResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                        } else if (cmdType.equals("RecordInfo")) {
                            int startIndex = body.indexOf("<DeviceID>");
                            String deviceId = body.substring(startIndex + 10, startIndex + 30);
                            int startTimeIndex = body.indexOf("<StartTime>");
                            String startTime = body.substring(startTimeIndex + 11, startTimeIndex + 21);


                            File[] files = getRecordInfoList(deviceId, startTime);
                            if (files == null) {
                                return;
                            }


                            CommonUtils.instance.logd(TAG, "recordInfo deviceId = " + deviceId);
                            String recordInfoBody = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
                                    "    <Response>\n" +
                                    "      <CmdType>RecordInfo</CmdType>\n" +
                                    "      <SN>" + sn + "</SN>\n" +
                                    "      <DeviceID>" + deviceId + "</DeviceID>\n" +
                                    "      <SumNum>" + files.length + "</SumNum>\n" +
                                    "      <RecordList>\n" +
                                    getReordItemString(files, deviceId) +
                                    "      </RecordList>\n" +
                                    "    </Response>";
                            //endregion
                            Message recordInfoRequest = MessageFactory.createMessageRequest(sipProvider, message.getFromHeader().getNameAddress(),
                                    message.getToHeader().getNameAddress(), null, XMLUtil.XML_MANSCDP_TYPE, recordInfoBody);
                            recordInfoRequest.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(recordInfoRequest, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                        }

                    }
                }
            } else if (message.isInvite()) {
                if (message.hasBody()) {
                    String body = message.getBody();
                    SessionDescriptor sdp = new SessionDescriptor(body);
                    MediaDescriptor mediaDescriptor = sdp.getMediaDescriptors().firstElement();
                    String address = sdp.getConnection().getAddress();
                    int port = mediaDescriptor.getMedia().getPort();
                    String deviceId = sdp.getOrigin().getUserName();
                    CommonUtils.instance.loge(TAG_, "收到Invite请求，body=" + body + "deviceId=" + deviceId + "address=" + address + "port=" + port);

//                    if (devices.get(deviceId) == null) {
//                        deviceUtils = new DeviceUtils(deviceId);
//                        devices.put(deviceId, deviceUtils);
//                    } else {
//                        deviceUtils = devices.get(deviceId);
//                    }

                    switch (sdp.getSessionName().getValue().toLowerCase()) {
                        case "play":
                            DeviceUtils deviceUtils = new DeviceUtils(deviceId);
                            devices.put(deviceId, deviceUtils);
                            deviceUtils.setPlayBack(false);
                            String y = body.substring(body.indexOf("y=") + 2, body.indexOf("y=") + 12);
                            //region InviteResponseBody
                            String InviteResponseBody = "v=0\n" +
                                    "o=" + deviceId + " 0 0 IN IP4 " + GB28181Params.getLocalSIPIPAddress() + "\n" +
                                    "s=" + sdp.getSessionName().getValue() + "\n" +
                                    "c=IN IP4 " + GB28181Params.getLocalSIPIPAddress() + "\n" +
                                    "t=0 0\n" +
                                    "m=video " + port + " TCP/RTP/AVP 98\n" +
                                    "a=sendonly\n" +
                                    "a=rtpmap:98 H264/90000\n" +
                                    "a=fmtp:98 profile-level-id=640028\n" +
                                    "y=" + y + "";

                            //endregion
                            ssrc = Integer.parseInt(y);

                            deviceUtils.setServiceIp(address);
                            deviceUtils.setServicePort(port);
                            deviceUtils.setSsrc(Integer.parseInt(y));
//
                            if (Objects.requireNonNull(BaseApplication.Companion.getApplication()).getDeviceList() == null || BaseApplication.Companion.getApplication().getDeviceList().isEmpty()) {
                                String gb = gb28181.stringFromJNI();
                                if (!TextUtils.isEmpty(gb)) {
                                    boolean isSuccess = gb28181.initGB28181(null);
                                    deviceUtils.setGb28181(gb28181);
                                    CommonUtils.instance.logd(TAG, "gb28181 init :" + isSuccess + "_gb_" + gb);
                                }
                                if (deviceId.equals(GB28181Params.fontId)) {
                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG1.getValue(), (deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
//                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG1.getValue(), ("42.48.130.2" + "_" + "7057" + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
                                    CommonUtils.instance.logd(TAG, "send gb28181 font sip:" + deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc + "_isSuccess_" + isSuccess);
                                } else if (deviceId.equals(GB28181Params.backId)) {
                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG2.getValue(), (deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
                                    CommonUtils.instance.logd(TAG, "send gb28181 back sip:" + deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc + "_isSuccess_" + isSuccess);
                                } else if (deviceId.equals(GB28181Params.leftFontId)) {
                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG3.getValue(), (deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
                                    CommonUtils.instance.logd(TAG, "send gb28181 left font sip:" + deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc + "_isSuccess_" + isSuccess);
                                } else if (deviceId.equals(GB28181Params.leftBackId)) {
                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG4.getValue(), (deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
                                    CommonUtils.instance.logd(TAG, "send gb28181 left back sip:" + deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc + "_isSuccess_" + isSuccess);
                                } else if (deviceId.equals(GB28181Params.rightFontId)) {
                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG5.getValue(), (deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
                                    CommonUtils.instance.logd(TAG, "send gb28181 right font sip:" + deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc + "_isSuccess_" + isSuccess);
                                } else if (deviceId.equals(GB28181Params.rightBackId)) {
                                    boolean isSuccess = gb28181.inputData(GB28181.DataType.SIP_CONFIG6.getValue(), (deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc).getBytes(StandardCharsets.UTF_8));
                                    CommonUtils.instance.logd(TAG, "send gb28181 right back sip:" + deviceUtils.getServiceIp() + "_" + deviceUtils.getServicePort() + "_" + ssrc + "_isSuccess_" + isSuccess);
                                }
                            }
                            Log.i(TAG_, ":收到INVATE,ADDRESS=" + deviceUtils.getServiceIp() + ";port=" + deviceUtils.getServicePort() + "；ssrc=" + ssrc + "");
                            Message InviteResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), SipProvider.pickTag(),
                                    new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress())), "Application/Sdp", InviteResponseBody);
                            UserAgentHeader hander = new UserAgentHeader(GB28181Params.defaultUserAgent);
                            InviteResponse.setUserAgentHeader(hander);
                            sipProvider.sendMessage(InviteResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                            break;
                        case "playback":
                            DeviceUtils deviceUtil = new DeviceUtils(deviceId);
                            devices.put(deviceId, deviceUtil);
                            deviceUtil.setPlayBack(true);
                            String startTime = sdp.getTime().getStartTime();
                            deviceUtil.setStartTime(startTime);
                            String yy = body.substring(body.indexOf("y=") + 2, body.indexOf("y=") + 12);
                            //region InviteResponseBody
                            String InviteBackResponseBody = "v=0\n" +
                                    "o=" + deviceId + " 0 0 IN IP4 " + GB28181Params.getLocalSIPIPAddress() + "\n" +
                                    "s=Playback" + "\n" +
                                    "c=IN IP4 " + GB28181Params.getLocalSIPIPAddress() + "\n" +
                                    "t=0 0\n" +
                                    "m=video " + port + " TCP/RTP/AVP 98\n" +
                                    "a=sendonly\n" +
                                    "a=rtpmap:98 H264/90000\n" +
                                    "a=fmtp:98 profile-level-id=640028\n" +
                                    "y=" + yy + "";
                            deviceUtil.setServiceIp(address);
                            deviceUtil.setServicePort(port);
                            deviceUtil.setSsrc(Integer.parseInt(yy));

                            ssrc = Integer.parseInt(yy);
                            Log.i(TAG_, ":收到 playback INVATE,ADDRESS=" + deviceUtil.getServiceIp() + ";port=" + deviceUtil.getServicePort() + "；ssrc=" + ssrc + "");
                            Message InviteBackResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), SipProvider.pickTag(),
                                    new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress())), "Application/Sdp", InviteBackResponseBody);
                            UserAgentHeader handerBack = new UserAgentHeader(GB28181Params.defaultUserAgent);
                            InviteBackResponse.setUserAgentHeader(handerBack);
                            sipProvider.sendMessage(InviteBackResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                            break;
                        case "download":
                            break;
                    }
                }
            } else if (message.isAck()) {
                int index = message.toString().indexOf(subString);
                String deviceId = message.toString().substring(index, index + subString.length() + 1);
                DeviceUtils device = devices.get(deviceId);

                if (device.getState() == 0 && !device.isPlayBack()) {
                    if (Objects.requireNonNull(BaseApplication.Companion.getApplication()).getDeviceList() == null || BaseApplication.Companion.getApplication().getDeviceList().isEmpty()) {
                        setVideoDataCallBack(device);
                    }
                    device.setState(1);
//                    GB28181Params.setCurDevicePlayMediaState(1);
//                    GB28181Params.setCameraState(GB28181Params.getCameraState() + 1);
                } else {
                    device.setPlayBackState(1);
                    File file = findFile(device);
                    if (file == null) {
                        return;
                    }
                    HyperSdkUtils.pushStream(file.getAbsolutePath(), device.getSsrc(), 99, device.getServiceIp(), device.getServicePort(), new ExecuteCallback() {
                        @Override
                        public void apply(long executionId, int returnCode) {
                        }
                    });
                }
                CommonUtils.instance.loge(TAG_, "ack deviceId =" + deviceId + " device.setState = " + device.getState() + " isPlayback =" + device.isPlayBack());
            } else if (message.isBye()) {
                int index = message.toString().indexOf(subString);
                String deviceId = message.toString().substring(index, index + subString.length() + 1);

                DeviceUtils device = devices.get(deviceId);

                CommonUtils.instance.loge(TAG_, "收到BYE请求，deviceId = " + deviceId);
                if (device != null && device.getState() == 1) {
                    device.setState(0);
//                    device.getTread().interrupt();
                    device.getQueue().clear();
                    if (device.getStream() != null && device.getGb28181() == null) {
                        device.getStream().stopStreaming();
//                        device.getTread().interrupt();
                        device.setStream(null);
                        devices.remove(deviceId);
                        CommonUtils.instance.logd(TAG, "停止播放 devices.size() = " + devices.size());
                    } else {
                        device.getGb28181().closeGB28181();
                        CommonUtils.instance.logd(TAG, "停止播放");
                    }
                }

                //200 OK
                Message ByeResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), null);
                ByeResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                sipProvider.sendMessage(ByeResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
//                }
            }
        }


    }

    private File findFile(DeviceUtils device) {
        long startTime = Long.parseLong(device.getStartTime());
        String day = new SimpleDateFormat("yyyy-MM-dd", Locale.getDefault())
                .format(new Date(startTime * 1000));

        // 将 LocalDateTime 对象格式化为字符串
        String formattedDate = new SimpleDateFormat("yyyyMMddHHmmss", Locale.getDefault())
                .format(new Date(startTime * 1000));
        File[] files = getRecordInfoList(device.getId(), day);
        for (int i = 0; i < files.length; i++) {
            if (files[i].getName().contains(formattedDate)) {
                return files[i];
            }
        }
        return null;
    }

    private String getReordItemString(File[] files, String deviceId) {
        String recodeItemsString = "";
        for (int i = 0; i < files.length; i++) {
            File file = files[i];
            recodeItemsString = recodeItemsString +
                    "        <Item>\n" +
                    "          <DeviceID>" + deviceId + "</DeviceID>\n" +
                    "          <Name>" + file.getName() + "</Name>\n" +
                    "          <FilePath>" + file.getAbsolutePath() + "</FilePath>\n" +
                    "          <StartTime>" + CommonUtils.instance.getISOTime(file.getName().replace(Constact.VIDEO, "")) + "</StartTime>\n" +
                    "          <EndTime>" + CommonUtils.instance.getISOTime(file.getName().replace(Constact.VIDEO, "")) + "</EndTime>\n" +
                    "          <FileSize>" + file.length() + "</FileSize>\n" +
                    "        </Item>\n";
        }
        return recodeItemsString;
    }

    private File[] getRecordInfoList(String deviceId, String day) {
        File directory = null;
        if (deviceId.equals(GB28181Params.getFontId())) {
            directory = HyperSdkUtils.createChildWithChildFolder(getApplicationContext().getApplicationContext(), Constact.MODE_RECORD, day, Constact.FRONT);
        } else if (deviceId.equals(GB28181Params.getBackId())) {
            directory = HyperSdkUtils.createChildWithChildFolder(getApplicationContext().getApplicationContext(), Constact.MODE_RECORD, day, Constact.BACK);
        } else if (deviceId.equals(GB28181Params.getLeftBackId())) {
            directory = HyperSdkUtils.createChildWithChildFolder(getApplicationContext().getApplicationContext(), Constact.MODE_RECORD, day, Constact.LEFT_TWO);
        } else if (deviceId.equals(GB28181Params.getLeftFontId())) {
            directory = HyperSdkUtils.createChildWithChildFolder(getApplicationContext().getApplicationContext(), Constact.MODE_RECORD, day, Constact.LEFT_ONE);
        } else if (deviceId.equals(GB28181Params.getRightFontId())) {
            directory = HyperSdkUtils.createChildWithChildFolder(getApplicationContext().getApplicationContext(), Constact.MODE_RECORD, day, Constact.RIGHT_ONE);
        } else if (deviceId.equals(GB28181Params.getRightBackId())) {
            directory = HyperSdkUtils.createChildWithChildFolder(getApplicationContext().getApplicationContext(), Constact.MODE_RECORD, day, Constact.RIGHT_TWO);
        }

        if (directory != null) {
            return directory.listFiles();
        } else {
            return null;
        }

    }
//endregion

    public static class VideoData {
        private byte[] data;
        private boolean isKeyFrame;
        private String deviceId;

        public String getDeviceId() {
            return deviceId;
        }

        public void setDeviceId(String deviceId) {
            this.deviceId = deviceId;
        }

        public byte[] getData() {
            return data;
        }

        public void setData(byte[] data) {
            this.data = data;
        }

        public boolean isKeyFrame() {
            return isKeyFrame;
        }

        public void setKeyFrame(boolean keyFrame) {
            isKeyFrame = keyFrame;
        }
    }

}


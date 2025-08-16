//
// Created by bxc on 2022/12/15.
//
#include "RtpSendPs.h"
#include <android/log.h>

#define LOG_TAG "GB28181-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[GB28181-JNI] " __VA_ARGS__)

#ifdef WIN32
#include <WS2tcpip.h>
#endif 


namespace BXC {

	RtpSendPs::RtpSendPs(const char* rtpServerIp, int rtpServerPort, int localRtpPort) :
		mRtpServerIp(rtpServerIp), mRtpServerPort(rtpServerPort), mLocalRtpPort(localRtpPort), mQuit(true), mNaluQueue(nullptr) {

		if ((mSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			LOGE("--------");
			return;
		}
		int ret;
		// 为udp的socket绑定指定IP或指定端口 start
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(mLocalRtpPort);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(mSockFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
			LOGE("bind error");
			return;
		}
	}

	RtpSendPs::RtpSendPs(const char* rtpServerIp, int rtpServerPort, int localRtpPort, ConcurrentQueueA<std::shared_ptr<NALUData>>* naluQueue) :
		mRtpServerIp(rtpServerIp), mRtpServerPort(rtpServerPort), mLocalRtpPort(localRtpPort), mQuit(true), mNaluQueue(naluQueue) {

		LOGI("Initializing RtpSendPs: target=%s:%d, local=%d", rtpServerIp, rtpServerPort, localRtpPort);
		
		if ((mSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			LOGE("Failed to create UDP socket");
			return;
		}
		
		LOGI("UDP socket created successfully: fd=%d", mSockFd);
		
		int ret;
		// 为udp的socket绑定指定IP或指定端口 start
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(mLocalRtpPort);
		//        addr.sin_addr.s_addr = inet_addr("localRtpIp");
		addr.sin_addr.s_addr = INADDR_ANY;
		socklen_t addr_len = sizeof(struct sockaddr_in);

		LOGI("Binding socket to local port: %d", mLocalRtpPort);
		
		ret = bind(mSockFd, (struct sockaddr*)&addr, addr_len);
		if (ret < 0)
		{
			LOGE("Failed to bind socket to port %d", mLocalRtpPort);
			return;
		}
		
		LOGI("Socket bound successfully to port %d", mLocalRtpPort);
		// 为udp的socket绑定指定IP或指定端口 end

#ifndef WIN32 // Linux系统
		int ul = 1;
		ret = ioctl(mSockFd, FIONBIO, &ul); //设置为非阻塞模式

		if (ret == -1) {
			LOGE("Failed to set socket non-blocking mode");
			return;
		}
		
		LOGI("Socket set to non-blocking mode");
#else
		unsigned long ul = 1;
		ret = ioctlsocket(mSockFd, FIONBIO, (unsigned long*)&ul);//设置非阻塞

		if (ret == SOCKET_ERROR) {
			LOGE("Failed to set socket non-blocking mode");
			return;
		}
		
		LOGI("Socket set to non-blocking mode");
#endif // !WIN32

		mQuit = false;
		LOGI("RtpSendPs initialization completed successfully");
	}
	RtpSendPs::~RtpSendPs() {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (mThread) {
			mThread->join();
			delete mThread;
			mThread = nullptr;
		}

		if (mSockFd > -1) {
#ifndef WIN32 // Linux系统
			close(mSockFd);
#else
			closesocket(mSockFd);
#endif // !WIN32
			mSockFd = -1;
		}
	}

	int RtpSendPs::findStartCode(unsigned char* buf, int zeros_in_startcode)
	{
		int info;
		int i;

		info = 1;
		for (i = 0; i < zeros_in_startcode; i++)
			if (buf[i] != 0)
				info = 0;

		if (buf[i] != 1)
			info = 0;
		return info;
	}
	int RtpSendPs::getNextNalu(FILE* inpf, unsigned char* buf)
	{
		int pos = 0;
		int startCodeFound = 0;
		int info2 = 0;
		int info3 = 0;

		while (!feof(inpf) && (buf[pos++] = fgetc(inpf)) == 0); // fgetc:读取成功时返回读取到的字符，读取到文件末尾或读取失败时返回EOF

		while (!startCodeFound)
		{
			if (feof(inpf))   //feof:其功能是检测流上的文件结束符，如果文件结束，则返回非0值，否则返回0
			{
				return pos - 1;
			}
			buf[pos++] = fgetc(inpf);
			info3 = findStartCode(&buf[pos - 4], 3);
			startCodeFound = (info3 == 1);
			if (info3 != 1)
				info2 = findStartCode(&buf[pos - 3], 2);
			startCodeFound = (info2 == 1 || info3 == 1);
		}
		if (info2)
		{
			fseek(inpf, -3, SEEK_CUR);  //fseek:重定位流(数据流/文件)上的文件内部位置指针 返回值：成功，返回0，失败返回非0值，并设置error的值  SEEK_CUR	1
			return pos - 3;
		}
		if (info3)
		{
			fseek(inpf, -4, SEEK_CUR);
			return pos - 4;
		}

		return 0;
	}

	/***
	*@remark:  音视频数据的打包成ps流，并封装成rtp
	*@param :  pData      [in] 需要发送的音视频数据
	*          nFrameLen  [in] 发送数据的长度
	*          pPacker    [in] 数据包的一些信息，包括时间戳，rtp数据buff，发送的socket相关信息
	*          stream_type[in] 数据类型 0 视频 1 音频
	*@return:  0 success others failed
	*/

	int RtpSendPs::gb28181_streampackageForH264(char* pData, int nFrameLen, Data_Info_s* pPacker, int stream_type)
	{

		char szTempPacketHead[256];
		int  nSizePos = 0;
		int  nSize = 0;
		char* pBuff = NULL;
		memset(szTempPacketHead, 0, 256);
		// 1 package for ps header
		gb28181_make_ps_header(szTempPacketHead + nSizePos, pPacker->s64CurPts);
		nSizePos += PS_HDR_LEN;
		//2 system header
		//if (pPacker->IFrame == 1)
		{
			// 如果是I帧的话，则添加系统头
			gb28181_make_sys_header(szTempPacketHead + nSizePos);
			nSizePos += SYS_HDR_LEN;
			//这个地方我是不管是I帧还是p帧都加上了map的，貌似只是I帧加也没有问题

			gb28181_make_psm_header(szTempPacketHead + nSizePos);
			nSizePos += PSM_HDR_LEN;

		}

		//加上rtp发送出去，这样的话，后面的数据就只要分片分包就只有加上pes头和rtp头了
		if (gb28181_send_rtp_pack(szTempPacketHead, nSizePos, 0, pPacker) != 0)
			return -1;

		// 这里向后移动是为了方便拷贝pes头
		//这里是为了减少后面音视频裸数据的大量拷贝浪费空间，所以这里就向后移动，在实际处理的时候，要注意地址是否越界以及覆盖等问题
		pBuff = pData - PES_HDR_LEN;
		while (nFrameLen > 0)
		{
			//pes帧的长度不要超过short类型，超过就需要分片循环行发送
			nSize = (nFrameLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : nFrameLen;
			// 添加pes头
			gb28181_make_pes_header(pBuff, stream_type ? 0xC0 : 0xE0, nSize, pPacker->s64CurPts, pPacker->s64CurPts);

			//最后在添加rtp头并发送数据
			if (gb28181_send_rtp_pack(pBuff, nSize + PES_HDR_LEN, ((nSize == nFrameLen) ? 1 : 0), pPacker) != 0)
			{
				LOGE("gb28181_send_rtp_pack error");
				return -1;
			}
			//分片后每次发送的数据移动指针操作
			nFrameLen -= nSize;
			//这里也只移动nSize,因为在while向后移动的pes头长度，正好重新填充pes头数据
			pBuff += nSize;

		}
		return 0;
	}

	/***
	*@remark:   ps头的封装,里面的具体数据的填写已经占位，可以参考标准
	*@param :   pData  [in] 填充ps头数据的地址
	*           s64Src [in] 时间戳
	*@return:   0 success, others failed
	*/
	int RtpSendPs::gb28181_make_ps_header(char* pData, unsigned long long s64Scr)
	{
		unsigned long long lScrExt = (s64Scr) % 100;
		//s64Scr = s64Scr / 100;

		// 这里除以100是由于sdp协议返回的video的频率是90000，帧率是25帧/s，所以每次递增的量是3600,
		// 所以实际你应该根据你自己编码里的时间戳来处理以保证时间戳的增量为3600即可，
		//如果这里不对的话，就可能导致卡顿现象了
		bits_buffer_s   bitsBuffer;
		bitsBuffer.i_size = PS_HDR_LEN;
		bitsBuffer.i_data = 0;
		bitsBuffer.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
		bitsBuffer.p_data = (unsigned char*)(pData);
		memset(bitsBuffer.p_data, 0, PS_HDR_LEN);
		bits_write(&bitsBuffer, 32, 0x000001BA);      /*start codes*/
		bits_write(&bitsBuffer, 2, 1);           /*marker bits '01b'*/
		bits_write(&bitsBuffer, 3, (s64Scr >> 30) & 0x07);     /*System clock [32..30]*/
		bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
		bits_write(&bitsBuffer, 15, (s64Scr >> 15) & 0x7FFF);   /*System clock [29..15]*/
		bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
		bits_write(&bitsBuffer, 15, s64Scr & 0x7fff);         /*System clock [14..0]*/
		bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
		bits_write(&bitsBuffer, 9, lScrExt & 0x01ff);    /*System clock ext*/
		bits_write(&bitsBuffer, 1, 1);           /*marker bit*/
		bits_write(&bitsBuffer, 22, (255) & 0x3fffff);    /*bit rate(n units of 50 bytes per second.)*/
		bits_write(&bitsBuffer, 2, 3);           /*marker bits '11'*/
		bits_write(&bitsBuffer, 5, 0x1f);          /*reserved(reserved for future use)*/
		bits_write(&bitsBuffer, 3, 0);           /*stuffing length*/
		return 0;
	}

	/***
	*@remark:   sys头的封装,里面的具体数据的填写已经占位，可以参考标准
	*@param :   pData  [in] 填充ps头数据的地址
	*@return:   0 success, others failed
	*/
	int RtpSendPs::gb28181_make_sys_header(char* pData)
	{

		bits_buffer_s   bitsBuffer;
		bitsBuffer.i_size = SYS_HDR_LEN;
		bitsBuffer.i_data = 0;
		bitsBuffer.i_mask = 0x80;
		bitsBuffer.p_data = (unsigned char*)(pData);
		memset(bitsBuffer.p_data, 0, SYS_HDR_LEN);
		/*system header*/
		bits_write(&bitsBuffer, 32, 0x000001BB); /*start code*/
		bits_write(&bitsBuffer, 16, SYS_HDR_LEN - 6);/*header_length 表示次字节后面的长度，后面的相关头也是次意思*/
		bits_write(&bitsBuffer, 1, 1);            /*marker_bit*/
		bits_write(&bitsBuffer, 22, 50000);    /*rate_bound*/
		bits_write(&bitsBuffer, 1, 1);            /*marker_bit*/
		bits_write(&bitsBuffer, 6, 1);            /*audio_bound*/
		bits_write(&bitsBuffer, 1, 0);            /*fixed_flag */
		bits_write(&bitsBuffer, 1, 1);          /*CSPS_flag */
		bits_write(&bitsBuffer, 1, 1);          /*system_audio_lock_flag*/
		bits_write(&bitsBuffer, 1, 1);          /*system_video_lock_flag*/
		bits_write(&bitsBuffer, 1, 1);          /*marker_bit*/
		bits_write(&bitsBuffer, 5, 1);          /*video_bound*/
		bits_write(&bitsBuffer, 1, 0);          /*dif from mpeg1*/
		bits_write(&bitsBuffer, 7, 0x7F);       /*reserver*/
		/*audio stream bound*/
		bits_write(&bitsBuffer, 8, 0xC0);         /*stream_id*/
		bits_write(&bitsBuffer, 2, 3);          /*marker_bit */
		bits_write(&bitsBuffer, 1, 0);            /*PSTD_buffer_bound_scale*/
		bits_write(&bitsBuffer, 13, 512);          /*PSTD_buffer_size_bound*/
		/*video stream bound*/
		bits_write(&bitsBuffer, 8, 0xE0);         /*stream_id*/
		bits_write(&bitsBuffer, 2, 3);          /*marker_bit */
		bits_write(&bitsBuffer, 1, 1);          /*PSTD_buffer_bound_scale*/
		bits_write(&bitsBuffer, 13, 2048);       /*PSTD_buffer_size_bound*/
		return 0;
	}

	/***
	*@remark:   psm头的封装,里面的具体数据的填写已经占位，可以参考标准
	*@param :   pData  [in] 填充ps头数据的地址
	*@return:   0 success, others failed
	*/
	int RtpSendPs::gb28181_make_psm_header(char* pData)
	{

		bits_buffer_s   bitsBuffer;
		bitsBuffer.i_size = PSM_HDR_LEN;
		bitsBuffer.i_data = 0;
		bitsBuffer.i_mask = 0x80;
		bitsBuffer.p_data = (unsigned char*)(pData);
		memset(bitsBuffer.p_data, 0, PSM_HDR_LEN);
		bits_write(&bitsBuffer, 24, 0x000001); /*start code*/
		bits_write(&bitsBuffer, 8, 0xBC);   /*map stream id*/
		bits_write(&bitsBuffer, 16, 18);     /*program stream map length*/
		bits_write(&bitsBuffer, 1, 1);      /*current next indicator */
		bits_write(&bitsBuffer, 2, 3);      /*reserved*/
		bits_write(&bitsBuffer, 5, 0);      /*program stream map version*/
		bits_write(&bitsBuffer, 7, 0x7F);   /*reserved */
		bits_write(&bitsBuffer, 1, 1);      /*marker bit */
		bits_write(&bitsBuffer, 16, 0);      /*programe stream info length*/
		bits_write(&bitsBuffer, 16, 8);     /*elementary stream map length  is*/
		/*audio*/
		bits_write(&bitsBuffer, 8, 0x90);       /*stream_type*/
		bits_write(&bitsBuffer, 8, 0xC0);   /*elementary_stream_id*/
		bits_write(&bitsBuffer, 16, 0);     /*elementary_stream_info_length is*/
		/*video*/
		bits_write(&bitsBuffer, 8, 0x1B);       /*stream_type*/
		bits_write(&bitsBuffer, 8, 0xE0);   /*elementary_stream_id*/
		bits_write(&bitsBuffer, 16, 0);     /*elementary_stream_info_length */
		/*crc (2e b9 0f 3d)*/
		bits_write(&bitsBuffer, 8, 0x45);   /*crc (24~31) bits*/
		bits_write(&bitsBuffer, 8, 0xBD);   /*crc (16~23) bits*/
		bits_write(&bitsBuffer, 8, 0xDC);   /*crc (8~15) bits*/
		bits_write(&bitsBuffer, 8, 0xF4);   /*crc (0~7) bits*/
		return 0;
	}

	/***
	*@remark:   pes头的封装,里面的具体数据的填写已经占位，可以参考标准
	*@param :   pData      [in] 填充ps头数据的地址
	*           stream_id  [in] 码流类型
	*           paylaod_len[in] 负载长度
	*           pts        [in] 时间戳
	*           dts        [in]
	*@return:   0 success, others failed
	*/
	int RtpSendPs::gb28181_make_pes_header(char* pData, int stream_id, int payload_len, unsigned long long pts, unsigned long long dts)
	{

		bits_buffer_s   bitsBuffer;
		bitsBuffer.i_size = PES_HDR_LEN;
		bitsBuffer.i_data = 0;
		bitsBuffer.i_mask = 0x80;
		bitsBuffer.p_data = (unsigned char*)(pData);
		memset(bitsBuffer.p_data, 0, PES_HDR_LEN);
		/*system header*/
		bits_write(&bitsBuffer, 24, 0x000001);  /*start code*/
		bits_write(&bitsBuffer, 8, (stream_id)); /*streamID*/
		bits_write(&bitsBuffer, 16, (payload_len)+13);  /*packet_len*/ //指出pes分组中数据长度和该字节后的长度和
		bits_write(&bitsBuffer, 2, 2);    /*'10'*/
		bits_write(&bitsBuffer, 2, 0);    /*scrambling_control*/
		bits_write(&bitsBuffer, 1, 0);    /*priority*/
		bits_write(&bitsBuffer, 1, 0);    /*data_alignment_indicator*/
		bits_write(&bitsBuffer, 1, 0);    /*copyright*/
		bits_write(&bitsBuffer, 1, 0);    /*original_or_copy*/
		bits_write(&bitsBuffer, 1, 1);    /*PTS_flag*/
		bits_write(&bitsBuffer, 1, 1);    /*DTS_flag*/
		bits_write(&bitsBuffer, 1, 0);    /*ESCR_flag*/
		bits_write(&bitsBuffer, 1, 0);    /*ES_rate_flag*/
		bits_write(&bitsBuffer, 1, 0);    /*DSM_trick_mode_flag*/
		bits_write(&bitsBuffer, 1, 0);    /*additional_copy_info_flag*/
		bits_write(&bitsBuffer, 1, 0);    /*PES_CRC_flag*/
		bits_write(&bitsBuffer, 1, 0);    /*PES_extension_flag*/
		bits_write(&bitsBuffer, 8, 10);    /*header_data_length*/
		// 指出包含在 PES 分组标题中的可选字段和任何填充字节所占用的总字节数。该字段之前
		//的字节指出了有无可选字段。

		/*PTS,DTS*/
		bits_write(&bitsBuffer, 4, 3);                    /*'0011'*/
		bits_write(&bitsBuffer, 3, ((pts) >> 30) & 0x07);     /*PTS[32..30]*/
		bits_write(&bitsBuffer, 1, 1);
		bits_write(&bitsBuffer, 15, ((pts) >> 15) & 0x7FFF);    /*PTS[29..15]*/
		bits_write(&bitsBuffer, 1, 1);
		bits_write(&bitsBuffer, 15, (pts) & 0x7FFF);          /*PTS[14..0]*/
		bits_write(&bitsBuffer, 1, 1);
		bits_write(&bitsBuffer, 4, 1);                    /*'0001'*/
		bits_write(&bitsBuffer, 3, ((dts) >> 30) & 0x07);     /*DTS[32..30]*/
		bits_write(&bitsBuffer, 1, 1);
		bits_write(&bitsBuffer, 15, ((dts) >> 15) & 0x7FFF);    /*DTS[29..15]*/
		bits_write(&bitsBuffer, 1, 1);
		bits_write(&bitsBuffer, 15, (dts) & 0x7FFF);          /*DTS[14..0]*/
		bits_write(&bitsBuffer, 1, 1);
		return 0;
	}

	/***
	*@remark:   rtp头的打包，并循环发送数据
	*@param :   pData      [in] 发送的数据地址
	*           nDatalen   [in] 发送数据的长度
	*           mark_flag  [in] mark标志位
	*           curpts     [in] 时间戳
	*           pPacker    [in] 数据包的基本信息
	*@return:   0 success, others failed
	*/
	int RtpSendPs::gb28181_send_rtp_pack(char* databuff, int nDataLen, int mark_flag, Data_Info_s* pPacker)
	{
		//LOGD("Sending RTP pack: dataLen=%d, mark=%d, seq=%d, pts=%llu, ssrc=%u", 
		 //nDataLen, mark_flag, pPacker->u16CSeq + 1, (unsigned long long)pPacker->s64CurPts, pPacker->u32Ssrc);
		
		int nRes = 0;
		int nPlayLoadLen = 0;
		int nSendSize = 0;
		char szRtpHdr[RTP_HDR_LEN];
		memset(szRtpHdr, 0, RTP_HDR_LEN);

		if (nDataLen + RTP_HDR_LEN <= RTP_MAX_PACKET_BUFF)// 1460 pPacker指针本来有一个1460大小的buffer数据缓存
		{
			//LOGD("Single RTP packet mode: total size=%d (data=%d + header=%d)", 
			//	 nDataLen + RTP_HDR_LEN, nDataLen, RTP_HDR_LEN);
			
			// 一帧数据发送完后，给mark标志位置1
			gb28181_make_rtp_header(szRtpHdr, ((mark_flag == 1) ? 1 : 0), ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
			memcpy(pPacker->szBuff, szRtpHdr, RTP_HDR_LEN);
			memcpy(pPacker->szBuff + RTP_HDR_LEN, databuff, nDataLen);
			nRes = SendDataBuff(pPacker->szBuff, nDataLen + RTP_HDR_LEN);
			if (nRes != (RTP_HDR_LEN + nDataLen))
			{
				LOGE("Single RTP packet send failed: expected=%d, actual=%d", RTP_HDR_LEN + nDataLen, nRes);
				return -1;
			} else {
				//LOGD("Single RTP packet sent successfully: %d bytes", nRes);
			}

		}
		else
		{
			int totalPackets = (nDataLen + nPlayLoadLen - 1) / (RTP_MAX_PACKET_BUFF - RTP_HDR_LEN);
			//LOGI("Fragmented RTP mode: dataLen=%d, estimated packets=%d", nDataLen, totalPackets);
			
			nPlayLoadLen = RTP_MAX_PACKET_BUFF - RTP_HDR_LEN; // 每次只能发送的数据长度 除去rtp头
			int packetCount = 1;
			
			//LOGD("Sending fragment %d: payload=%d bytes", packetCount, nPlayLoadLen);
			gb28181_make_rtp_header(pPacker->szBuff, 0, ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
			memcpy(pPacker->szBuff + RTP_HDR_LEN, databuff, nPlayLoadLen);
			nRes = SendDataBuff(pPacker->szBuff, RTP_HDR_LEN + nPlayLoadLen);
			if (nRes != (RTP_HDR_LEN + nPlayLoadLen))
			{
				LOGE("Fragment %d send failed: expected=%d, actual=%d", packetCount, RTP_HDR_LEN + nPlayLoadLen, nRes);
				return -1;
			} else {
				//LOGD("Fragment %d sent successfully: %d bytes", packetCount, nRes);
			}

			nDataLen -= nPlayLoadLen;
			// databuff += (nPlayLoadLen - RTP_HDR_LEN);
			databuff += nPlayLoadLen; // 表明前面到数据已经发送出去
			databuff -= RTP_HDR_LEN; // 用来存放rtp头
			
			while (nDataLen > 0)
			{
				packetCount++;
				
				if (nDataLen <= nPlayLoadLen)
				{
					//一帧数据发送完，置mark标志位
					//LOGD("Sending final fragment %d: payload=%d bytes, mark=1", packetCount, nDataLen);
					gb28181_make_rtp_header(databuff, mark_flag, ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
					nSendSize = nDataLen;
				}
				else
				{
					//LOGD("Sending fragment %d: payload=%d bytes", packetCount, nPlayLoadLen);
					gb28181_make_rtp_header(databuff, 0, ++pPacker->u16CSeq, pPacker->s64CurPts, pPacker->u32Ssrc);
					nSendSize = nPlayLoadLen;
				}

				nRes = SendDataBuff(databuff, RTP_HDR_LEN + nSendSize);
				if (nRes != (RTP_HDR_LEN + nSendSize))
				{
					LOGE("Fragment %d send failed: expected=%d, actual=%d", packetCount, RTP_HDR_LEN + nSendSize, nRes);
					return -1;
				} else {
					//LOGD("Fragment %d sent successfully: %d bytes", packetCount, nRes);
				}
				
				nDataLen -= nSendSize;
				databuff += nSendSize;
				//因为buffer指针已经向后移动一次rtp头长度后，
				//所以每次循环发送rtp包时，只要向前移动裸数据到长度即可，这是buffer指针实际指向到位置是
				//databuff向后重复的rtp长度的裸数据到位置上

			}
			
			//LOGI("Fragmented RTP transmission completed: total %d packets sent", packetCount);

		}
		return 0;
	}

	/**
	 * @remark 设置rtp头
	 * @param pData
	 * @param marker_flag
	 * @param cseq
	 * @param curpts
	 * @param ssrc
	 * @return
	 */
	int RtpSendPs::gb28181_make_rtp_header(char* pData, int marker_flag, unsigned short cseq, long long curpts, unsigned int ssrc)
	{
		bits_buffer_s   bitsBuffer;
		if (pData == NULL)
			return -1;
		bitsBuffer.i_size = RTP_HDR_LEN;
		bitsBuffer.i_data = 0;
		bitsBuffer.i_mask = 0x80;
		bitsBuffer.p_data = (unsigned char*)(pData);
		memset(bitsBuffer.p_data, 0, RTP_HDR_LEN);
		bits_write(&bitsBuffer, 2, RTP_VERSION);  /* rtp version  */
		bits_write(&bitsBuffer, 1, 0);        /* rtp padding  */
		bits_write(&bitsBuffer, 1, 0);        /* rtp extension  */
		bits_write(&bitsBuffer, 4, 0);        /* rtp CSRC count */
		bits_write(&bitsBuffer, 1, (marker_flag));      /* rtp marker   */
		bits_write(&bitsBuffer, 7, 96);     /* rtp payload type*/
		bits_write(&bitsBuffer, 16, (cseq));      /* rtp sequence    */
		bits_write(&bitsBuffer, 32, (curpts));    /* rtp timestamp   */
		bits_write(&bitsBuffer, 32, (ssrc));    /* rtp SSRC    */
		return 0;
	}

	

	//发送数据包
	int RtpSendPs::SendDataBuff(char* buff, int size) {
	//	LOGD("Sending RTP packet: size=%d bytes to %s:%d", size, mRtpServerIp.c_str(), mRtpServerPort);
		
		/* 设置address */
		struct sockaddr_in addr_serv;
		int len;
		memset(&addr_serv, 0, sizeof(addr_serv));  //memset 在一段内存块中填充某个给定的值，它是对较大的结构体或数组进行清零操作的一种最快方法
		addr_serv.sin_family = AF_INET;
		addr_serv.sin_addr.s_addr = inet_addr(mRtpServerIp.c_str());
		addr_serv.sin_port = htons(mRtpServerPort);
		len = sizeof(addr_serv);

		int ret = sendto(mSockFd, buff, size, 0, (struct sockaddr*)&addr_serv, len);  //send函数专用于TCP链接，sendto函数专用与UDP连接。
		if (ret <= 0) {
			LOGE("UDP sendto failed: ret=%d, expected=%d, target=%s:%d", ret, size, mRtpServerIp.c_str(), mRtpServerPort);
		} else if (ret != size) {
			LOGW("UDP sendto partial: sent=%d, expected=%d", ret, size);
		} else {
			//LOGD("UDP sendto success: sent=%d bytes to %s:%d", ret, mRtpServerIp.c_str(), mRtpServerPort);
		}
		return ret;
	}

	//    RtpSendPs::~RtpSendPs() {
	//        stop(); // 确保线程停止
	//        if (mSockFd != -1) {
	//#ifdef WIN32
	//            closesocket(mSockFd);
	//#else
	//            close(mSockFd);
	//#endif
	//            mSockFd = -1;
	//        }
	//    }

	void RtpSendPs::setNaluQueue(ConcurrentQueueA<std::shared_ptr<NALUData>>* naluQueue) {
		mNaluQueue = naluQueue;
	}

	void RtpSendPs::SendDataThread(void* arg) {
		RtpSendPs* rtpSendPs = (RtpSendPs*)arg;
		LOGI("RTP SendDataThread started");

		Data_Info_s pPacker;
		pPacker.IFrame = 1;
		pPacker.u32Ssrc = 1234567890;//10进制的ssrc
		pPacker.s64CurPts = 0;
		
		LOGI("RTP Packer initialized - SSRC: %u, Initial PTS: %llu", pPacker.u32Ssrc, (unsigned long long)pPacker.s64CurPts);

		// 如果有队列，从队列中读取数据
		if (rtpSendPs->mNaluQueue != nullptr) {
			LOGI("Using NALU queue mode for RTP streaming");
			
			while (!rtpSendPs->mQuit) {
				std::shared_ptr<NALUData> naluData;
				if (rtpSendPs->mNaluQueue->pop(naluData, 100)) { // 100ms超时
					if (naluData && !naluData->data.empty()) {
						// 获取不包含起始码的数据
						//std::vector<uint8_t> dataWithoutStartCode = naluData->getDataWithoutStartCode();
						std::vector<uint8_t> dataWithoutStartCode = naluData->data;

						if (!dataWithoutStartCode.empty()) {
							//LOGI("Processing NALU data: type=%d, size=%zu bytes, PTS=%llu", 
							//	 naluData->type, dataWithoutStartCode.size(), (unsigned long long)pPacker.s64CurPts);
							
							// 分配缓冲区，预留PES头空间
							char* buf = (char*)malloc(dataWithoutStartCode.size() + PES_HDR_LEN);
							if (buf) {
								// 复制数据到缓冲区
								memcpy(buf + PES_HDR_LEN, dataWithoutStartCode.data(), dataWithoutStartCode.size());

								// 将NALU数据封装到PS并通过RTP推流
								int result = rtpSendPs->gb28181_streampackageForH264(buf + PES_HDR_LEN, dataWithoutStartCode.size(), &pPacker, 0);
								
								if (result == 0) {
									//LOGI("Successfully sent NALU data via RTP: type=%d, size=%zu, PTS=%llu", 
									//	 naluData->type, dataWithoutStartCode.size(), (unsigned long long)pPacker.s64CurPts);
								} else {
									LOGE("Failed to send NALU data via RTP: result=%d", result);
								}

								pPacker.s64CurPts += 3600;

								free(buf);
							} else {
								LOGE("Failed to allocate buffer for NALU data, size: %zu", dataWithoutStartCode.size() + PES_HDR_LEN);
							}
						} else {
							LOGW("Empty NALU data received");
						}
					} else {
						LOGW("Invalid NALU data received (null or empty)");
					}
					
				}
				else {
					//LOGD("No NALU data available in queue (timeout)");
				}

#ifndef WIN32
			//	usleep(40 * 1000);//函数的休眠单位是微秒，这里休眠40毫秒
#else
				// Sleep(40);//休眠40毫秒，考虑到我们准备的测试视频的fps是25，所以每一帧间隔40ms
#endif // !WIN32
			}
		}
		else {


			// 原有的文件读取模式
			const char* filename = "video.h264";
			FILE* fp = fopen(filename, "rb");
			if (!fp) {
				LOGE("fopen error:%s", filename);
				return;
			}
			char* buf = (char*)malloc(1024 * 1024);

			while (!rtpSendPs->mQuit) {
				int size = rtpSendPs->getNextNalu(fp, (unsigned char*)(buf + PES_HDR_LEN)); //PES_HDR_LEN 19   size：发送数据的长度
				if (size <= 0) {
					//LOGI("----");
					rtpSendPs->mQuit = true;
					break;
				}
				//LOGI("大小 %d", size);

				// 将h264码流读取到的一个一个nalu封装到ps并通过rtp推流
				rtpSendPs->gb28181_streampackageForH264(buf + PES_HDR_LEN, size, &pPacker, 0); //0 表示传递的是视频数据

				pPacker.s64CurPts += 3600;

#ifndef WIN32
				usleep(40 * 1000);//函数的休眠单位是微秒，这里休眠40毫秒
#else
				Sleep(40);//休眠40毫秒，考虑到我们准备的测试视频的fps是25，所以每一帧间隔40ms
#endif // !WIN32
			}

			free(buf);
			buf = nullptr;
			fclose(fp);
		}
	}
	void RtpSendPs::start()
	{
		LOGI("Starting RTP sender thread: target=%s:%d, local=%d", mRtpServerIp.c_str(), mRtpServerPort, mLocalRtpPort);
		
		mQuit = false; // 启动时设置为false
		mThread = new std::thread(RtpSendPs::SendDataThread, this);
		
		LOGI("RTP sender thread started successfully");
	}
	
	void RtpSendPs::stop() {
		LOGI("Stopping RTP sender thread");
		
		mQuit = true;
		if (mThread && mThread->joinable()) {
			LOGI("Waiting for RTP sender thread to finish");
			mThread->join();
			delete mThread;
			mThread = nullptr;
			LOGI("RTP sender thread stopped successfully");
		} else {
			LOGI("RTP sender thread was not running");
		}
	}

}

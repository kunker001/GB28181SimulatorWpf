#ifdef _WIN32

//#include <vld.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "EasyGB28181DeviceAPI.h"
#include "EasyStreamClientAPI.h"		// 启用或禁用该行
#include "xmlConfigChannel.h"

#ifdef _EASY_STREAM_CLIENT_API_H

#ifdef _WIN32
#pragma comment(lib, "libEasyStreamClient.lib")
#pragma comment(lib, "libEasyGBD.lib")
#endif
#else
#include <windows.h>
#include <process.h>
#include <timeapi.h>
#include "FileParser/ESFileParser.h"
#include "gettimeofdayEx.h"
#include "g711.h"
#pragma comment(lib, "winmm.lib")
#endif

typedef struct __CHANNEL_T
{
#ifdef _EASY_STREAM_CLIENT_API_H
    Easy_Handle     streamHandle;
#else
	OSTHREAD_OBJ_T* readVideoFileThread;
	OSTHREAD_OBJ_T* readAudioFileThread;
#endif
    char    url[260];

	GB28181_CLIENT_HANDLE pDeviceHandle;
    void* pusherHandle;
}CHANNEL_T;

#ifdef _EASY_STREAM_CLIENT_API_H
int Easy_APICALL __EasyStreamClientCallBack(void* _channelPtr, int _frameType, void* pBuf, EASY_FRAME_INFO* frameInfo)
{
	CHANNEL_T* pChannel = (CHANNEL_T*)_channelPtr;

    //printf("%s line[%d]  _frameType: %d\n", __FUNCTION__, __LINE__, _frameType);

	if (EASY_SDK_VIDEO_FRAME_FLAG == _frameType)
	{
        GB28181_MEDIA_FRAME_INFO_T  mediaFrameInfo;
        memset(&mediaFrameInfo, 0x00, sizeof(GB28181_MEDIA_FRAME_INFO_T));
        mediaFrameInfo.codecID = frameInfo->codec;
        mediaFrameInfo.frameType = frameInfo->type;
        mediaFrameInfo.frameSize = frameInfo->length;
        mediaFrameInfo.pBuffer = (unsigned char* )pBuf;
		//mediaFrameInfo.rtpTimestamp = frameInfo->timestamp_sec * 1000 + frameInfo->timestamp_usec / 1000;

		libGB28181Client_PushFrame(pChannel->pDeviceHandle, pChannel->pusherHandle, MEDIA_TYPE_VIDEO, &mediaFrameInfo, (char*)pBuf);
	}

	else if (EASY_SDK_AUDIO_FRAME_FLAG == _frameType)
	{
        GB28181_MEDIA_FRAME_INFO_T  mediaFrameInfo;
        memset(&mediaFrameInfo, 0x00, sizeof(GB28181_MEDIA_FRAME_INFO_T));
        mediaFrameInfo.codecID = frameInfo->codec;
        mediaFrameInfo.frameType = frameInfo->type;
        mediaFrameInfo.frameSize = frameInfo->length;
		mediaFrameInfo.samplerate = frameInfo->sample_rate;
		mediaFrameInfo.channels = frameInfo->channels;
		mediaFrameInfo.bitPerSample = frameInfo->bits_per_sample;
        mediaFrameInfo.pBuffer = (unsigned char*)pBuf;
		//mediaFrameInfo.rtpTimestamp = frameInfo->timestamp_sec * 1000 + frameInfo->timestamp_usec / 1000;

#ifdef _DEBUG__
        static FILE* f = fopen("1.aac", "wb");
        if (f)
        {
            fwrite(pBuf, 1, frameInfo->length, f);
        }
#endif

		libGB28181Client_PushFrame(pChannel->pDeviceHandle, pChannel->pusherHandle, MEDIA_TYPE_AUDIO, &mediaFrameInfo, (char*)pBuf);
	}
	else if (EASY_SDK_MEDIA_INFO_FLAG == _frameType)
	{
		EASY_MEDIA_INFO_T* mediainfo = (EASY_MEDIA_INFO_T*)pBuf;
		if (pBuf != NULL)
		{
			memcpy(mediainfo, pBuf, sizeof(EASY_MEDIA_INFO_T));

			GB28181_MEDIA_INFO_T	gbMediaInfo;
			memset(&gbMediaInfo, 0x00, sizeof(GB28181_MEDIA_INFO_T));

			gbMediaInfo.videoCodec = mediainfo->u32VideoCodec;
			gbMediaInfo.audioCodec = mediainfo->u32AudioCodec;
			gbMediaInfo.audioSampleRate = mediainfo->u32AudioSamplerate;
			gbMediaInfo.audioBitsPerSample = mediainfo->u32AudioBitsPerSample;
			gbMediaInfo.audioChannel = mediainfo->u32AudioChannel;

			libGB28181Client_SetMediaInfo(pChannel->pDeviceHandle, pChannel->pusherHandle, &gbMediaInfo);

#if 0
			printf("Source;[%s] Get MediaInfo: video:%u fps:%u audio:%u channel:%u sampleRate:%u spslen: %d ppslen:%d\n",
				pChannel->url,
				mediainfo->u32VideoCodec, mediainfo->u32VideoFps,
				mediainfo->u32AudioCodec, mediainfo->u32AudioChannel, mediainfo->u32AudioSamplerate,
				mediainfo->u32SpsLength, mediainfo->u32PpsLength);
#endif
		}
	}

    //printf("%s line[%d]\n", __FUNCTION__, __LINE__);

	return 0;
}

#else

void SleepEx(int ms)
{
#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	Sleep(ms);

#ifdef _WIN32
	timeEndPeriod(1);
#endif
}

int		videoCodecID = 0;
int		audioCodecID = 0;

uint64_t u64_max_value = 0xFFFFFFFFFFFFFFFF;		// uint64_t最大值
uint64_t u64_max_dts = u64_max_value / 10000;		// 因为库中还要再乘10000,所以此处使用的dts为uint64_t最大值除以10000
uint64_t u64_init_dts = 0;							// dts 初始值
uint64_t videoPTS = u64_init_dts;					// 视频时间戳
uint64_t audioDTS = u64_init_dts;					// 音频时间戳
#ifdef _WIN32
DWORD WINAPI __ReadVideoFileThread(void* lpParam)
#else
void* __ReadVideoFileThread(void* lpParam)
#endif
{
	OSTHREAD_OBJ_T* pThread = (OSTHREAD_OBJ_T*)lpParam;
	CHANNEL_T* pChannel = (CHANNEL_T*)pThread->userPtr;

	pThread->flag = THREAD_STATUS_RUNNING;

	printf("read video file thread startup. [%d]\n", pThread->customId);

	// 读文件
	ESFileParser	esFileParse;
	if (0 == esFileParse.OpenEsFile("1M.h264", true))
	{
		videoCodecID = esFileParse.GetVideoCodec();

		int interval_ms = 40;

		struct timeval tvStartTime = { 0,0 };
		while (1)
		{
			if (pThread->flag == THREAD_STATUS_EXIT)			break;

			if (audioDTS < 1)		// 如果还未读到音频,则等待
			{
				SleepEx(1);
				continue;
			}

			char* frameData = NULL;
			int frameSize = 0;
			int frameType = 0;
			if (esFileParse.ReadFrame(&frameData, &frameSize, &frameType) == 0)
			{
				videoPTS += interval_ms;
				if (videoPTS > u64_max_dts)
				{
					videoPTS = interval_ms;
				}

				GB28181_MEDIA_FRAME_INFO_T  mediaFrameInfo;
				memset(&mediaFrameInfo, 0x00, sizeof(GB28181_MEDIA_FRAME_INFO_T));
				mediaFrameInfo.codecID = videoCodecID;
				mediaFrameInfo.frameType = frameType;
				mediaFrameInfo.frameSize = frameSize;
				mediaFrameInfo.pBuffer = (unsigned char*)frameData;
				mediaFrameInfo.rtpTimestamp = videoPTS;

				pChannel->pClient->PushFrame(pChannel->pusherHandle, MEDIA_TYPE_VIDEO, &mediaFrameInfo, (char*)frameData);

				//pThis->SendVideoFrame((char*)frameData, frameSize, frameType, videoPTS);
			}

			int delay = interval_ms;
			if (tvStartTime.tv_sec > 0)
			{
				struct timeval tvEndTime;
				gettimeofdayEx(&tvEndTime, NULL);
				uint64_t u64Interval = 0;
				if (tvEndTime.tv_sec == tvStartTime.tv_sec)
				{
					u64Interval = (tvEndTime.tv_usec - tvStartTime.tv_usec) / 1000;
				}
				else
				{
					u64Interval = ((uint64_t)(tvEndTime.tv_sec - tvStartTime.tv_sec) - 1) * 1000;
					u64Interval += (1000000 - tvStartTime.tv_usec + tvEndTime.tv_usec) / 1000;
				}

				delay -= (int)u64Interval;
			}

			if (delay > 0 && delay <= interval_ms)
			{
				SleepEx(delay);
			}
			else
			{
				printf("video delay: %d\n", delay);
			}
			gettimeofdayEx(&tvStartTime, NULL);
		}
	}

	pThread->flag = THREAD_STATUS_INIT;


	printf("read video file thread shutdown. [%d]\n", pThread->customId);

	return 0;
}

#ifdef _WIN32
DWORD WINAPI __ReadAudioFileThread(void* lpParam)
#else
void* __ReadAudioFileThread(void* lpParam)
#endif
{
	OSTHREAD_OBJ_T* pThread = (OSTHREAD_OBJ_T*)lpParam;
	CHANNEL_T* pChannel = (CHANNEL_T*)pThread->userPtr;

	pThread->flag = THREAD_STATUS_RUNNING;

	printf("read audio file thread startup. [%d]\n", pThread->customId);

	int samplerate = 8000;
	int channels = 1;

	int pcm_buf_size_per_sec = samplerate * 16 * channels / 8;			// 每秒数据量		比如8000*16*1/8=16000
	int pcm_buf_size_per_ms = pcm_buf_size_per_sec / 1000;				// 每毫秒数据量		16000/1000=16

	int interval_ms = 20;												// 间隔20毫秒
	int bytes_per_20ms = pcm_buf_size_per_ms * interval_ms;				// 每20毫秒数据量


	BUFF_T	buff;
	memset(&buff, 0x00, sizeof(BUFF_T));
	BUFF_MALLOC(&buff, bytes_per_20ms + 1);

	BUFF_T	bufG711;
	memset(&bufG711, 0x00, sizeof(BUFF_T));
	BUFF_MALLOC(&bufG711, bytes_per_20ms + 1);

	// 读PCM文件
	FILE* fAudio = fopen("music.pcm", "rb");		// 8K,16bit,1ch
	if (NULL != fAudio)
	{
		//audioCodecID = 0x10006;	// mulaw
		audioCodecID = 0x10007;		// alaw

		struct timeval tvStartTime = { 0,0 };
		while (1)
		{
			if (pThread->flag == THREAD_STATUS_EXIT)			break;

			buff.bufpos = fread(buff.pbuf, 1, bytes_per_20ms, fAudio);
			if (buff.bufpos < bytes_per_20ms)
			{
				fseek(fAudio, 0, SEEK_SET);
				buff.bufpos = fread(buff.pbuf, 1, bytes_per_20ms, fAudio);
			}
			if (buff.bufpos == bytes_per_20ms)
			{
				// 转码为G711ALAW
				int idx = 0;
				for (int i = 0; i < buff.bufpos; i += 2) {

					unsigned char uc1 = buff.pbuf[i];
					unsigned char uc2 = buff.pbuf[i + 1];

					short s = ((uc2 << 8) & 0xFF00) | (uc1 & 0xFF);
					bufG711.pbuf[idx++] = linear2alaw(s);
				}

				bufG711.bufpos = idx;

				// 时间戳递增
				audioDTS += interval_ms;
				if (audioDTS > u64_max_dts)
				{
					audioDTS = interval_ms;
				}

				GB28181_MEDIA_FRAME_INFO_T  mediaFrameInfo;
				memset(&mediaFrameInfo, 0x00, sizeof(GB28181_MEDIA_FRAME_INFO_T));
				mediaFrameInfo.codecID = audioCodecID;
				mediaFrameInfo.frameType = 0;
				mediaFrameInfo.frameSize = bufG711.bufpos;
				mediaFrameInfo.pBuffer = (unsigned char*)bufG711.pbuf;
				mediaFrameInfo.rtpTimestamp = audioDTS;

#ifdef _DEBUG__
				static FILE* f = fopen("1.aac", "wb");
				if (f)
				{
					fwrite(pBuf, 1, frameInfo->length, f);
				}
#endif

				pChannel->pClient->PushFrame(pChannel->pusherHandle, MEDIA_TYPE_AUDIO, &mediaFrameInfo, (char*)bufG711.pbuf);

				//pThis->SendAudioFrame((char*)bufG711.pbuf, bufG711.bufpos, audioDTS);
			}
			int delay = interval_ms;
			if (tvStartTime.tv_sec > 0)
			{
				struct timeval tvEndTime;
				gettimeofdayEx(&tvEndTime, NULL);
				uint64_t u64Interval = 0;

				if (tvEndTime.tv_sec == tvStartTime.tv_sec)
				{
					u64Interval = (tvEndTime.tv_usec - tvStartTime.tv_usec) / 1000;
				}
				else
				{
					u64Interval = (tvEndTime.tv_sec - tvStartTime.tv_sec - 1) * 1000;
					u64Interval += (1000000 - tvStartTime.tv_usec + tvEndTime.tv_usec) / 1000;
				}

				delay -= (int)u64Interval;
			}


			if (delay > 0 && delay <= interval_ms)
			{
				SleepEx(delay);
			}
			else
			{
				printf("audio delay: %d\n", delay);
			}

			gettimeofdayEx(&tvStartTime, NULL);
		}
	}
	BUFF_FREE(&buff);
	BUFF_FREE(&bufG711);
	pThread->flag = THREAD_STATUS_INIT;


	printf("read audio file thread shutdown. [%d]\n", pThread->customId);

	return 0;
}
#endif







int OpenStream(CHANNEL_T* pChannel)
{
#ifdef _EASY_STREAM_CLIENT_API_H
    if (NULL == pChannel->streamHandle)
    {
        EasyStreamClient_Init(&pChannel->streamHandle, 2);

        if (!pChannel->streamHandle)
        {
            printf("EasyStreamClient_Init Initial fail.\n");
            return 0;
        }

        EasyStreamClient_SetAudioEnable(pChannel->streamHandle, 1);

        EasyStreamClient_SetCallback(pChannel->streamHandle, __EasyStreamClientCallBack);

        unsigned int mediaType = EASY_SDK_VIDEO_FRAME_FLAG | EASY_SDK_AUDIO_FRAME_FLAG;

        EasyStreamClient_OpenStream(pChannel->streamHandle, pChannel->url, EASY_RTP_OVER_TCP, (void*)pChannel, 1000, 1, 1);

        //EasyStreamClient_SetAudioOutFormat(pChannel->streamHandle, EASY_SDK_AUDIO_CODEC_G711U, 8000, 1);

        //EasyStreamClient_SetPlaySpeed(pChannel->streamHandle, EASY_STREAM_CLIENT_PLAY_SPEED_FAST_X4);
    }
#else

	// 创建读视频文件线程
	CreateOSThread(&pChannel->readVideoFileThread, __ReadVideoFileThread, (void*)pChannel, 0);

	// 创建读音频文件线程
	CreateOSThread(&pChannel->readAudioFileThread, __ReadAudioFileThread, (void*)pChannel, 0);
#endif
    return 0;
}
void CloseStream(CHANNEL_T* pChannel)
{
#ifdef _EASY_STREAM_CLIENT_API_H
    if (NULL != pChannel->streamHandle)
    {
        EasyStreamClient_Deinit(pChannel->streamHandle);
        pChannel->streamHandle = NULL;
    }
#else
	DeleteOSThread(&pChannel->readVideoFileThread);				// 关闭读视频文件线程
	DeleteOSThread(&pChannel->readAudioFileThread);				// 关闭读音频文件线程

#endif
}


int __GB28181Client_Callback(void* userptr, GB28181_CLIENT_CALLBACK_TYPE_ENUM type, const char *serverID, const char* channelID, char* data, int size, const char* startTime, const char* endTime, void* ext)
{
    CHANNEL_T* pChannel = (CHANNEL_T*)userptr;

    if (type == GB28181_DEVICE_EVENT_CONNECTING)
    {
        printf("%s line[%d] 连接中....\n", __FUNCTION__, __LINE__);
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_ING)
    {
        if (size > 0)
        {
            printf("%s line[%d] 注册中....\n", __FUNCTION__, __LINE__);
        }
        else
        {
            printf("%s line[%d] 注销....\n", __FUNCTION__, __LINE__);
        }
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_TIMEOUT)
    {
        printf("%s line[%d] 注册超时....\n", __FUNCTION__, __LINE__);
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_OK)
    {
        printf("%s line[%d] 注册成功....\n", __FUNCTION__, __LINE__);
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_AUTH_FAIL)
    {
        printf("%s line[%d] 验证失败....\n", __FUNCTION__, __LINE__);
    }
    else if (type == GB28181_DEVICE_EVENT_DISCONNECT)
    {
        printf("%s line[%d] 已断开连接....\n", __FUNCTION__, __LINE__);
    }
    else if (type == GB28181_DEVICE_EVENT_START_AUDIO_VIDEO)
    {
        printf("%s line[%d] 播放音视频 channelID[%s]....\n", __FUNCTION__, __LINE__, channelID);

        pChannel->pusherHandle = ext;   // 此时ext为pusherHandle;
        OpenStream(pChannel);
    }
    else if (type == GB28181_DEVICE_EVENT_STOP_AUDIO_VIDEO)
    {
        printf("%s line[%d] 停止音视频 channelID[%s]....\n", __FUNCTION__, __LINE__, channelID);

        CloseStream(pChannel);
    }


    return 0;
}


int main__()
{
    //LOGGER_HANDLE   loggerHandle = NULL;
    //libLogger_Initialize(&loggerHandle, "GB28181SIPService", 1024 * 1024 * 2, LOG_TYPE_DEBUG, 1, 1, 1024 * 4, 0, NULL, NULL);

	XMLConfig_MultiChannel	xmlConfig;
	XMLConfig_MultiChannel::XML_CONFIG_T config;
	memset(&config, 0x00, sizeof(XMLConfig_MultiChannel::XML_CONFIG_T));
	xmlConfig.LoadConfig(XML_CONFIG_FILENAME, &config);


    GB28181CLIENT_ACCESS_INFO_T accessInfo;
    memset(&accessInfo, 0x00, sizeof(GB28181CLIENT_ACCESS_INFO_T));

    accessInfo.enable = 1;
    strcpy(accessInfo.name, "GB28181Client");
    strcpy(accessInfo.serverID, config.serverSipID);
    strcpy(accessInfo.domainID, config.serverDomainID);
    strcpy(accessInfo.serverSipAddr, config.serverSipIPAddr);
    accessInfo.serverSipPort = config.serverSipPort;

    strcpy(accessInfo.deviceID, config.deviceSipID);
    //accessInfo.deviceSipPort = 5060;
    strcpy(accessInfo.accessPwd, config.accessPwd);

    accessInfo.registerPeriod = config.registerPeriod;
    accessInfo.heartbeatPeriod = config.heartbeatPeriod;
    accessInfo.maxHeartbeatCount = config.maxHeartbeatCount;
	accessInfo.sipProtocol = config.sipProtocol;

    CHANNEL_T   streamChannel;
    memset(&streamChannel, 0x00, sizeof(CHANNEL_T));

	libGB28181Client_Create(&streamChannel.pDeviceHandle, NULL, __GB28181Client_Callback, &streamChannel);

    strcpy(streamChannel.url, "rtsp://admin:admin12345@192.168.1.13");

	libGB28181Client_AddAccessNode(streamChannel.pDeviceHandle, &accessInfo);

	int channelNum = sizeof(config.channel) / sizeof(config.channel[0]);
    for (int i = 0; i < channelNum; i++)
    {
        GB28181_CHANNEL_INFO_T  channelInfo;
        memset(&channelInfo, 0x00, sizeof(GB28181_CHANNEL_INFO_T));

		if (0 == strcmp(config.channel[i].ID, "\0"))	continue;

		strcpy(channelInfo.channelID, config.channel[i].ID);
		strcpy(channelInfo.name, config.channel[i].name);
		strcpy(channelInfo.manufacturer, config.channel[i].manufacturer);
		strcpy(channelInfo.model, config.channel[i].model);
		strcpy(channelInfo.owner, config.channel[i].owner);
		strcpy(channelInfo.civilCode, config.channel[i].civilcode);
		strcpy(channelInfo.address, config.channel[i].address);
        strcpy(channelInfo.parentID, accessInfo.serverID);
		channelInfo.registerWay = config.channel[i].registerWay;
        channelInfo.status = config.channel[i].status;

		libGB28181Client_AddChannel(streamChannel.pDeviceHandle, accessInfo.serverID, &channelInfo, false);
    }

	libGB28181Client_Start(streamChannel.pDeviceHandle);
    getchar();

    CloseStream(&streamChannel);
	libGB28181Client_Stop(streamChannel.pDeviceHandle);
    getchar();

    return 0;
}
#endif

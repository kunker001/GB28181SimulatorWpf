#ifndef __GB28181_CLIENT_API_H__
#define __GB28181_CLIENT_API_H__

//#include "libGB28181ServerAPI.h"
#include "GBDeviceInfo.h"

#ifdef _WIN32
#define LIB_GB28181_CLIENT_API  __declspec(dllexport)
#ifndef LIB_APICALL
#define LIB_APICALL  __stdcall
#endif
#define WIN32_LEAN_AND_MEAN
#else
#define LIB_GB28181_CLIENT_API  __attribute__ ((visibility("default"))) 
#define LIB_APICALL 
#endif


#ifndef MEDIA_TYPE_VIDEO
#define MEDIA_TYPE_VIDEO		0x00000001
#endif
#ifndef MEDIA_TYPE_AUDIO
#define MEDIA_TYPE_AUDIO		0x00000002
#endif
#ifndef MEDIA_TYPE_EVENT
#define MEDIA_TYPE_EVENT		0x00000004
#endif

typedef struct __GB28181CLIENT_ACCESS_INFO_T
{
	int			enable;
	char		serverID[32];			// 服务器ID
	char		domainID[16];			// 服务器域(ID前10字节)
	char		name[64];				// 服务器名称
	char		serverSipAddr[32];		// 服务器IP地址
	int			serverSipPort;			// 服务器SIP端口

	char		deviceID[32];			// 本地SIP 用户名
	int			deviceSipPort;			// 本地SIP 端口
	char		accessPwd[32];			// 本地SIP 用户认证密码

	char		deviceWanAddr[32];		// 本地外网IP地址
	int			deviceWanPort;			// 本地外网SIP端口

	int			registerPeriod;			// 注册有效期
	int			heartbeatPeriod;		// 心跳周期
	int			maxHeartbeatCount;		// 最大心跳次数
	int			catalogPacketSize;		// 目录分组大小	1,2,4,8
	int			sipProtocol;			// SIP协议交互	1(TCP) 2(UDP)

	char		charset[16];


}GB28181CLIENT_ACCESS_INFO_T;



//媒体信息
typedef struct __GB28181_MEDIA_INFO_T
{
	unsigned int videoCodec;				//视频编码类型
	unsigned int videoFps;					//视频帧率
	unsigned int videoQueueSize;			//视频队列大小	如: 1024 * 1024

	unsigned int audioCodec;				//音频编码类型
	unsigned int audioSampleRate;			//音频采样率
	unsigned int audioChannel;				//音频通道数
	unsigned int audioBitsPerSample;		//音频采样精度
	unsigned int audioQueueSize;			//音频队列大小	如: 1024 * 128

	unsigned int metadataCodec;				//Metadata类型
	unsigned int metadataQueueSize;			//Metadata队列大小	如: 1024 * 512

	unsigned int vpsLength;				//视频vps帧长度
	unsigned int spsLength;				//视频sps帧长度
	unsigned int ppsLength;				//视频pps帧长度
	unsigned int seiLength;				//视频sei帧长度
	unsigned char	 vps[256];				//视频vps帧内容
	unsigned char	 sps[256];				//视频sps帧内容
	unsigned char	 pps[128];				//视频sps帧内容
	unsigned char	 sei[128];				//视频sei帧内容
}GB28181_MEDIA_INFO_T;

typedef struct __GB28181_MEDIA_FRAME_INFO_T
{
	unsigned int	codecID;
	unsigned int    frameSize;		// 帧长
	unsigned int    frameType;		// 视频的类型，I帧或P帧
	int				samplerate;		// 音频采样率
	int				channels;		// 音频通道数
	int				bitPerSample;	// 采样位数
	unsigned char* pBuffer;			// 数据
	unsigned int	rtpTimestamp;	// 时间戳
	unsigned int	fps;
	unsigned char	timestamp[6];	// 当前播放时间	年月日时分秒  年为后两位
}GB28181_MEDIA_FRAME_INFO_T;




typedef enum __GB28181_CLIENT_CALLBACK_TYPE_ENUM
{
	GB28181_DEVICE_EVENT_CONNECTING = 1,		//连接中
	GB28181_DEVICE_EVENT_REGISTER_ING,			//注册中
	GB28181_DEVICE_EVENT_REGISTER_TIMEOUT,		//注册超时
	GB28181_DEVICE_EVENT_REGISTER_OK,			//注册成功
	GB28181_DEVICE_EVENT_REGISTER_AUTH_FAIL,	//用户验证失败
	GB28181_DEVICE_EVENT_START_AUDIO_VIDEO,		//请求推送音视频
	GB28181_DEVICE_EVENT_STOP_AUDIO_VIDEO,		//请求停止推送音视频
	GB28181_DEVICE_EVENT_TALK_AUDIO_DATA,		//对端对讲数据
	GB28181_DEVICE_EVENT_START_PLAYBACK,		//请求回放音视频
	GB28181_DEVICE_EVENT_STOP_PLAYBACK,			//请求停止回放音视频
	GB28181_DEVICE_EVENT_START_DOWNLOAD,		//请求下载音视频
	GB28181_DEVICE_EVENT_STOP_DOWNLOAD,			//请求停止下载音视频
	GB28181_DEVICE_EVENT_DISCONNECT,			//已断线

	GB28181_DEVICE_EVENT_SUBSCRIBE_ALARM,		// 告警订阅
	GB28181_DEVICE_EVENT_SUBSCRIBE_CATALOG,		// 目录订阅
	GB28181_DEVICE_EVENT_SUBSCRIBE_MOBILEPOSITION,		// 位置订阅


}GB28181_CLIENT_CALLBACK_TYPE_ENUM;

typedef int (*GB28181Client_Callback)(void* userptr, GB28181_CLIENT_CALLBACK_TYPE_ENUM type, const char *serverID, const char* channelID, 
									char* data, int size, const char *startTime, const char *endTime, void* ext/*pusherHandle or accessNode*/);


typedef void* GB28181_CLIENT_HANDLE;


#ifdef __cplusplus
extern "C"
{
#endif


	// 创建GB28181Client句柄
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_Create(GB28181_CLIENT_HANDLE *handle, void* loggerHandle, GB28181Client_Callback callback, void* userptr);
	// 释放GB28181Client句柄
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_Release(GB28181_CLIENT_HANDLE* handle);

	// 添加SIP服务器
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_AddAccessNode(GB28181_CLIENT_HANDLE handle, GB28181CLIENT_ACCESS_INFO_T* pServerInfo);
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_DeleteAccessNode(GB28181_CLIENT_HANDLE handle, const char* serverID);

	// 添加通道
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_AddChannel(GB28181_CLIENT_HANDLE handle, const char* serverID, GB28181_CHANNEL_INFO_T* pChannelInfo, bool update);
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_DeleteChannel(GB28181_CLIENT_HANDLE handle, const char* serverID, const char* channelID, bool update);

	// 启动
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_Start(GB28181_CLIENT_HANDLE handle);
	// 停止
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_Stop(GB28181_CLIENT_HANDLE handle);

	// 设置媒体编码类型
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_SetMediaInfo(GB28181_CLIENT_HANDLE handle, void* streamHandle, GB28181_MEDIA_INFO_T* pMediaInfo);

	// 推送帧数据
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_PushFrame(GB28181_CLIENT_HANDLE handle, void* streamHandle, unsigned int mediaType, GB28181_MEDIA_FRAME_INFO_T* frameInfo, const char* frameData);

	// 更新移动位置
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_SetMobilePosition(GB28181_CLIENT_HANDLE handle, float longitude, float latitude);
	// 发送告警信息
	int	LIB_GB28181_CLIENT_API LIB_APICALL	libGB28181Client_SendAlarmInfo(GB28181_CLIENT_HANDLE handle, void* accessNode, const char* channelID, const int alarmPriority, const char* alarmTime, const int alarmMethod, int alarmType, int eventType, const char *imageData);

#ifdef __cplusplus
}
#endif




#endif

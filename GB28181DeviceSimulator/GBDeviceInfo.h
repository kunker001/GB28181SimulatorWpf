#ifndef __GB_DEVICE_INFO_H__
#define __GB_DEVICE_INFO_H__


#include <string>
#include <vector>
#include <map>
using namespace std;


#define GB28181_DEV_DVR			        111   //DVR
#define GB28181_DEV_NVR			        118   //NVR
#define GB28181_DEV_HVR			        130   //HVR
#define GB28181_CAMERA			        131   //CAMERA
#define GB28181_DEV_IPC			        132   //IPC
#define GB28181_DEV_ACCESS_GATEWAY		209	  //接入网关编码
#define GB28181_DEV_ALARM_IN	        134   //告警输入设备
#define GB28181_DEV_ALARM_OUT	        135   //告警输出设备
#define GB28181_DEV_AUDIO_IN	        136   //语音输入设备
#define GB28181_DEV_AUDIO_OUT	        137   //语音输出设备
#define GB28181_SIP_SERVICE		        200   //中心信令控制服务器
#define GB28181_WEB_SERVICE		        201   //Web应用服务器编码
#define GB28181_STREAM_SERVICE	        202   //媒体分发服务器


#define     GB28181_LEN_64      64


typedef struct __SIP_REGISTER_INFO_T
{
    //Sip层返回的请求的标志 响应时返回即可
    int  sipRequestId;

    char deviceID[128];			//SIP用户名
    char name[128];
    char userpasswd[128];		//SIP用户认证密码
    char domainName[32];        // 域名
    char host[64];
    int  sipPort;
    char userAgent[32];
    char callId[128];			//维护一次注册
    char content[128];
    char status[128];			//响应状态信息
    int  expires;				//过期时间,时间单位为秒

    
    char	wanAddr[64];
    int		wanPort;


    char xmlEncoding[16];
    char digestRealm[128];
    char nonce[128];            // 平台提供的随机数
    char algorithm[128];        // 加密算法MD5
}SIP_REGISTER_INFO_T;



typedef struct __SIP_MEDIA_INFO_T
{
    char		streamingServerIpAddr[64];
    char		cameraIpAddr[64];
    int			cameraPort;
}SIP_MEDIA_INFO_T;


typedef struct __SIP_CAMERA_INFO_CONTEXT
{
    int         id;
    //GB_DEVICE_CAMERA_INFO_T info;

    int         did;
}SIP_CAMERA_INFO_CONTEXT;
typedef map<string, SIP_CAMERA_INFO_CONTEXT*> SIP_CAMERA_INFO_MAP;

typedef struct __SIP_ALARM_IO_CONTEXT
{
    //GB_DEVICE_ALARM_IO_T    info;
}SIP_ALARM_IO_CONTEXT;
typedef map<string, SIP_ALARM_IO_CONTEXT*> SIP_ALARM_IO_CONTEXT_MAP;


typedef struct __SIP_DEVICE_INFO_CONTEXT
{
    SIP_REGISTER_INFO_T  sipDeviceInfo;    //设备注册信息

    int					sipDevId;	    //设备DevID//CMS端
    int					sipRegisterStatus;    //设备注册状态
    int					sipBroadcastStatus;   //语音广播状态
    int					sipBroadcastDid;      //语音广播did
    int					sipBroadcastCid;      //语音广播cid
    SIP_CAMERA_INFO_MAP cameraInfoMap;
    SIP_ALARM_IO_CONTEXT_MAP alarmIOMap;
    time_t				curKeepaliveTime;     //当前心跳时间

    struct eXosip_t* context_eXosip;

    unsigned int        registerTime;           //注册时间
    int                 getCatalogRet;         //获取目录信息结果
    unsigned int        getCatalogLastTime;    //最后获取目录信息时间

    char			gb28181XmlEncoding[16];
    int				messageSN;


    int				referenceCount;	//引用计数

    char            localIPAddr[32];
}SIP_DEVICE_INFO_CONTEXT;



typedef struct __GB28181_DEVICE_STATUS_T
{
    time_t              registerTime;       // 注册时间
    time_t				keepaliveTime;      // 心跳时间
    time_t              getCatalogTime;     // 获取目录时间
    bool                registerStatus;
    bool                onlineStatus;
    bool                gotCatalogRet;      // 是否已获取目录列表
    int                 channelNum;

    int                 messageSN;
}GB28181_DEVICE_STATUS_T;

typedef struct __GB28181_CHANNEL_INFO_T
{
    char                channelID[32];
    char                name[64];
    char                manufacturer[32];
    char                model[32];
    char                owner[32];
    char                civilCode[32];
    char                address[64];
    int                 parental;
    char                parentID[32];
    int                 secrecy;
    int                 registerWay;
    int                 status;
    char                event[64];
    float               longitude;
    float               latitude;
}GB28181_CHANNEL_INFO_T;
typedef map<string, GB28181_CHANNEL_INFO_T>     GB28181_CHANNEL_MAP;

typedef struct __GB28181_DEVICE_INFO_T
{
    struct eXosip_t* exosip;
    SIP_REGISTER_INFO_T  registerInfo;      // 设备注册信息
    GB28181_DEVICE_STATUS_T status;         // 设备状态信息

    GB28181_CHANNEL_MAP* pChannels;


    int    GetMessageSN()
    {
        status.messageSN++;
        if (status.messageSN > 0x00FFFFFF)
        {
            status.messageSN = 1;
        }

        return status.messageSN;
    }
}GB28181_DEVICE_INFO_T;
typedef map<string, GB28181_DEVICE_INFO_T*>     GB28181_DEVICE_MAP;

typedef struct __GB28181_GLOBAL_HEADER_T
{
    char        cmdType[32];
    char        deviceID[32];
    char        deviceName[64];
    int         sn;
    char        status[16];
    int         sumNum;
    char        guardCmd[32];
    int         interval;
    char        ptzCmd[32];
    char        startTime[32];
    char        endTime[32];
    char        type[32];

}GB28181_PUBLIC_HEADER_T;


#endif


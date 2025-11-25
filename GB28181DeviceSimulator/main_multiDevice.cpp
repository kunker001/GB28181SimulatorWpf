#include "xmlConfig.h"
#include "xmlConfigDevice.h"
#include "EasyGB28181DeviceAPI.h"
#include "EasyStreamClientAPI.h"
#include <winsock2.h>

typedef struct __GB28181_CHANNEL_T
{
    char            url[1024];
    Easy_Handle     streamHandle;                   // 拉流句柄
    void* pusherHandle;                             // GB28181推流句柄

    GB28181_CLIENT_HANDLE* pGB28181DeviceHandle;
}GB28181_CHANNEL_T;

typedef map<string, GB28181_CHANNEL_T>  GB28181_DEVICE_CHANNEL_MAP;

typedef struct __GB28181_DEVICE_T
{
    GB28181CLIENT_ACCESS_INFO_T     accessInfo;

    GB28181_CLIENT_HANDLE pDeviceHandle;            // GB28181设备端句柄
    GB28181_DEVICE_CHANNEL_MAP* pChannelMap;
}GB28181_DEVICE_T;



int Easy_APICALL __EasyStreamClient_CallBack(void* _channelPtr, int _frameType, void* pBuf, EASY_FRAME_INFO* frameInfo)
{
    GB28181_CHANNEL_T* pChannel = (GB28181_CHANNEL_T*)_channelPtr;

    //printf("%s line[%d]  _frameType: %d\n", __FUNCTION__, __LINE__, _frameType);

    if (EASY_SDK_VIDEO_FRAME_FLAG == _frameType)
    {
        GB28181_MEDIA_FRAME_INFO_T  mediaFrameInfo;
        memset(&mediaFrameInfo, 0x00, sizeof(GB28181_MEDIA_FRAME_INFO_T));
        mediaFrameInfo.codecID = frameInfo->codec;
        mediaFrameInfo.frameType = frameInfo->type;
        mediaFrameInfo.frameSize = frameInfo->length;
        mediaFrameInfo.pBuffer = (unsigned char*)pBuf;

        libGB28181Client_PushFrame(*pChannel->pGB28181DeviceHandle, pChannel->pusherHandle, MEDIA_TYPE_VIDEO, &mediaFrameInfo, (char*)pBuf);
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

        libGB28181Client_PushFrame(*pChannel->pGB28181DeviceHandle, pChannel->pusherHandle, MEDIA_TYPE_AUDIO, &mediaFrameInfo, (char*)pBuf);
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

            libGB28181Client_SetMediaInfo(*pChannel->pGB28181DeviceHandle, pChannel->pusherHandle, &gbMediaInfo);
        }
    }

    //printf("%s line[%d]\n", __FUNCTION__, __LINE__);

    return 0;
}

int OpenStream(GB28181_CHANNEL_T* pChannel)
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

        EasyStreamClient_SetCallback(pChannel->streamHandle, __EasyStreamClient_CallBack);

        unsigned int mediaType = EASY_SDK_VIDEO_FRAME_FLAG | EASY_SDK_AUDIO_FRAME_FLAG;

        EasyStreamClient_OpenStream(pChannel->streamHandle, pChannel->url, EASY_RTP_OVER_TCP, (void*)pChannel, 1000, 1, 1);
    }
#else

    // 创建读视频文件线程
    CreateOSThread(&pChannel->readVideoFileThread, __ReadVideoFileThread, (void*)pChannel, 0);

    // 创建读音频文件线程
    CreateOSThread(&pChannel->readAudioFileThread, __ReadAudioFileThread, (void*)pChannel, 0);
#endif
    return 0;
}
void CloseStream(GB28181_CHANNEL_T* pChannel)
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


int __GB28181Device_Callback(void* userptr, GB28181_CLIENT_CALLBACK_TYPE_ENUM type, const char* serverID, const char* channelID, char* data, int size, const char* startTime, const char* endTime, void* ext)
{
    GB28181_DEVICE_T* pDevice = (GB28181_DEVICE_T*)userptr;

    if (type == GB28181_DEVICE_EVENT_CONNECTING)
    {
        printf("%s line[%d] 连接中....[%s --> %s]\n", __FUNCTION__, __LINE__, pDevice->accessInfo.deviceID, serverID);
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_ING)
    {
        if (size > 0)
        {
            printf("%s line[%d] 注册中....[%s --> %s]\n", __FUNCTION__, __LINE__, pDevice->accessInfo.deviceID, serverID);
        }
        else
        {
            printf("%s line[%d] 注销....[%s --> %s]\n", __FUNCTION__, __LINE__, pDevice->accessInfo.deviceID, serverID);
        }
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_TIMEOUT)
    {
        printf("%s line[%d] 注册超时....[%s]\n", __FUNCTION__, __LINE__, serverID);
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_OK)
    {
        printf("%s line[%d] 注册成功....[%s --> %s]\n", __FUNCTION__, __LINE__, pDevice->accessInfo.deviceID, serverID);
    }
    else if (type == GB28181_DEVICE_EVENT_REGISTER_AUTH_FAIL)
    {
        printf("%s line[%d] 验证失败....[%s --> %s]\n", __FUNCTION__, __LINE__, pDevice->accessInfo.deviceID, serverID);
    }
    else if (type == GB28181_DEVICE_EVENT_DISCONNECT)
    {
        printf("%s line[%d] 已断开连接....[%s --> %s]\n", __FUNCTION__, __LINE__, pDevice->accessInfo.deviceID, serverID);
    }
    else if (type == GB28181_DEVICE_EVENT_START_AUDIO_VIDEO)
    {
        printf("%s line[%d] 播放音视频 channelID[%s - %s]....\n", __FUNCTION__, __LINE__, serverID, channelID);

        GB28181_DEVICE_CHANNEL_MAP::iterator it = pDevice->pChannelMap->find(channelID);
        if (it != pDevice->pChannelMap->end())
        {
            it->second.pusherHandle = ext;              // 此时ext为pusherHandle;

            OpenStream(&it->second);
        }
    }
    else if (type == GB28181_DEVICE_EVENT_STOP_AUDIO_VIDEO)
    {
        printf("%s line[%d] 停止音视频 channelID[%s - %s]....\n", __FUNCTION__, __LINE__, serverID, channelID);

        GB28181_DEVICE_CHANNEL_MAP::iterator it = pDevice->pChannelMap->find(channelID);
        if (it != pDevice->pChannelMap->end())
        {
            CloseStream(&it->second);
        }
    }

    return 0;
}

int main()//_multiDevice()
{
    //XMLConfig

    XMLConfig	xmlConfig;
    XMLConfig::XML_CONFIG_T config;
    memset(&config, 0x00, sizeof(XMLConfig::XML_CONFIG_T));
    xmlConfig.LoadConfig(XML_CONFIG_FILENAME, &config);

    int len = (int)strlen(config.deviceSipID);
    if (len < 20)
    {
        printf("DeviceID error.\n");
        return 0;
    }

    int device_num = config.deviceNum;

    GB28181_DEVICE_T* pGB28181Device = new GB28181_DEVICE_T[device_num];
    if (NULL == pGB28181Device)		return 0;

    printf("共[%d]个设备.\n", device_num);

    int IncId = 0;
    char deviceID[64] = { 0 };
    char idStr[16] = { 0 };
    memcpy(idStr, config.deviceSipID + len - 5, 5);
    IncId = atoi(idStr);
    memcpy(deviceID, config.deviceSipID, len - 5);

    memset(pGB28181Device, 0x00, sizeof(GB28181_DEVICE_T) * device_num);
    for (int i = 0; i < device_num; i++)
    {
        pGB28181Device[i].accessInfo.enable = 1;
        sprintf(pGB28181Device[i].accessInfo.name, "EasyGBD-%05d", i + 1);
        strcpy(pGB28181Device[i].accessInfo.serverID, config.serverSipID);
        strcpy(pGB28181Device[i].accessInfo.domainID, config.serverDomainID);
        strcpy(pGB28181Device[i].accessInfo.serverSipAddr, config.serverSipIPAddr);
        pGB28181Device[i].accessInfo.serverSipPort = config.serverSipPort;
        

        sprintf(pGB28181Device[i].accessInfo.deviceID, "%s%05d", deviceID, IncId + i);
        strcpy(pGB28181Device[i].accessInfo.accessPwd, config.accessPwd);
        pGB28181Device[i].accessInfo.deviceSipPort = 5060 + i;

        pGB28181Device[i].accessInfo.registerPeriod = config.registerPeriod;
        pGB28181Device[i].accessInfo.heartbeatPeriod = config.heartbeatPeriod;
        pGB28181Device[i].accessInfo.maxHeartbeatCount = config.maxHeartbeatCount;
        pGB28181Device[i].accessInfo.sipProtocol = config.sipProtocol;

        libGB28181Client_Create(&pGB28181Device[i].pDeviceHandle, NULL, __GB28181Device_Callback, &pGB28181Device[i]);
        libGB28181Client_AddAccessNode(pGB28181Device[i].pDeviceHandle, &pGB28181Device[i].accessInfo);
        
        int channel_num = 2;
        pGB28181Device[i].pChannelMap = new GB28181_DEVICE_CHANNEL_MAP;

        for (int j = 0; j < channel_num; j++)
        {
            GB28181_CHANNEL_T   gb28181Channel;
            memset(&gb28181Channel, 0x00, sizeof(GB28181_CHANNEL_T));
            gb28181Channel.pGB28181DeviceHandle = &pGB28181Device[i].pDeviceHandle;
            strcpy(gb28181Channel.url, config.filePath);

            char channelID[64] = { 0 };
            strcpy(channelID, pGB28181Device[i].accessInfo.deviceID);
            channelID[10] = '1';
            channelID[11] = '3';
            channelID[12] = '1';

            if (channel_num > 1)
            {
                int len = (int)strlen(channelID);
                memset(channelID + len - 4, 0x00, 4);
                sprintf(channelID + len - 4, "%04d", j + 1);
            }

            pGB28181Device[i].pChannelMap->insert(GB28181_DEVICE_CHANNEL_MAP::value_type(channelID, gb28181Channel));


            GB28181_CHANNEL_INFO_T  channelInfo;
            memset(&channelInfo, 0x00, sizeof(GB28181_CHANNEL_INFO_T));

            strcpy(channelInfo.channelID, channelID);
            sprintf(channelInfo.name, "CH%02d", j);
            strcpy(channelInfo.manufacturer, "TSINGSEE");
            strcpy(channelInfo.model, "EasyGBD");
            strcpy(channelInfo.owner, "owner");
            strcpy(channelInfo.civilCode, "civilcode");
            strcpy(channelInfo.address, "");
            strcpy(channelInfo.parentID, pGB28181Device[i].accessInfo.deviceID);
            channelInfo.registerWay = 1;
            channelInfo.status = 1;
            libGB28181Client_AddChannel(pGB28181Device[i].pDeviceHandle, pGB28181Device[i].accessInfo.serverID, &channelInfo, false);
        }

        libGB28181Client_Start(pGB28181Device[i].pDeviceHandle);
    }


    printf("\n\n\n按三次Enter键退出...\n\n\n");
    getchar();
    getchar();
    getchar();

    for (int i = 0; i < device_num; i++)
    {
        GB28181_DEVICE_CHANNEL_MAP::iterator it = pGB28181Device[i].pChannelMap->begin();
        while (it != pGB28181Device[i].pChannelMap->end())
        {
            CloseStream(&it->second);

            it++;
        }

        libGB28181Client_Stop(pGB28181Device[i].pDeviceHandle);
        delete pGB28181Device[i].pChannelMap;
    }

    delete[]pGB28181Device;

    return 0;
}


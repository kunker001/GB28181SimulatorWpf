#ifndef __XML_CONFIG_DEVICE_H__
#define __XML_CONFIG_DEVICE_H__

#include "tinyxml/tinystr.h"
#include "tinyxml/tinyxml.h"


#define	XML_CONFIG_FILENAME "GB28181Device.xml"

class XMLConfig_MultiDevice
{
public:
	typedef struct __XML_CHANNEL_INFO_T
	{
		char		ID[32];
		char		name[64];
		char		manufacturer[64];
		char		model[32];
		char		owner[32];
		char		civilcode[32];
		char		address[64];
		int			registerWay;
		int			status;				// 1(ÔÚÏß)

		char		sourceURL[1024];
	}XML_CHANNEL_INFO_T;

	typedef struct __XML_DEVICE_INFO_T
	{
		char		name[64];
		char		deviceSipID[32];
		char		accessPwd[32];

		int			registerPeriod;
		int			heartbeatPeriod;
		int			maxHeartbeatCount;
		int			sipProtocol;		// 1(TCP)	2(UDP)

		int			channelNum;
		XML_CHANNEL_INFO_T	channel[1];
	}XML_DEVICE_INFO_T;


	typedef struct __XML_CONFIG_T
	{
		char		serverSipID[32];
		char		serverDomainID[32];
		char		serverSipIPAddr[64];
		int			serverSipPort;

		int			deviceNum;
		XML_DEVICE_INFO_T	deviceInfo[10];
	}XML_CONFIG_T;

public:
	XMLConfig_MultiDevice(void);
	~XMLConfig_MultiDevice(void);

	int		LoadConfig(const char *filename, XML_CONFIG_T *pConfig);
	void	SaveConfig(const char *filename, XML_CONFIG_T *pConfig);

	XML_CONFIG_T	*GetConfig()	{return &mXmlConfig;}
protected:

	XML_CONFIG_T		mXmlConfig;

	int	AddElement(const char* propertyName, char* propertyValue, TiXmlElement* pParent);
	int	AddElement(const char* propertyName, int value, TiXmlElement* pParent);
};


#endif

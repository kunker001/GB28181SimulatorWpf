#pragma once

#define	XML_CONFIG_FILENAME "GB28181Device_MultiChannel.xml"

class XMLConfig_MultiChannel
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
	}XML_CHANNEL_INFO_T;

	typedef struct __XML_CONFIG_T
	{
		char		serverSipID[32];
		char		serverDomainID[32];
		char		serverSipIPAddr[64];
		int			serverSipPort;

		char		deviceSipID[32];
		char		accessPwd[32];

		int			registerPeriod;
		int			heartbeatPeriod;
		int			maxHeartbeatCount;
		int			sipProtocol;		// 1(TCP)	2(UDP)

		XML_CHANNEL_INFO_T	channel[16];
	}XML_CONFIG_T;

public:
	XMLConfig_MultiChannel(void);
	~XMLConfig_MultiChannel(void);

	int		LoadConfig(const char *filename, XML_CONFIG_T *pConfig);
	void	SaveConfig(const char *filename, XML_CONFIG_T *pConfig);

	XML_CONFIG_T	*GetConfig()	{return &mXmlConfig;}
protected:

	XML_CONFIG_T		mXmlConfig;
};


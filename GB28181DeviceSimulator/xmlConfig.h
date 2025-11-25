#ifndef __XML_CONFIG_H__
#define __XML_CONFIG_H__

#include "tinyxml/tinystr.h"
#include "tinyxml/tinyxml.h"


#define	XML_CONFIG_FILENAME "GB28181Device.xml"

class XMLConfig
{
public:
	typedef struct __XML_CONFIG_T
	{
		char		serverSipID[32];
		char		serverDomainID[32];
		char		serverSipIPAddr[64];
		int			serverSipPort;

		char		deviceSipID[32];
		char		accessPwd[32];
		int			deviceNum;
		int			registerPeriod;
		int			heartbeatPeriod;
		int			maxHeartbeatCount;
		int			sipProtocol;			//1(TCP)	2(UDP)
		char		filePath[260];
	}XML_CONFIG_T;

public:
	XMLConfig(void);
	~XMLConfig(void);

	int		LoadConfig(const char *filename, XML_CONFIG_T *pConfig);
	void	SaveConfig(const char *filename, XML_CONFIG_T *pConfig);

	XML_CONFIG_T	*GetConfig()	{return &mXmlConfig;}
protected:

	XML_CONFIG_T		mXmlConfig;

	int	AddElement(const char* propertyName, const char* propertyValue, TiXmlElement* pParent);
	int	AddElement(const char* propertyName, int value, TiXmlElement* pParent);
};


#endif

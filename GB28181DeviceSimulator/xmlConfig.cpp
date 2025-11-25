#include "xmlConfig.h"

XMLConfig::XMLConfig(void)
{
	memset(&mXmlConfig, 0x00, sizeof(XML_CONFIG_T));
}


XMLConfig::~XMLConfig(void)
{
}


int		XMLConfig::LoadConfig(const char *filename, XML_CONFIG_T *pConfig)
{
	if (NULL == filename)			return -1;

	int ret = -1;
	TiXmlDocument m_DocR;
	if (! m_DocR.LoadFile(filename))
	{
		memset(&mXmlConfig, 0x00, sizeof(XML_CONFIG_T));

		strcpy(mXmlConfig.serverSipID, "11010100032098000001");
		strncpy(mXmlConfig.serverDomainID, mXmlConfig.serverSipID, 10);
		strcpy(mXmlConfig.serverSipIPAddr, "192.168.100.101");
		mXmlConfig.serverSipPort = 15060;
		strcpy(mXmlConfig.accessPwd, "12345678");
		strcpy(mXmlConfig.deviceSipID, "11010000001320000001");
		mXmlConfig.registerPeriod = 3600;
		mXmlConfig.heartbeatPeriod = 60;
		mXmlConfig.maxHeartbeatCount = 3;
		mXmlConfig.sipProtocol = 2;
		mXmlConfig.deviceNum = 1;
		strcpy(mXmlConfig.filePath, "test.mp4");

		SaveConfig(filename, &mXmlConfig);			//不存在配置文件, 生成一个新的配置文件
		if (NULL != pConfig)		memcpy(pConfig, &mXmlConfig, sizeof(XML_CONFIG_T));
		return ret;
	}

	TiXmlHandle hDoc(&m_DocR);
	TiXmlHandle hRoot(0);

	TiXmlElement *pConfigXML = hDoc.FirstChild("XMLConfig").ToElement();
	if (NULL != pConfigXML)
	{
		TiXmlElement *pE;

		pE = pConfigXML->FirstChildElement("ServerSIPID");
		if (pE && pE->GetText())		strcpy(mXmlConfig.serverSipID, pE->GetText());
		pE = pConfigXML->FirstChildElement("ServerDomainID");
		if (pE && pE->GetText())		strcpy(mXmlConfig.serverDomainID, pE->GetText());
		pE = pConfigXML->FirstChildElement("ServerIP");
		if (pE && pE->GetText())		strcpy(mXmlConfig.serverSipIPAddr, pE->GetText());
		pE = pConfigXML->FirstChildElement("ServerPort");
		if (pE && pE->GetText())		mXmlConfig.serverSipPort = atoi(pE->GetText());

		pE = pConfigXML->FirstChildElement("AccessPwd");
		if (pE && pE->GetText())		strcpy(mXmlConfig.accessPwd, pE->GetText());
		pE = pConfigXML->FirstChildElement("DeviceSIPID");
		if (pE && pE->GetText())		strcpy(mXmlConfig.deviceSipID, pE->GetText());
		pE = pConfigXML->FirstChildElement("RegisterPeriod");
		if (pE && pE->GetText())		mXmlConfig.registerPeriod = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("HeartbeatPeriod");
		if (pE && pE->GetText())		mXmlConfig.heartbeatPeriod = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("MaxHeartbeatCount");
		if (pE && pE->GetText())		mXmlConfig.maxHeartbeatCount = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("SipProtocol");
		if (pE && pE->GetText())		mXmlConfig.sipProtocol = (0 == strcmp(pE->GetText(), "TCP")) ? 0x01 : 0x02;

		pE = pConfigXML->FirstChildElement("DeviceNum");
		if (pE && pE->GetText())		mXmlConfig.deviceNum = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("FilePath");
		if (pE && pE->GetText())		strcpy(mXmlConfig.filePath, pE->GetText());

		if (NULL != pConfig)		memcpy(pConfig, &mXmlConfig, sizeof(XML_CONFIG_T));

		ret = 0;
	}

	return ret;
}


int	XMLConfig::AddElement(const char *propertyName, const char *propertyValue, TiXmlElement *pParent)
{
	TiXmlElement *pProperty = new TiXmlElement(propertyName);
	TiXmlText* pValue = new TiXmlText(propertyValue);
	pProperty->InsertEndChild(*pValue);
	pParent->InsertEndChild(*pProperty);
	delete pValue;
	delete pProperty;

	return 0;
}

int	XMLConfig::AddElement(const char *propertyName, int value, TiXmlElement *pParent)
{
	char sztmp[16] = {0};
	sprintf(sztmp, "%d", value);

	TiXmlElement *pProperty = new TiXmlElement(propertyName);
	TiXmlText* pValue = new TiXmlText(sztmp);
	pProperty->InsertEndChild(*pValue);
	pParent->InsertEndChild(*pProperty);
	delete pValue;
	delete pProperty;

	return 0;
}
void	XMLConfig::SaveConfig(const char *filename, XML_CONFIG_T *pConfig)
{
	if (NULL == filename)		return;
	if (NULL != pConfig)
	{
		memcpy(&mXmlConfig, pConfig, sizeof(XML_CONFIG_T));
	}

	TiXmlDocument xmlDoc( filename );
	TiXmlDeclaration Declaration( "1.0", "UTF-8", "yes" );
	xmlDoc.InsertEndChild( Declaration );

	TiXmlElement* pRootElm = NULL;
	pRootElm = new TiXmlElement( "XMLConfig" );


	AddElement("ServerSIPID",		mXmlConfig.serverSipID,		pRootElm);
	AddElement("ServerDomainID",	mXmlConfig.serverDomainID,	pRootElm);
	AddElement("ServerIP",			mXmlConfig.serverSipIPAddr, pRootElm);
	AddElement("ServerPort",		mXmlConfig.serverSipPort,	pRootElm);

	AddElement("AccessPwd", mXmlConfig.accessPwd, pRootElm);
	AddElement("DeviceSIPID", mXmlConfig.deviceSipID, pRootElm);
	

	AddElement("RegisterPeriod", mXmlConfig.registerPeriod, pRootElm);
	AddElement("HeartbeatPeriod", mXmlConfig.heartbeatPeriod, pRootElm);
	AddElement("MaxHeartbeatCount", mXmlConfig.maxHeartbeatCount, pRootElm);
	AddElement("SipProtocol", mXmlConfig.sipProtocol == 1 ? "TCP" : "UDP", pRootElm);

	AddElement("DeviceNum", mXmlConfig.deviceNum, pRootElm);
	AddElement("FilePath", mXmlConfig.filePath, pRootElm);

	xmlDoc.InsertEndChild(*pRootElm) ;

	//xmlDoc.Print() ;
	if (xmlDoc.SaveFile())
	{
	}
	delete pRootElm;
}

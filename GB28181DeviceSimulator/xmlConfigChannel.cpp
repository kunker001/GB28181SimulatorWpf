#include "xmlConfigChannel.h"
#include "tinyxml/tinystr.h"
#include "tinyxml/tinyxml.h"

XMLConfig_MultiChannel::XMLConfig_MultiChannel(void)
{
	memset(&mXmlConfig, 0x00, sizeof(XML_CONFIG_T));
}


XMLConfig_MultiChannel::~XMLConfig_MultiChannel(void)
{
}


int		XMLConfig_MultiChannel::LoadConfig(const char *filename, XML_CONFIG_T *pConfig)
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
		mXmlConfig.serverSipPort = 25061;

		strcpy(mXmlConfig.deviceSipID, "11010000001320000641");
		strcpy(mXmlConfig.accessPwd, "12345678");

		mXmlConfig.registerPeriod = 3600;
		mXmlConfig.heartbeatPeriod = 60;
		mXmlConfig.maxHeartbeatCount = 3;
		mXmlConfig.sipProtocol = 2;					// 1(TCP)	2(UDP)

		int channel_num = sizeof(mXmlConfig.channel)/sizeof(mXmlConfig.channel[0]);
		for (int i=0; i<channel_num; i++)
		{
			sprintf(mXmlConfig.channel[i].ID,	"340200000013100006%02d", i + 1);
			sprintf(mXmlConfig.channel[i].name, "CH%04d", i + 1);
			strcpy(mXmlConfig.channel[i].manufacturer, "soft");
			strcpy(mXmlConfig.channel[i].model, "ipc");
			strcpy(mXmlConfig.channel[i].owner, "owner");
			strcpy(mXmlConfig.channel[i].civilcode, "civilcode");
			strcpy(mXmlConfig.channel[i].address, "address");
			mXmlConfig.channel[i].registerWay = 1;
			mXmlConfig.channel[i].status = 1;
		}

		SaveConfig(filename, &mXmlConfig);			//不存在配置文件, 生成一个新的配置文件
		if (NULL != pConfig)		memcpy(pConfig, &mXmlConfig, sizeof(XML_CONFIG_T));
		return ret;
	}

	TiXmlHandle hDoc(&m_DocR);
	TiXmlHandle hRoot(0);

	TiXmlElement *pConfigXML = hDoc.FirstChild("XMLConfig_MultiChannel").ToElement();
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

		pE = pConfigXML->FirstChildElement("DeviceSIPID");
		if (pE && pE->GetText())		strcpy(mXmlConfig.deviceSipID, pE->GetText());
		pE = pConfigXML->FirstChildElement("AccessPwd");
		if (pE && pE->GetText())		strcpy(mXmlConfig.accessPwd, pE->GetText());
		pE = pConfigXML->FirstChildElement("RegisterPeriod");
		if (pE && pE->GetText())		mXmlConfig.registerPeriod = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("HeartbeatPeriod");
		if (pE && pE->GetText())		mXmlConfig.heartbeatPeriod = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("MaxHeartbeatCount");
		if (pE && pE->GetText())		mXmlConfig.maxHeartbeatCount = atoi(pE->GetText());
		pE = pConfigXML->FirstChildElement("SipProtocol");
		if (pE && pE->GetText())		mXmlConfig.sipProtocol = atoi(pE->GetText());

		TiXmlNode *pNode = pConfigXML->FirstChild("Channels");
		if (NULL != pNode)
		{
			TiXmlElement *pChannel = pNode->ToElement();
			if (NULL != pChannel)
			{
				int chIdx = 0;
				TiXmlElement *pCh = pChannel->FirstChild("Channel")->ToElement();
				while (NULL != pCh)
				{
					TiXmlAttribute *pAttr = pCh->FirstAttribute();
					while (NULL != pAttr)
					{
						if (0 == strcmp(pAttr->Name(), "ID"))					strcpy(mXmlConfig.channel[chIdx].ID, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Name"))			strcpy(mXmlConfig.channel[chIdx].name, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Manufacturer"))	strcpy(mXmlConfig.channel[chIdx].manufacturer, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Model"))			strcpy(mXmlConfig.channel[chIdx].model, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Owner"))			strcpy(mXmlConfig.channel[chIdx].owner, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Civilcode"))		strcpy(mXmlConfig.channel[chIdx].civilcode, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Address"))			strcpy(mXmlConfig.channel[chIdx].address, pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "RegisterWay"))		mXmlConfig.channel[chIdx].registerWay =  atoi(pAttr->Value());
						else if (0 == strcmp(pAttr->Name(), "Status"))			mXmlConfig.channel[chIdx].status = atoi(pAttr->Value());

						pAttr = pAttr->Next();
					}

					chIdx ++;
					pCh = pCh->NextSiblingElement();
				}
				//pChannel = pChannel->NextSiblingElement();
			}
		}


		if (NULL != pConfig)		memcpy(pConfig, &mXmlConfig, sizeof(XML_CONFIG_T));

		ret = 0;
	}

	return ret;
}


int	AddElement(const char *propertyName, char *propertyValue, TiXmlElement *pParent)
{
	TiXmlElement *pProperty = new TiXmlElement(propertyName);
	TiXmlText* pValue = new TiXmlText(propertyValue);
	pProperty->InsertEndChild(*pValue);
	pParent->InsertEndChild(*pProperty);
	delete pValue;
	delete pProperty;

	return 0;
}

int	AddElement(const char *propertyName, int value, TiXmlElement *pParent)
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
void	XMLConfig_MultiChannel::SaveConfig(const char *filename, XML_CONFIG_T *pConfig)
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
	pRootElm = new TiXmlElement( "XMLConfig_MultiChannel" );


	AddElement("ServerSIPID",		mXmlConfig.serverSipID,		pRootElm);
	AddElement("ServerDomainID",	mXmlConfig.serverDomainID,	pRootElm);
	AddElement("ServerIP",			mXmlConfig.serverSipIPAddr, pRootElm);
	AddElement("ServerPort",		mXmlConfig.serverSipPort,	pRootElm);
	AddElement("DeviceSIPID",		mXmlConfig.deviceSipID,		pRootElm);
	AddElement("AccessPwd",			mXmlConfig.accessPwd,		pRootElm);
	AddElement("RegisterPeriod",	mXmlConfig.registerPeriod,	pRootElm);
	AddElement("HeartbeatPeriod",	mXmlConfig.heartbeatPeriod, pRootElm);
	AddElement("MaxHeartbeatCount", mXmlConfig.maxHeartbeatCount, pRootElm);
	AddElement("SipProtocol",		mXmlConfig.sipProtocol,		pRootElm);

	TiXmlElement* pChannel = NULL;
	pChannel = new TiXmlElement( "Channels" );

	int channel_num = sizeof(mXmlConfig.channel)/sizeof(mXmlConfig.channel[0]);
	for (int i=0; i<channel_num; i++)
	{
		TiXmlElement *pCh = new TiXmlElement("Channel");
		pChannel->LinkEndChild(pCh);

		pCh->SetAttribute("No", i+1);

		pCh->SetAttribute("ID", mXmlConfig.channel[i].ID);
		pCh->SetAttribute("Name", mXmlConfig.channel[i].name);
		pCh->SetAttribute("Manufacturer", mXmlConfig.channel[i].manufacturer);
		pCh->SetAttribute("Model", mXmlConfig.channel[i].model);
		pCh->SetAttribute("Owner", mXmlConfig.channel[i].owner);
		pCh->SetAttribute("Civilcode", mXmlConfig.channel[i].civilcode);
		pCh->SetAttribute("Address", mXmlConfig.channel[i].address);
		pCh->SetAttribute("RegisterWay", mXmlConfig.channel[i].registerWay);
		pCh->SetAttribute("Status", mXmlConfig.channel[i].status);
	}
	pRootElm->LinkEndChild(pChannel);                          //链接到节点RootLv1下  

	xmlDoc.InsertEndChild(*pRootElm) ;

	//xmlDoc.Print() ;
	if (xmlDoc.SaveFile())
	{
	}
	delete pRootElm;
}

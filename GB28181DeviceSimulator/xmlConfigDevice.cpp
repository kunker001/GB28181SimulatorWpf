#include "xmlConfigDevice.h"

XMLConfig_MultiDevice::XMLConfig_MultiDevice(void)
{
	memset(&mXmlConfig, 0x00, sizeof(XML_CONFIG_T));
}


XMLConfig_MultiDevice::~XMLConfig_MultiDevice(void)
{
}


int		XMLConfig_MultiDevice::LoadConfig(const char *filename, XML_CONFIG_T *pConfig)
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

		int device_num = sizeof(mXmlConfig.deviceInfo) / sizeof(mXmlConfig.deviceInfo[0]);
		mXmlConfig.deviceNum = device_num;
		for (int dev = 0; dev < device_num; dev++)
		{
			sprintf(mXmlConfig.deviceInfo[dev].name, "EasyGBD-%05d", dev + 1);
			sprintf(mXmlConfig.deviceInfo[dev].deviceSipID, "110100000013200%05d", dev+1);
			strcpy(mXmlConfig.deviceInfo[dev].accessPwd, "12345678");

			mXmlConfig.deviceInfo[dev].registerPeriod = 3600;
			mXmlConfig.deviceInfo[dev].heartbeatPeriod = 60;
			mXmlConfig.deviceInfo[dev].maxHeartbeatCount = 3;
			mXmlConfig.deviceInfo[dev].sipProtocol = 2;					// 1(TCP)	2(UDP)

			int channel_num = sizeof(mXmlConfig.deviceInfo[dev].channel) / sizeof(mXmlConfig.deviceInfo[dev].channel[0]);
			mXmlConfig.deviceInfo[dev].channelNum = channel_num;
			for (int i = 0; i < channel_num; i++)
			{
				sprintf(mXmlConfig.deviceInfo[dev].channel[i].ID, "34020000001310%04d%02d", dev+1, i + 1);
				sprintf(mXmlConfig.deviceInfo[dev].channel[i].name, "CH%04d", i + 1);
				strcpy(mXmlConfig.deviceInfo[dev].channel[i].manufacturer, "TSINGSEE");
				strcpy(mXmlConfig.deviceInfo[dev].channel[i].model, "EasyGBD");
				strcpy(mXmlConfig.deviceInfo[dev].channel[i].owner, "owner");
				strcpy(mXmlConfig.deviceInfo[dev].channel[i].civilcode, "civilcode");
				strcpy(mXmlConfig.deviceInfo[dev].channel[i].address, "address");
				mXmlConfig.deviceInfo[dev].channel[i].registerWay = 1;
				mXmlConfig.deviceInfo[dev].channel[i].status = 1;
				strcpy(mXmlConfig.deviceInfo[dev].channel[i].sourceURL, "test.mp4");
			}
		}
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

		TiXmlNode* pDeviceList = pConfigXML->FirstChild("Devices");
		if (NULL != pDeviceList)
		{
			TiXmlElement* pDeviceInfo = pDeviceList->ToElement();
			if (NULL != pDeviceInfo)
			{
				int chIdx = 0;
				TiXmlElement* pDevice = pDeviceInfo->FirstChild("Device")->ToElement();

				int deviceIndex = 0;
				while (NULL != pDevice)
				{
					TiXmlNode* pNode = pDevice->FirstChild();
					while (NULL != pNode)
					{
						if (0 == strcmp(pNode->Value(), "DeviceSIPID"))
						{
							strcpy(mXmlConfig.deviceInfo[deviceIndex].deviceSipID, pNode->FirstChild()->Value());

							if (0 == strcmp(mXmlConfig.deviceInfo[deviceIndex].deviceSipID, "\0"))	break;

							int len = (int)strlen(mXmlConfig.deviceInfo[deviceIndex].deviceSipID);
							char suffix[16] = { 0 };
							memcpy(suffix, mXmlConfig.deviceInfo[deviceIndex].deviceSipID + len - 5, 5);

							sprintf(mXmlConfig.deviceInfo[deviceIndex].name, "EasyGBD-%s", suffix);
						}
						else if (0 == strcmp(pNode->Value(), "AccessPwd"))
						{
							strcpy(mXmlConfig.deviceInfo[deviceIndex].accessPwd, pNode->FirstChild()->Value());
						}
						else if (0 == strcmp(pNode->Value(), "RegisterPeriod"))
						{
							mXmlConfig.deviceInfo[deviceIndex].registerPeriod = atoi(pNode->FirstChild()->Value());
						}
						else if (0 == strcmp(pNode->Value(), "HeartbeatPeriod"))
						{
							mXmlConfig.deviceInfo[deviceIndex].heartbeatPeriod = atoi(pNode->FirstChild()->Value());
						}
						else if (0 == strcmp(pNode->Value(), "MaxHeartbeatCount"))
						{
							mXmlConfig.deviceInfo[deviceIndex].maxHeartbeatCount = atoi(pNode->FirstChild()->Value());
						}
						else if (0 == strcmp(pNode->Value(), "SipProtocol"))
						{
							mXmlConfig.deviceInfo[deviceIndex].sipProtocol = atoi(pNode->FirstChild()->Value());
						}
						else if (0 == strcmp(pNode->Value(), "Channels"))
						{
							int chIdx = 0;
							TiXmlElement* pCh = pNode->FirstChild("Channel")->ToElement();
							while (NULL != pCh)
							{
								memset(&mXmlConfig.deviceInfo[deviceIndex].channel[chIdx], 0x00, sizeof(XML_CHANNEL_INFO_T));

								TiXmlAttribute* pAttr = pCh->FirstAttribute();
								while (NULL != pAttr)
								{
									if (0 == strcmp(pAttr->Name(), "ID"))					strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].ID, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Name"))			strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].name, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Manufacturer"))	strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].manufacturer, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Model"))			strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].model, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Owner"))			strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].owner, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Civilcode"))		strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].civilcode, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Address"))			strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].address, pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "RegisterWay"))		mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].registerWay = atoi(pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "Status"))			mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].status = atoi(pAttr->Value());
									else if (0 == strcmp(pAttr->Name(), "SourceURL"))		strcpy(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].sourceURL, pAttr->Value());


									pAttr = pAttr->Next();
								}

								if (0 != strcmp(mXmlConfig.deviceInfo[deviceIndex].channel[chIdx].ID, "\0"))
								{
									chIdx++;

									mXmlConfig.deviceInfo[deviceIndex].channelNum = chIdx;
								}
								
								pCh = pCh->NextSiblingElement();
							}
						}

						pNode = pNode->NextSibling();
					}

					if (0 != strcmp(mXmlConfig.deviceInfo[deviceIndex].deviceSipID, "\0"))
					{
						deviceIndex++;
						mXmlConfig.deviceNum = deviceIndex;
					}
					pDevice = pDevice->NextSiblingElement();
				}
			}

		}

		if (NULL != pConfig)		memcpy(pConfig, &mXmlConfig, sizeof(XML_CONFIG_T));

		ret = 0;
	}

	return ret;
}


int	XMLConfig_MultiDevice::AddElement(const char *propertyName, char *propertyValue, TiXmlElement *pParent)
{
	TiXmlElement *pProperty = new TiXmlElement(propertyName);
	TiXmlText* pValue = new TiXmlText(propertyValue);
	pProperty->InsertEndChild(*pValue);
	pParent->InsertEndChild(*pProperty);
	delete pValue;
	delete pProperty;

	return 0;
}

int	XMLConfig_MultiDevice::AddElement(const char *propertyName, int value, TiXmlElement *pParent)
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
void	XMLConfig_MultiDevice::SaveConfig(const char *filename, XML_CONFIG_T *pConfig)
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


	TiXmlElement* pDeviceList = NULL;
	pDeviceList = new TiXmlElement("Devices");

	int device_num = sizeof(mXmlConfig.deviceInfo) / sizeof(mXmlConfig.deviceInfo[0]);
	for (int dev = 0; dev < device_num; dev++)
	{
		TiXmlElement* pDeviceInfo = new TiXmlElement("Device");
		pDeviceList->LinkEndChild(pDeviceInfo);


		AddElement("DeviceSIPID", mXmlConfig.deviceInfo[dev].deviceSipID, pDeviceInfo);
		AddElement("AccessPwd", mXmlConfig.deviceInfo[dev].accessPwd, pDeviceInfo);
		AddElement("RegisterPeriod", mXmlConfig.deviceInfo[dev].registerPeriod, pDeviceInfo);
		AddElement("HeartbeatPeriod", mXmlConfig.deviceInfo[dev].heartbeatPeriod, pDeviceInfo);
		AddElement("MaxHeartbeatCount", mXmlConfig.deviceInfo[dev].maxHeartbeatCount, pDeviceInfo);
		AddElement("SipProtocol", mXmlConfig.deviceInfo[dev].sipProtocol, pDeviceInfo);

		TiXmlElement* pChannelList = NULL;
		pChannelList = new TiXmlElement("Channels");

		int channel_num = sizeof(mXmlConfig.deviceInfo[dev].channel) / sizeof(mXmlConfig.deviceInfo[dev].channel[0]);
		for (int i = 0; i < channel_num; i++)
		{
			TiXmlElement* pCh = new TiXmlElement("Channel");
			pChannelList->LinkEndChild(pCh);

			pCh->SetAttribute("No", i + 1);

			pCh->SetAttribute("ID", mXmlConfig.deviceInfo[dev].channel[i].ID);
			pCh->SetAttribute("Name", mXmlConfig.deviceInfo[dev].channel[i].name);
			pCh->SetAttribute("Manufacturer", mXmlConfig.deviceInfo[dev].channel[i].manufacturer);
			pCh->SetAttribute("Model", mXmlConfig.deviceInfo[dev].channel[i].model);
			pCh->SetAttribute("Owner", mXmlConfig.deviceInfo[dev].channel[i].owner);
			pCh->SetAttribute("Civilcode", mXmlConfig.deviceInfo[dev].channel[i].civilcode);
			pCh->SetAttribute("Address", mXmlConfig.deviceInfo[dev].channel[i].address);
			pCh->SetAttribute("RegisterWay", mXmlConfig.deviceInfo[dev].channel[i].registerWay);
			pCh->SetAttribute("Status", mXmlConfig.deviceInfo[dev].channel[i].status);
			pCh->SetAttribute("SourceURL", mXmlConfig.deviceInfo[dev].channel[i].sourceURL);
		}
		pDeviceInfo->LinkEndChild(pChannelList);                          //链接到节点RootLv1下  
	}
	pRootElm->LinkEndChild(pDeviceList);                          //链接到节点RootLv1下  
	xmlDoc.InsertEndChild(*pRootElm) ;

	//xmlDoc.Print() ;
	if (xmlDoc.SaveFile())
	{
	}
	delete pRootElm;
}

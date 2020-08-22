#include "stdafx.h"
#include <mutex>
#include <fstream>
#include "ClientConfig.h"
#include <NetImgui_Api.h>
#include <ThirdParty/nlohmann_json/json.hpp>

static ImVector<ClientConfig*>	gConfigList;
static std::mutex				gConfigLock;
static uint32_t					gRuntimeID(1);

static constexpr uint32_t kConfigValue_Version			= 1;	//!< Configuration saved file version
static constexpr const char* kConfigFile				= "netImgui.cfg";
static constexpr const char* kConfigField_ServerPort	= "ServerPort";
static constexpr const char* kConfigField_Note			= "Note";
static constexpr const char* kConfigField_Version		= "Version";
static constexpr const char* kConfigField_Configs		= "Configs";
static constexpr const char* kConfigField_Name			= "Name";
static constexpr const char* kConfigField_Hostname		= "Hostname";
static constexpr const char* kConfigField_Hostport		= "HostPort";
static constexpr const char* kConfigField_AutoConnect	= "Auto";

//=================================================================================================
// Find entry index with same configId. (-1 if not found)
//
// Note: 'gConfigLock' should have already locked before calling this
static int FindClientIndex(uint32_t configID)
//=================================================================================================
{
	for (int i(0); i < gConfigList.size(); ++i)
	{
		if(gConfigList[i] && gConfigList[i]->RuntimeID == configID)
			return i;
	}
	return -1;
}

uint32_t ClientConfig::ServerPort = NetImgui::kDefaultServerPort;

//=================================================================================================
ClientConfig::ClientConfig()
//=================================================================================================
: HostPort(NetImgui::kDefaultClientPort)
, RuntimeID(kInvalidRuntimeID)
, ConnectAuto(false)
, ConnectRequest(false)
, Connected(false)
, Transient(false)
{
	strcpy_s(ClientName, "New Client");
	strcpy_s(HostName, "localhost");
}

//=================================================================================================
ClientConfig& ClientConfig::operator=(const ClientConfig& Copy)
//=================================================================================================
{
	memcpy(this, &Copy, sizeof(*this));
	return *this;
}

//=================================================================================================
void ClientConfig::SetConfig(const ClientConfig& config)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);	
	
	// Only allow 1 transient connection to keep things clean	
	for(int i=0; config.Transient && i<gConfigList.size(); ++i)
	{
		if( gConfigList[i] && gConfigList[i]->Transient )
		{
			delete gConfigList[i];
			gConfigList.erase(&gConfigList[i]);
		}	
	}

	int index = FindClientIndex(config.RuntimeID);
	// Config not found, add it to our list
	if( index == -1 )
	{
		index = gConfigList.size();
		gConfigList.push_back(new ClientConfig());
	}

	// Update the entry	
	*gConfigList[index] = config;
	gConfigList.back()->RuntimeID = (config.RuntimeID == kInvalidRuntimeID ? gRuntimeID++ : config.RuntimeID);
}

//=================================================================================================
void ClientConfig::DelConfig(uint32_t configID)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	int index = FindClientIndex(configID);
	if( index != -1 )
	{
		delete gConfigList[index];
		gConfigList.erase(&gConfigList[index]);
	}
}

//=================================================================================================
bool ClientConfig::GetConfigByID(uint32_t configID, ClientConfig& config)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	int index = FindClientIndex(configID);
	if (index != -1)
	{
		config = *gConfigList[index];
		return true;
	}
	return false;
}

//=================================================================================================
// Warning: This is not multithread safe. 
// While no crash will occurs, it is possible for a config  to change position between 2 calls.
// 
// Only use if there's no problem iterating over the same item 2x, or miss it entirely
bool ClientConfig::GetConfigByIndex(uint32_t index, ClientConfig& config)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	if( index < static_cast<uint32_t>(gConfigList.size()) )
	{
		config = *gConfigList[index];
		return true;
	}
	return false;
}

//=================================================================================================
uint32_t ClientConfig::GetConfigCount()
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	return gConfigList.size();
}

//=================================================================================================
void ClientConfig::SetProperty_Connected(uint32_t configID, bool value)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	int index = FindClientIndex(configID);
	if( index != -1 )
		gConfigList[index]->Connected = value;
}

//=================================================================================================
void ClientConfig::SetProperty_ConnectAuto(uint32_t configID, bool value)
//=================================================================================================
{
	int index = FindClientIndex(configID);
	if (index != -1)
		gConfigList[index]->ConnectAuto = value;
}

//=================================================================================================
void ClientConfig::SetProperty_ConnectRequest(uint32_t configID, bool value)
//=================================================================================================
{
	int index = FindClientIndex(configID);
	if (index != -1)
		gConfigList[index]->ConnectRequest = value;
}

//=================================================================================================
void ClientConfig::SaveAll()
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	nlohmann::json configRoot;
	
	for (int i(0); i<gConfigList.size(); ++i)
	{		
		ClientConfig* pConfig					= gConfigList[i];
		if( pConfig && !pConfig->Transient )
		{
			auto& config						= configRoot[kConfigField_Configs][i] = nullptr;
			config[kConfigField_Name]			= pConfig->ClientName;
			config[kConfigField_Hostname]		= pConfig->HostName;
			config[kConfigField_Hostport]		= pConfig->HostPort;
			config[kConfigField_AutoConnect]	= pConfig->ConnectAuto;
		}
	}
	configRoot[kConfigField_Version]			= kConfigValue_Version;
	configRoot[kConfigField_Note]				= "netImgui Server's list of Clients (Using JSON format).";
	configRoot[kConfigField_ServerPort]			= ServerPort;
	
	std::ofstream outputFile(kConfigFile);
	if( outputFile.is_open() )
	{
		std::string result = configRoot.dump(1);
		outputFile << result;
	}
}

//=================================================================================================
void ClientConfig::LoadAll()
//=================================================================================================
{
	Clear();
	std::lock_guard<std::mutex> guard(gConfigLock);
	
	nlohmann::json configRoot;
	std::ifstream inputFile(kConfigFile);
	if( !inputFile.is_open() )
		return;

	inputFile >> configRoot;	
	uint32_t configVersion	= configRoot.find(kConfigField_Version) != configRoot.end() ? configRoot[kConfigField_Version].get<uint32_t>() : 0u;
	ServerPort				= configRoot.find(kConfigField_ServerPort) != configRoot.end() ? configRoot[kConfigField_ServerPort].get<uint32_t>() : NetImgui::kDefaultServerPort;
	if( configVersion >= 1 )
	{	
		for(const auto& config : configRoot[kConfigField_Configs] )
		{
			bool isValid(true);
			if( isValid )
			{
				gConfigList.push_back(new ClientConfig());
				ClientConfig* pConfig = gConfigList.back();
				pConfig->RuntimeID		= gRuntimeID++;

				if( config.find(kConfigField_Name) != config.end() )
					strcpy_s(pConfig->ClientName, sizeof(pConfig->ClientName), config[kConfigField_Name].get<std::string>().c_str());
				
				if( config.find(kConfigField_Hostname) != config.end() )
					strcpy_s(pConfig->HostName, sizeof(pConfig->HostName), config[kConfigField_Hostname].get<std::string>().c_str());
				
				if( config.find(kConfigField_Hostport) != config.end() )
					pConfig->HostPort		= config[kConfigField_Hostport].get<uint32_t>();

				if( config.find(kConfigField_AutoConnect) != config.end() )
					pConfig->ConnectAuto	= config[kConfigField_AutoConnect].get<bool>();
			}
		}
	}
}

//=================================================================================================
void ClientConfig::Clear()
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	while( gConfigList.size() )
	{
		delete gConfigList.back();
		gConfigList.pop_back();
	}
}

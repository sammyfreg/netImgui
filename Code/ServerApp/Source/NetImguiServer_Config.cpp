#include <NetImgui_Api.h>
#include <mutex>
#include <fstream>
#include "NetImguiServer_Config.h"
#include <ThirdParty/nlohmann_json/json.hpp>

static ImVector<ClientConfig*>	gConfigList;
static std::mutex				gConfigLock;
static ClientConfig::RuntimeID	gRuntimeID = static_cast<ClientConfig::RuntimeID>(1);

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
	if (configID != ClientConfig::kInvalidRuntimeID)
	{
		for (int i(0); i < gConfigList.size(); ++i)
		{
			if(gConfigList[i] && gConfigList[i]->mRuntimeID == configID)
				return i;
		}
	}
	return -1;
}

uint32_t ClientConfig::sServerPort = NetImgui::kDefaultServerPort;

//=================================================================================================
ClientConfig::ClientConfig()
//=================================================================================================
: mHostPort(NetImgui::kDefaultClientPort)
, mRuntimeID(kInvalidRuntimeID)
, mConnectAuto(false)
, mConnectRequest(false)
, mConnected(false)
, mTransient(false)
{
	strcpy_s(mClientName, "New Client");
	strcpy_s(mHostName, "localhost");
}

//=================================================================================================
 ClientConfig::ClientConfig(const ClientConfig& Copy)
//=================================================================================================
{
	memcpy(this, &Copy, sizeof(*this));
}

//=================================================================================================
void ClientConfig::SetConfig(const ClientConfig& config)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);	
	
	// Only allow 1 transient connection to keep things clean	
	for(int i=0; config.mTransient && i<gConfigList.size(); ++i)
	{
		if( gConfigList[i] && gConfigList[i]->mTransient )
		{
			delete gConfigList[i];
			gConfigList.erase(&gConfigList[i]);
		}	
	}

	int index = FindClientIndex(config.mRuntimeID);
	// Config not found, add it to our list
	if( index == -1 )
	{
		index = gConfigList.size();
		gConfigList.push_back(new ClientConfig());
	}

	// Update the entry	
	*gConfigList[index] = config;
	gConfigList.back()->mRuntimeID = (config.mRuntimeID == kInvalidRuntimeID ? gRuntimeID++ : config.mRuntimeID);
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
		gConfigList[index]->mConnected = value;
}

//=================================================================================================
void ClientConfig::SetProperty_ConnectAuto(uint32_t configID, bool value)
//=================================================================================================
{
	int index = FindClientIndex(configID);
	if (index != -1)
		gConfigList[index]->mConnectAuto = value;
}

//=================================================================================================
void ClientConfig::SetProperty_ConnectRequest(uint32_t configID, bool value)
//=================================================================================================
{
	int index = FindClientIndex(configID);
	if (index != -1)
		gConfigList[index]->mConnectRequest = value;
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
		if( pConfig && !pConfig->mTransient )
		{
			auto& config						= configRoot[kConfigField_Configs][i] = nullptr;
			config[kConfigField_Name]			= pConfig->mClientName;
			config[kConfigField_Hostname]		= pConfig->mHostName;
			config[kConfigField_Hostport]		= pConfig->mHostPort;
			config[kConfigField_AutoConnect]	= pConfig->mConnectAuto;
		}
	}
	configRoot[kConfigField_Version]			= kConfigValue_Version;
	configRoot[kConfigField_Note]				= "netImgui Server's list of Clients (Using JSON format).";
	configRoot[kConfigField_ServerPort]			= sServerPort;
	
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
	sServerPort				= configRoot.find(kConfigField_ServerPort) != configRoot.end() ? configRoot[kConfigField_ServerPort].get<uint32_t>() : NetImgui::kDefaultServerPort;
	if( configVersion >= 1 )
	{	
		for(const auto& config : configRoot[kConfigField_Configs] )
		{
			bool isValid(true);
			if( isValid )
			{
				gConfigList.push_back(new ClientConfig());
				ClientConfig* pConfig = gConfigList.back();
				pConfig->mRuntimeID		= gRuntimeID++;

				if( config.find(kConfigField_Name) != config.end() )
					strcpy_s(pConfig->mClientName, sizeof(pConfig->mClientName), config[kConfigField_Name].get<std::string>().c_str());
				
				if( config.find(kConfigField_Hostname) != config.end() )
					strcpy_s(pConfig->mHostName, sizeof(pConfig->mHostName), config[kConfigField_Hostname].get<std::string>().c_str());
				
				if( config.find(kConfigField_Hostport) != config.end() )
					pConfig->mHostPort		= config[kConfigField_Hostport].get<uint32_t>();

				if( config.find(kConfigField_AutoConnect) != config.end() )
					pConfig->mConnectAuto	= config[kConfigField_AutoConnect].get<bool>();
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

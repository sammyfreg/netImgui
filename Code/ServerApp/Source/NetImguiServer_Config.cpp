#include <NetImgui_Api.h>
#include <mutex>
#include <fstream>
#include "NetImguiServer_Config.h"
#include "NetImguiServer_RemoteClient.h"
#include <ThirdParty/nlohmann_json/json.hpp>

namespace NetImguiServer { namespace Config
{

static ImVector<Client*>	gConfigList;
static std::mutex			gConfigLock;
static Client::RuntimeID	gRuntimeID = static_cast<Client::RuntimeID>(1);

static constexpr char kConfigFile[]							= "netImgui.cfg";
static constexpr char kConfigField_ServerPort[]				= "ServerPort";
static constexpr char kConfigField_ServerRefreshActive[]	= "RefreshFPSActive";
static constexpr char kConfigField_ServerRefreshInactive[]	= "RefreshFPSInactive";

static constexpr char kConfigField_Note[]					= "Note";
static constexpr char kConfigField_Version[]				= "Version";
static constexpr char kConfigField_Configs[]				= "Configs";
static constexpr char kConfigField_Name[]					= "Name";
static constexpr char kConfigField_Hostname[]				= "Hostname";
static constexpr char kConfigField_Hostport[]				= "HostPort";
static constexpr char kConfigField_AutoConnect[]			= "Auto";

uint32_t Server::sPort				= NetImgui::kDefaultServerPort;
float Server::sRefreshFPSActive		= 30.f;
float Server::sRefreshFPSInactive	= 30.f;


//=================================================================================================
// Find entry index with same configId. (-1 if not found)
// Note: 'gConfigLock' should have already locked before calling this
static int FindClientIndex(uint32_t configID)
//=================================================================================================
{
	if (configID != Client::kInvalidRuntimeID)
	{
		for (int i(0); i < gConfigList.size(); ++i)
		{
			if(gConfigList[i] && gConfigList[i]->mRuntimeID == configID)
				return i;
		}
	}
	return -1;
}

//=================================================================================================
template <typename TType>
TType GetPropertyValue(const nlohmann::json& config, const char* zPropertyName, const TType& valueDefault)
//=================================================================================================
{
	const auto& valueNode = config.find(zPropertyName);
	return valueNode != config.end() ? valueNode->get<TType>() : valueDefault;
}

//=================================================================================================
Client::Client()
//=================================================================================================
: mHostPort(NetImgui::kDefaultClientPort)
, mRuntimeID(kInvalidRuntimeID)
, mConnectAuto(false)
, mConnectRequest(false)
, mConnected(false)
, mTransient(false)
{
	NetImgui::Internal::StringCopy(mClientName, "New Client");
	NetImgui::Internal::StringCopy(mHostName, "localhost");
}

//=================================================================================================
 Client::Client(const Client& Copy)
//=================================================================================================
{
	memcpy(this, &Copy, sizeof(*this));
}

//=================================================================================================
void Client::SetConfig(const Client& config)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);	
	
	// Only allow 1 transient connection to keep things clean	
	for(int i=0; config.mTransient && i<gConfigList.size(); ++i)
	{
		if( gConfigList[i] && gConfigList[i]->mTransient )
		{
			NetImgui::Internal::netImguiDelete(gConfigList[i]);
			gConfigList.erase(&gConfigList[i]);
		}	
	}

	int index = FindClientIndex(config.mRuntimeID);
	// Config not found, add it to our list
	if( index == -1 )
	{
		index = gConfigList.size();
		gConfigList.push_back( NetImgui::Internal::netImguiNew<NetImguiServer::Config::Client>());
	}

	// Update the entry	
	*gConfigList[index] = config;
	gConfigList[index]->mRuntimeID = (config.mRuntimeID == kInvalidRuntimeID ? gRuntimeID++ : config.mRuntimeID);
}

//=================================================================================================
void Client::DelConfig(uint32_t configID)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	int index = FindClientIndex(configID);
	if( index != -1 )
	{
		NetImgui::Internal::netImguiDelete(gConfigList[index]);
		gConfigList.erase(&gConfigList[index]);
	}
}

//=================================================================================================
bool Client::GetConfigByID(uint32_t configID, Client& config)
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
bool Client::GetConfigByIndex(uint32_t index, Client& config)
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
uint32_t Client::GetConfigCount()
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	return gConfigList.size();
}

//=================================================================================================
void Client::SetProperty_Connected(uint32_t configID, bool value)
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	int index = FindClientIndex(configID);
	if( index != -1 )
		gConfigList[index]->mConnected = value;
}

//=================================================================================================
void Client::SetProperty_ConnectAuto(uint32_t configID, bool value)
//=================================================================================================
{
	int index = FindClientIndex(configID);
	if (index != -1)
		gConfigList[index]->mConnectAuto = value;
}

//=================================================================================================
void Client::SetProperty_ConnectRequest(uint32_t configID, bool value)
//=================================================================================================
{
	int index = FindClientIndex(configID);
	if (index != -1)
		gConfigList[index]->mConnectRequest = value;
}

//=================================================================================================
void Client::SaveAll()
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	nlohmann::json configRoot;
	
	for (int i(0); i<gConfigList.size(); ++i)
	{		
		Client* pConfig	= gConfigList[i];
		if( pConfig && !pConfig->mTransient )
		{
			auto& config						= configRoot[kConfigField_Configs][i] = nullptr;
			config[kConfigField_Name]			= pConfig->mClientName;
			config[kConfigField_Hostname]		= pConfig->mHostName;
			config[kConfigField_Hostport]		= pConfig->mHostPort;
			config[kConfigField_AutoConnect]	= pConfig->mConnectAuto;
		}
	}
	configRoot[kConfigField_Version]				= eVersion::_Latest;
	configRoot[kConfigField_Note]					= "netImgui Server's list of Clients (Using JSON format).";
	configRoot[kConfigField_ServerPort]				= Server::sPort;
	configRoot[kConfigField_ServerRefreshActive]	= Server::sRefreshFPSActive;
	configRoot[kConfigField_ServerRefreshInactive]	= Server::sRefreshFPSInactive;
	
	std::ofstream outputFile(kConfigFile);
	if( outputFile.is_open() )
	{
		std::string result = configRoot.dump(1);
		outputFile << result;
	}
}

//=================================================================================================
void Client::LoadAll()
//=================================================================================================
{
	Clear();
	std::lock_guard<std::mutex> guard(gConfigLock);
	
	nlohmann::json configRoot;
	std::ifstream inputFile(kConfigFile);
	if( !inputFile.is_open() )
		return;

	inputFile >> configRoot;

	uint32_t configVersion		= GetPropertyValue(configRoot, kConfigField_Version, 0u);
	Server::sPort				= GetPropertyValue(configRoot, kConfigField_ServerPort, NetImgui::kDefaultServerPort);
	Server::sRefreshFPSActive	= GetPropertyValue(configRoot, kConfigField_ServerRefreshActive, Server::sRefreshFPSActive);
	Server::sRefreshFPSInactive	= GetPropertyValue(configRoot, kConfigField_ServerRefreshInactive, Server::sRefreshFPSInactive);
	if( configVersion >= static_cast<uint32_t>(eVersion::Initial) )
	{	
		for(const auto& config : configRoot[kConfigField_Configs] )
		{
			bool isValid(true);
			if( isValid )
			{
				gConfigList.push_back(NetImgui::Internal::netImguiNew<NetImguiServer::Config::Client>());
				Client* pConfig			= gConfigList.back();
				pConfig->mRuntimeID		= gRuntimeID++;

				if( config.find(kConfigField_Name) != config.end() )
					NetImgui::Internal::StringCopy(pConfig->mClientName, config[kConfigField_Name].get<std::string>().c_str());
				
				if( config.find(kConfigField_Hostname) != config.end() )
					NetImgui::Internal::StringCopy(pConfig->mHostName, config[kConfigField_Hostname].get<std::string>().c_str());
				
				pConfig->mHostPort		= GetPropertyValue(config, kConfigField_Hostport, pConfig->mHostPort);
				pConfig->mConnectAuto	= GetPropertyValue(config, kConfigField_AutoConnect, pConfig->mConnectAuto);
			}
		}
	}
}

//=================================================================================================
void Client::Clear()
//=================================================================================================
{
	std::lock_guard<std::mutex> guard(gConfigLock);
	while( gConfigList.size() )
	{
		NetImgui::Internal::netImguiDelete(gConfigList.back());
		gConfigList.pop_back();
	}
}

}} // namespace NetImguiServer { namespace Config

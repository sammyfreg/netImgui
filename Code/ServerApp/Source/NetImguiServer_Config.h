#pragma once

#include <stdint.h>

//=================================================================================================
// ClientConfigs are used by this server to reach remote netImgui Clients.
//
// Note:For multihreading safety, we are always working with copies of the original data.
//
// Note:It is also possible for a remote netImgui client to connect to Server directly,
//		in which case they don't need to have an associated config.
//=================================================================================================
class ClientConfig
{
public:
	using RuntimeID									= uint32_t;
	static constexpr RuntimeID kInvalidRuntimeID	= static_cast<RuntimeID>(0);

					ClientConfig();
					ClientConfig(const ClientConfig& Copy);

	char			mClientName[128];			//!< Client display name
	char			mHostName[128];				//!< Client IP or remote host address to attempt connection at	
	uint32_t		mHostPort;					//!< Client Port to attempt connection at
	RuntimeID		mRuntimeID;					//!< Unique RuntimeID used to find this Config
	bool			mConnectAuto;				//!< Try automatically connecting to client
	bool			mConnectRequest;			//!< Attempt connecting to Client, after user request
	bool			mConnected;					//!< Associated client is connected to this server
	bool			mTransient;					//!< Temporary client that should not be saved (comes from cmdline)
	
	static uint32_t	sServerPort;				//!< Port that Server should use for connection. (Note: not really a 'Client' setting, but easier to just bundle the value here for the moment)

	// Add/Edit/Remove config
	static void		SetConfig(const ClientConfig& config);						//!< Add or replace a client configuration info
	static void		DelConfig(uint32_t configID);								//!< Remove a client configuration
	static bool		GetConfigByID(uint32_t configID, ClientConfig& outConfig);	//!< Find client configuration with this id (return true if found)
	static bool		GetConfigByIndex(uint32_t index, ClientConfig& outConfig);	//!< Find client configuration at the x position 
	static uint32_t	GetConfigCount();

	// Set property value directly (without having to copy entire structure)
	static void		SetProperty_Connected(uint32_t configID, bool value);	
	static void		SetProperty_ConnectAuto(uint32_t configID, bool value);
	static void		SetProperty_ConnectRequest(uint32_t configID, bool value);
	
	// Client Config list management
	static void		SaveAll();
	static void		LoadAll();
	static void		Clear();
};

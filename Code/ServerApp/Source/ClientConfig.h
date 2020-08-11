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
	static constexpr uint32_t kInvalidRuntimeID = 0;

					ClientConfig();
	ClientConfig&	operator=(const ClientConfig& Copy);

	char			ClientName[128];			//!< Client display name
	char			HostName[128];				//!< Client IP or remote host address to attempt connection at	
	uint32_t		HostPort;					//!< Client Port to attempt connection at
	uint32_t		RuntimeID;					//!< Unique RuntimeID used to find this Config
	bool			ConnectAuto;				//!< Try automatically connecting to client
	bool			ConnectRequest;				//!< Attempt connecting to Client, after user request
	bool			Connected;					//!< Associated client is connected to this server
	bool			Transient;					//!< Temporary client that should not be saved (comes from cmdline)
	
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

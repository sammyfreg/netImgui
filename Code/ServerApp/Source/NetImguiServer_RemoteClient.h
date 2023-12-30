#pragma once

#include <vector>
#include <chrono>
#include <unordered_map>
#include <Private/NetImgui_CmdPackets.h>
#include "NetImguiServer_App.h"

namespace NetImguiServer { namespace RemoteClient
{

//=================================================================================================
// ImDrawData wrapper
// 
// Allocate a single ImDrawList and assign it to itself. 
// 
// This child class leave the original ImDrawData behavior intact, but add the proper 
// memory freeing of the ImDrawList member.
//=================================================================================================
struct NetImguiImDrawData : ImDrawData
{
				NetImguiImDrawData();
	ImDrawList	mCommandList;
};

//=================================================================================================
// All info needed by the server to communicate with a remote client, and render its content
//=================================================================================================
struct Client
{	
	static constexpr uint32_t kInvalidClient = static_cast<uint32_t>(-1);
	using ExchPtrInput		= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdInput>;
	using ExchPtrClipboard	= NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdClipboard>;
	using ExchPtrBackground = NetImgui::Internal::ExchangePtr<NetImgui::Internal::CmdBackground>;
	using ExchPtrImguiDraw	= NetImgui::Internal::ExchangePtr<NetImguiImDrawData>;
	using TextureTable		= std::unordered_map<uint64_t, App::ServerTexture>;

											Client();
											~Client();
											Client(const Client&)	= delete;
											Client(const Client&&)	= delete;
	void									operator=(const Client&) = delete;
	void									Initialize();
	void									Uninitialize();
	void									Release();
	bool									IsValid()const;

	void									ReceiveTexture(NetImgui::Internal::CmdTexture*);
	void									ReceiveDrawFrame(NetImgui::Internal::CmdDrawFrame*);
	NetImguiImDrawData*						ConvertToImguiDrawData(const NetImgui::Internal::CmdDrawFrame* pCmdDrawFrame);
	NetImguiImDrawData*						GetImguiDrawData(void* pEmtpyTextureHAL);	// Get current active Imgui draw data
		
	void									CaptureImguiInput();
	NetImgui::Internal::CmdInput*			TakePendingInput();
	NetImgui::Internal::CmdClipboard*		TakePendingClipboard();
	void									ProcessPendingTextures();

	void*									mpHAL_AreaRT			= nullptr;
	void*									mpHAL_AreaTexture		= nullptr;
	uint16_t								mAreaRTSizeX			= 0;		//!< Currently allocated RenderTarget size
	uint16_t								mAreaRTSizeY			= 0;		//!< Currently allocated RenderTarget size
	uint16_t								mAreaSizeX				= 0;		//!< Available area size available to remote client
	uint16_t								mAreaSizeY				= 0;		//!< Available area size available to remote client
	char									mInfoName[128]			= {};
	char									mWindowID[128+16]		= {};
	char									mInfoImguiVerName[16]	= {};
	char									mInfoNetImguiVerName[16]= {};
	uint32_t								mInfoImguiVerID			= 0;
	uint32_t								mInfoNetImguiVerID		= 0;
	char									mConnectHost[64]		= {};		//!< Connected Hostname of this remote client
	int										mConnectPort			= 0;		//!< Connected Port of this remote client

	NetImguiImDrawData*						mpImguiDrawData			= nullptr;	//!< Current Imgui Data that this client is the owner of
	NetImgui::Internal::CmdDrawFrame*		mpFrameDrawPrev			= nullptr;	//!< Last valid DrawDrame (used by com thread, to uncompress data)
	TextureTable							mTextureTable;						//!< Table of textures received and used by the client
	ExchPtrImguiDraw						mPendingImguiDrawDataIn;			//!< Pending received Imgui DrawData, waiting to be taken ownership of
	ExchPtrBackground						mPendingBackgroundIn;				//!< Background settings received and waiting to update client setting
	ExchPtrClipboard						mPendingClipboardIn;				//!< Clipboard received from Client and waiting to be processed on Server
	ExchPtrInput							mPendingInputOut;					//!< Input command waiting to be sent out to client
	ExchPtrClipboard						mPendingClipboardOut;				//!< Clipboard command waiting to be sent out to client
	std::vector<ImWchar>					mPendingInputChars;					//!< Captured Imgui characters input waiting to be added to new InputCmd
	NetImgui::Internal::CmdTexture*			mpPendingTextures[64]	= {};		//!< Textures commands waiting to be processed in main update loop
	std::atomic_uint64_t					mPendingTextureReadIndex;
	std::atomic_uint64_t					mPendingTextureWriteIndex;
	bool									mbIsVisible				= false;	//!< If currently shown
	bool									mbIsActive				= false;	//!< Is the current active window (will receive input, only one is true at a time)
	bool									mbIsReleased			= false;	//!< If released in com thread and main thread should delete resources
	std::atomic_bool						mbIsFree;							//!< If available to use for a new connected client
	std::atomic_bool						mbIsConnected;						//!< If connected to a remote client
	std::atomic_bool						mbDisconnectPending;				//!< Server requested a disconnect on this item
	std::atomic_bool						mbCompressionSkipOncePending;		//!< When we detect invalid previous DrawFrame command, cancel compression for 1 frame, to get good data
	std::chrono::steady_clock::time_point	mConnectedTime;						//!< When the connection was established with this remote client
	std::chrono::steady_clock::time_point	mLastUpdateTime;					//!< When the client last send a content refresh request
	std::chrono::steady_clock::time_point	mLastDrawFrame;						//!< When we last receive a new drawframe commant	
	uint32_t								mClientConfigID;					//!< ID of ClientConfig that connected (if connection came from our list of ClientConfigs)	
	uint32_t								mClientIndex			= 0;		//!< Entry idx into table of connected clients
	uint64_t								mStatsDataRcvd;						//!< Current amount of Bytes received since connected
	uint64_t								mStatsDataSent;						//!< Current amount of Bytes sent to client since connected
	uint64_t								mStatsDataRcvdPrev;					//!< Last amount of Bytes received since connected
	uint64_t								mStatsDataSentPrev;					//!< Last amount of Bytes sent to client since connected
	std::chrono::steady_clock::time_point	mStatsTime;							//!< Time when info was collected (with history of last x values)
	uint32_t								mStatsRcvdBps;						//!< Average Bytes received per second
	uint32_t								mStatsSentBps;						//!< Average Bytes sent per second
	float									mStatsFPS;							//!< Average refresh rate of content
	uint32_t								mStatsIndex;
	float									mMousePos[2]				= {0,0};
	float									mMouseWheelPos[2]			= {0,0};
	ImGuiMouseCursor						mMouseCursor				= ImGuiMouseCursor_None;	// Last mosue cursor remote client requested
	ImGuiContext*							mpBGContext					= nullptr;					// Special Imgui Context used to render the background (only updated when needed)	
	bool									mBGNeedUpdate				= true;						// Let engine know that we should regenerate the background draw commands
	NetImgui::Internal::CmdBackground		mBGSettings;											// Settings for client background drawing settings
	
	static bool								Startup(uint32_t clientCountMax);
	static void								Shutdown();
	static uint32_t							GetCountMax();
	static uint32_t							GetFreeIndex();
	static Client&							Get(uint32_t index);
};

}} // namespace NetImguiServer { namespace Client
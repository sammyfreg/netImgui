#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "NetImGui_Client.h"
#include "NetImGui_Network.h"
#include "NetImGui_CmdPackets.h"

namespace NetImgui { namespace Internal { namespace Client 
{

//=================================================================================================
// COMMUNICATIONS INITIALIZE
// Initialize a new connection to a RemoteImgui server
//=================================================================================================
bool Communications_Initialize(ClientInfo& client)
{
	CmdVersion cmdVersionSend;
	CmdVersion cmdVersionRcv;	
	strcpy_s(cmdVersionSend.mClientName, client.mName);
	bool bResultSend =		Network::DataSend(client.mpSocket, &cmdVersionSend, cmdVersionSend.mHeader.mSize);
	bool bResultRcv	=		Network::DataReceive(client.mpSocket, &cmdVersionRcv, sizeof(cmdVersionRcv));		
	client.mbConnected =	bResultRcv && bResultSend && 
							cmdVersionRcv.mHeader.mType == CmdHeader::kCmdVersion && 
							cmdVersionRcv.mVersion == CmdVersion::kVer_Current;

	if( client.mbConnected )
	{
		client.mbHasTextureUpdate = true;
		for(auto& texture : client.mTextures)
			texture.mbSent = false;

		return true;
	}
	return false;
}

//=================================================================================================
// INCOM: INPUT
// Receive new keyboard/mouse/screen resolution input to pass on to dearImgui
//=================================================================================================
void Communications_Incoming_Input(ClientInfo& client, void* pData)
{
	CmdInput* pCmdInput	 = reinterpret_cast<CmdInput*>(pData);
	size_t keyCount(pCmdInput->KeyCharCount);
	client.mPendingKeyIn.AddData(pCmdInput->KeyChars, keyCount);
	client.mPendingInputIn.Assign(pCmdInput);
}

//=================================================================================================
// OUTCOM: TEXTURE
// Transmit all pending new/updated texture
//=================================================================================================
bool Communications_Outgoing_Textures(ClientInfo& client)
{	
	bool bSuccess(true);
	client.ProcessTextures();
	if( client.mbHasTextureUpdate )
	{
		for(auto& cmdTexture : client.mTextures)
		{
			if( cmdTexture.mbSent == false )
			{
				bSuccess &= Network::DataSend(client.mpSocket, cmdTexture.mpCmdTexture, cmdTexture.mpCmdTexture->mHeader.mSize);
				cmdTexture.mbSent = bSuccess;
			}
		}
		client.mbHasTextureUpdate = !bSuccess;
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: FRAME
// Transmit a new dearImgui frame to render
//=================================================================================================
bool Communications_Outgoing_Frame(ClientInfo& client)
{
	bool bSuccess(true);
	CmdDrawFrame* pPendingDrawFrame = client.mPendingFrameOut.Release();
	if( pPendingDrawFrame )
	{
		bSuccess = Network::DataSend(client.mpSocket, pPendingDrawFrame, pPendingDrawFrame->mHeader.mSize);
		netImguiDeleteSafe(pPendingDrawFrame);
	}
	return bSuccess;
}

//=================================================================================================
// OUTCOM: DISCONNECT
// Signal that we will be disconnecting
//=================================================================================================
bool Communications_Outgoing_Disconnect(ClientInfo& client)
{
	if( client.mbDisconnectRequest )
	{
		CmdDisconnect cmdDisconnect;
		Network::DataSend(client.mpSocket, &cmdDisconnect, cmdDisconnect.mHeader.mSize);
	}
	return true;
}

//=================================================================================================
// OUTCOM: PING
// Signal end of outgoing communications and still alive
//=================================================================================================
bool Communications_Outgoing_Ping(ClientInfo& client)
{
	CmdPing cmdPing;
	return Network::DataSend(client.mpSocket, &cmdPing, cmdPing.mHeader.mSize);		
}

//=================================================================================================
// INCOMING COMMUNICATIONS
//=================================================================================================
bool Communications_Incoming(ClientInfo& client)
{
	bool bOk(true);
	bool bPingReceived(false);
	while( bOk && !bPingReceived )
	{
		CmdHeader cmdHeader;
		char* pCmdData		= nullptr;
		bOk					= Network::DataReceive(client.mpSocket, &cmdHeader, sizeof(cmdHeader));
		if( bOk && cmdHeader.mSize > sizeof(CmdHeader) )
		{
			pCmdData		= netImguiNew<char>(cmdHeader.mSize);
			memcpy(pCmdData, &cmdHeader, sizeof(CmdHeader));
			bOk				= Network::DataReceive(client.mpSocket, &pCmdData[sizeof(cmdHeader)], cmdHeader.mSize-sizeof(cmdHeader));	
		}

		if( bOk )
		{
			switch( cmdHeader.mType )
			{
			case CmdHeader::kCmdPing:		
				bPingReceived = true; 
				break;
			case CmdHeader::kCmdDisconnect:	
				bOk = false; 
				break;
			case CmdHeader::kCmdInput:		
				Communications_Incoming_Input(client, pCmdData);
				pCmdData = nullptr; // Took ownership of the data, prevent Free
				break;
			default: break;
			}
		}
		
		netImguiDeleteSafe(pCmdData);
	}
	return bOk;
}


//=================================================================================================
// OUTGOING COMMUNICATIONS
//=================================================================================================
bool Communications_Outgoing(ClientInfo& client)
{		
	bool bSuccess(true);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Textures(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Frame(client);	
	if( bSuccess )
		bSuccess = Communications_Outgoing_Disconnect(client);
	if( bSuccess )
		bSuccess = Communications_Outgoing_Ping(client); // Always finish with a ping

	return bSuccess;
}

//=================================================================================================
// COMMUNICATIONS THREAD 
//=================================================================================================
void Communications(ClientInfo* pClient)
{	
	Communications_Initialize(*pClient);
	while( pClient->mbConnected )
	{
		pClient->mbConnected =	Communications_Outgoing(*pClient) && 
								Communications_Incoming(*pClient);
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
	Network::Disconnect(pClient->mpSocket);
	pClient->mbDisconnectRequest = false;
}

void ClientInfo::ProcessTextures()
{
	while( mTexturesPendingCount > 0 )
	{
		int32_t count				= mTexturesPendingCount.fetch_sub(1);
		CmdTexture* pCmdTexture		= mTexturesPending[count-1];
		mTexturesPending[count-1]	= nullptr;

		// Find the TextureId from our list
		int texIdx(0);
		while( texIdx < mTextures.size() && pCmdTexture->mTextureId != mTextures[texIdx].mpCmdTexture->mTextureId )
			++texIdx;

		// Remove a texture
		if( texIdx < mTextures.size() && pCmdTexture->mpTextureData.mOffset == 0 )
		{
			mTextures[texIdx].Set(nullptr);
			mTextures[texIdx] = mTextures[mTextures.size()-1];
			mTextures.resize(mTextures.size()-1); //SF Should improve this to avoid sizedown reallocation
			netImguiDeleteSafe(pCmdTexture);
		}
		// Add/Update a texture
		else if( pCmdTexture->mpTextureData.mOffset != 0 )
		{
			if( texIdx == mTextures.size() )
				mTextures.resize(texIdx+1);
			
			mTextures[texIdx].Set( pCmdTexture );
			mbHasTextureUpdate = true; //Only notify server of new/updated texture
		}		
	}
}

}}} // namespace NetImgui::Internal::Client

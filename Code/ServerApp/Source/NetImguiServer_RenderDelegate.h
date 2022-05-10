#pragma once

#include <cstdint>

#include "NetImgui_Api.h"

struct ImDrawData;

namespace NetImguiServer {

	namespace RemoteClient { struct Client; }
	namespace App { struct ServerTexture; }
	
	class RenderDelegate
	{
	public:

		virtual ~RenderDelegate() = default;

		// Receive a ImDrawData drawlist and request Dear ImGui's backend to output it into a texture
		virtual void RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData) const = 0;

		// Allocate a texture resource
		virtual bool CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, App::ServerTexture& OutTexture) const = 0;

		// Free a Texture resource
		virtual void DestroyTexture(App::ServerTexture& OutTexture) const = 0;

		// Allocate a RenderTarget that each client will use to output their ImGui drawing into.
		virtual bool CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture) const = 0;

		// Free a RenderTarget resource
		virtual void DestroyRenderTarget(void*& pOutRT, void*& pOutTexture) const = 0;

		virtual bool IsRenderTargetInverted() const { return false; }
	};
}


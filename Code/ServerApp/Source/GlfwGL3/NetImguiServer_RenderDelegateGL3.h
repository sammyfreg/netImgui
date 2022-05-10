#include "NetImguiServer_RenderDelegate.h"

namespace NetImguiServer {

    class RenderDelegateGL3 : public RenderDelegate
    {
    public:

        // Receive a ImDrawData drawlist and request Dear ImGui's backend to output it into a texture
        void RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData) const override;

        // Allocate a texture resource
        bool CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, App::ServerTexture& OutTexture) const override;

        // Free a Texture resource
        void DestroyTexture(App::ServerTexture& OutTexture) const override;

        // Allocate a RenderTarget that each client will use to output their ImGui drawing into.
        bool CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture) const override;

        // Free a RenderTarget resource
        void DestroyRenderTarget(void*& pOutRT, void*& pOutTexture) const override;

        // Invert client render target Y axis(since OpenGL start texture UV from BottomLeft instead of DirectX TopLeft)
        bool IsRenderTargetInverted() const override;
    };
}
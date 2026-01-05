//=================================================================================================
// SAMPLE TEXTURES
//-------------------------------------------------------------------------------------------------
// Example of using various textures in the ImGui context, after making netImgui aware of them.
//=================================================================================================

#include "../Common/Sample.h"
#include "../Common/TextureResource.h"

// Enable handling of a Custom Texture Format samples.
// Only meant as an example, users are free to replace it with their own support
//Look for this define for implementation details.
#define TEXTURE_CUSTOM_SAMPLE 0 //@sammyfreg todo re-implement custom texture support
#if TEXTURE_CUSTOM_SAMPLE
//=================================================================================================
// This is our Custom texture data format
// It can be customized to anything needed, as long as Client/Server use the same content
// For this sample, we rely on the struct size and a starting stamp to identify the custom data
//
// You can imagine using custom texture format to send some compressed video frame, 
// or other new format handling, but require modifications to the Server codebase 
//=================================================================================================
struct CustomTextureDataA{ 
	static constexpr uint64_t kStamp = 0x0123456789000001;
	uint64_t m_Stamp		= kStamp;
	uint32_t m_ColorStart	= 0; 
	uint32_t m_ColorEnd		= 0; 
};

struct CustomTextureDataB{
	static constexpr uint64_t kStamp = 0x0123456789000002;
	uint64_t m_Stamp		= kStamp;
	uint64_t m_TimeUS		= 0;
};
#endif

//=================================================================================================
// Utility function to convert a 0-1 value to [0-255] with 0.5 being the perceptual middle value
//=================================================================================================
inline uint8_t LinearToGamma8(float LinearValue)
{
	return static_cast<uint8_t>(pow(LinearValue, 2.2f) * float(0xFF));
}

//=================================================================================================
// TEXTURE SAMPLE CLASS
//=================================================================================================
class SampleTextures : public Sample::Base
{
public:
						SampleTextures() : Base("SampleTextures") {}
	virtual bool		Startup() override;
	virtual void		Shutdown() override;
	virtual void		Draw() override;

protected:
	// Struct storing data needed by this Sample's UI
	// to show informations about each texture entry
	struct TexInfo
	{
		TexInfo() = default;
		TexInfo(const char* Name, const char* Description) : mName(Name), mDescription(Description){}
		TexInfo(const char* Name, const char* Action, const char* Description) : mName(Name), mDescription(Description), mAction(Action){}
		const char*		mName 	= "";			// Text displayed for texture's name
		const char*		mDescription = "";		// Text displayed for texture's description
		const char*		mAction = "";			// Text displayed for associated action button
		bool 			mRemoteOnly = false;	// If it should only be displayed on remote NetImgui
		ImTextureRef 	mTexRef;				// Updated every frame to associated texture object
	};

	// List of Dear Imgui managed Textures
	enum class EImguiTex{ DefaultEmpty, White, CreateDestroy, Update, _Count};
	static constexpr uint32_t kTexCountImgui = static_cast<uint32_t>(EImguiTex::_Count);
	
	// List of user managed textures
	enum class EUserTex{ RGBA32, White, CreateDestroy, DoubleSend, Alpha8,	CustomA, CustomB, Invalid, _Count};	
	static constexpr uint32_t kTexCountUser	= static_cast<uint32_t>(EUserTex::_Count);

	void 					InitImguiTexture();
	void 					InitUserTexture();
	void 					UpdateTextures();
	void 					UpdateCustomTexture();
	bool 					DrawTextureInfo(const TexInfo& TextureInfo);
	void 					DemoImActionCreateDestroy();
	void					DemoImActionPartialUpdate();
	void 					DemoUserActionCreateDestroy();
	void 					DemoUserActionDoubleSend();

	inline TexInfo&			GetTexInfo(EImguiTex Entry);
	inline TexInfo&			GetTexInfo(EUserTex Entry);
	inline TexResImgui&		GetTexResource(EImguiTex Entry);
	inline TexResUser&		GetTexResource(EUserTex Entry);

	TexInfo					mImguiTexInfo[kTexCountImgui];
	TexInfo					mUserTexInfo[kTexCountUser];
	TexResImgui				mImguiTextures[kTexCountImgui];
	TexResUser				mUserTextures[kTexCountUser];
	uint8_t					mActionPartialUpdateCount = 0;
};

//=================================================================================================
// GET SAMPLE
// Each project must return a valid sample object
//=================================================================================================
Sample::Base& GetSample()
{
	static SampleTextures sample;
	return sample;
}

//=================================================================================================
// Get Texture Entries
//=================================================================================================
TexResImgui& SampleTextures::GetTexResource(EImguiTex Entry) 
{ 
	assert(static_cast<int>(Entry) >= 0 && Entry <= EImguiTex::_Count);
	return mImguiTextures[static_cast<int>(Entry)];
}

TexResUser& SampleTextures::GetTexResource(EUserTex Entry)
{
	assert(static_cast<int>(Entry) >= 0 && Entry <= EUserTex::_Count);
	return mUserTextures[static_cast<int>(Entry)];
}

SampleTextures::TexInfo& SampleTextures::GetTexInfo(EImguiTex Entry)
{ 
	assert(static_cast<int>(Entry) >= 0 && Entry <= EImguiTex::_Count);
	return mImguiTexInfo[static_cast<int>(Entry)];
}

SampleTextures::TexInfo& SampleTextures::GetTexInfo(EUserTex Entry)
{ 
	assert(static_cast<int>(Entry) >= 0 && Entry <= EUserTex::_Count);
	return mUserTexInfo[static_cast<int>(Entry)]; 
}

//=================================================================================================
// Update our texture resources every frame, to process 
// 'texture destruction' requests with 1 frame delay. 
//
// Also keep the ImTexureRef information of User Textures
// up-to-date with their associated texture resource.
//=================================================================================================
void SampleTextures::UpdateTextures()
{
	for (uint32_t i = 0; i < kTexCountImgui; ++i)
	{
		mImguiTextures[i].Update();
		mImguiTexInfo[i].mTexRef = mImguiTextures[i].IsValid() ? mImguiTextures[i].mImTexRef : GetTexResource(EImguiTex::DefaultEmpty).mImTexRef;
	}

	for (uint32_t i = 0; i < kTexCountUser; ++i)
	{
		mUserTextures[i].Update();
		mUserTexInfo[i].mTexRef = mUserTextures[i].IsValid() ? mUserTextures[i].mTextureId : GetTexResource(EImguiTex::DefaultEmpty).mImTexRef;
	}

	// When we are drawing on remote connection, assign an invalid
	// texture id to test its proper handling
	if( NetImgui::IsConnected() )
	{
		GetTexInfo(EUserTex::Invalid).mTexRef = ImTextureRef(0x1010101010101010);
	}

	UpdateCustomTexture();
}

//=================================================================================================
bool SampleTextures::Startup()
{
	if (!Base::Startup())
		return false;

	InitImguiTexture();
	InitUserTexture();
	return true;
}

//=================================================================================================
// Called when exiting
//=================================================================================================
void SampleTextures::Shutdown()
{
	//Release user created textures
	//Note:	Native Dear Imgui Textures are auto released by the Dear ImGui backend,
	//		without need for manual handling like the user textures below.
	for(uint32_t i(0); i<kTexCountUser; ++i){
		if( mUserTextures[i].IsValid() ){
			mUserTextures[i].Destroy();
		}
	}

	Base::Shutdown();
}

//=================================================================================================
// Function used by the sample, to draw all ImGui Content
//=================================================================================================
void SampleTextures::Draw()
{
	//---------------------------------------------------------------------------------------------
	// (0) Update Texture references and destruction status
	//---------------------------------------------------------------------------------------------
	UpdateTextures();

	//---------------------------------------------------------------------------------------------
	// (1) Start a new Frame
	//---------------------------------------------------------------------------------------------
	if (NetImgui::NewFrame(true))
	{
		//-----------------------------------------------------------------------------------------
		// (2) Draw ImGui Content
		//-----------------------------------------------------------------------------------------
		Base::Draw_Connect(); //Note: Connection to remote server done in there

		ImGui::SetNextWindowPos(ImVec2(32, 48), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(1024, 800), ImGuiCond_Once);

		if (ImGui::Begin("Sample Textures", nullptr))
		{
			if( ImGui::BeginTable("2x2", 2, ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerV) )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
			
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "DearImgui Backend Textures");
				ImGui::TextWrapped("Native Dear ImGui Textures changes are automatically detected by NetImgui, they don't need manual management. ");
				ImGui::NewLine();

				ImGui::TableNextColumn();
				ImGui::TextColored(ImVec4(0.1, 1, 0.1, 1), "User Managed Textures");
				ImGui::TextWrapped("User texture on the otherhand (texture created and managed outside of the Dear ImGui backend support) need to let NetImgui know about them.");
				ImGui::NewLine();

				//-----------------------------------------------------------------
				// Demo of Dear ImGui Backend Texture support
				//-----------------------------------------------------------------
				ImGui::TableNextColumn();
				if( ImGui::BeginTable("ImEntries", 3, ImGuiTableFlags_SizingFixedFit|ImGuiTableFlags_RowBg) )
				{
					for (uint32_t i = 0; i < kTexCountImgui; ++i)
					{
						if( !mImguiTexInfo[i].mRemoteOnly || NetImgui::IsDrawingRemote() )
						{
							if( DrawTextureInfo(mImguiTexInfo[i]))
							{
								switch(static_cast<EImguiTex>(i))
								{
									case EImguiTex::CreateDestroy: DemoImActionCreateDestroy(); break;
									case EImguiTex::Update: DemoImActionPartialUpdate(); break;
									default:break;
								}
							}
						}
					}
					ImGui::EndTable();
				}
				//-----------------------------------------------------------------
				// Demo of older NetImgui texture handling
				//-----------------------------------------------------------------
				ImGui::TableNextColumn();
				if( ImGui::BeginTable("ImEntries", 3, ImGuiTableFlags_SizingFixedFit|ImGuiTableFlags_RowBg) )
				{
					for (uint32_t i = 0; i < kTexCountUser; ++i)
					{
						if( !mUserTexInfo[i].mRemoteOnly || NetImgui::IsDrawingRemote() )
						{
							if( DrawTextureInfo(mUserTexInfo[i]) )
							{
								switch(static_cast<EUserTex>(i))
								{
									case EUserTex::CreateDestroy: DemoUserActionCreateDestroy(); break;
									default:break;
								}
							}
						}
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndTable();
		}
		ImGui::End();

		//-----------------------------------------------------------------------------------------
		// (3) Finish the frame, preparing the drawing data and...
		// (3a) Send the data to the netImGui server when connected
		//-----------------------------------------------------------------------------------------
		NetImgui::EndFrame();
	}
}

//=================================================================================================
// Initialize the Textures we want the DearImGui Backend to manage for us (creation,update/destroy).
// Create each entry and initialize their pixels data with valid content.
//
// Note1:	Automatically sent to the NetImgui Server (no need for manual management).
// Note2:	Do not release the pixel data once the GPU texture has been created, it is needed
//			for partial updates, re-creation and auto transmission to NetImgui Server
// Note3:	Currently limited to the RGBA8 format
void SampleTextures::InitImguiTexture()
//=================================================================================================
{
	TexResImgui* ImTexObj(nullptr);

	//-------------------------------------------------------------------------
	GetTexInfo(EImguiTex::DefaultEmpty) = TexInfo("Empty", "Default empty texture displayed when requested texture is invalid");
	ImTexObj = &GetTexResource(EImguiTex::DefaultEmpty).InitPixels(ImTextureFormat_RGBA32, 8, 8);
	memset(ImTexObj->mImTexData.GetPixels(), 0, ImTexObj->mImTexData.GetSizeInBytes());	
	ImTexObj->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EImguiTex::White) = TexInfo("White", "Simple white texture");
	ImTexObj = &GetTexResource(EImguiTex::White).InitPixels(ImTextureFormat_RGBA32, 8, 8);
	memset(ImTexObj->mImTexData.GetPixels(), 0xFFFFFFFF, ImTexObj->mImTexData.GetSizeInBytes());
	ImTexObj->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EImguiTex::CreateDestroy) = TexInfo("Texture", "Destroy", "Create and Destroy a color texture");
	ImTexObj = &GetTexResource(EImguiTex::CreateDestroy).InitPixels(ImTextureFormat_RGBA32, 64, 64);
	for (int y(0); y < ImTexObj->mImTexData.Height; ++y)
	{
		uint8_t* pPixRow = static_cast<uint8_t*>(ImTexObj->mImTexData.GetPixelsAt(0,y));
		for (int x(0); x < ImTexObj->mImTexData.Width; ++x)
		{
			pPixRow[x * 4 + 0] = LinearToGamma8(float(x) / ImTexObj->mImTexData.Width);
			pPixRow[x * 4 + 1] = LinearToGamma8(float(y) / ImTexObj->mImTexData.Height);
			pPixRow[x * 4 + 2] = 0xFF;
			pPixRow[x * 4 + 3] = 0xFF;
		}
	}
	ImTexObj->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EImguiTex::Update) = TexInfo("Partial", "Update", "Demonstration of partial texture update support with Dear ImGui backend");
	ImTexObj = &GetTexResource(EImguiTex::Update).InitPixels(ImTextureFormat_RGBA32, 64, 64);
	for (int y(0); y < ImTexObj->mImTexData.Height; ++y)
	{
		uint8_t* pPixRow = static_cast<uint8_t*>(ImTexObj->mImTexData.GetPixelsAt(0,y));
		for (int x(0); x < ImTexObj->mImTexData.Width; ++x)
		{
			pPixRow[x * 4 + 0] = LinearToGamma8(float(x) / ImTexObj->mImTexData.Width);
			pPixRow[x * 4 + 1] = pPixRow[x * 4 + 0];
			pPixRow[x * 4 + 2] = pPixRow[x * 4 + 0];
			pPixRow[x * 4 + 3] = 0xFF;
		}
	}
	ImTexObj->Create();
}

//=================================================================================================
// Initialize the Textures we want to manually manage ourselves.
// This entail manually creating,destroying the GPU texture resources and sending them 
// to the NetImgui Server (unlike the Dear ImGui 1.92+ texture support)
//
// Some texture formats are currently not supported by the Sample Dear ImGui backend, so they
// will only be visible on the NetImgui Server, not locally
//
// Note:	This was the only way to handle texture pre Dear ImGui 1.92
void SampleTextures::InitUserTexture()
//=================================================================================================
{
	TexResUser* imTexInfo(nullptr);

	//-------------------------------------------------------------------------
	GetTexInfo(EUserTex::RGBA32) = TexInfo("RGBA8", "Standard RGBA8 texture");
	imTexInfo = &GetTexResource(EUserTex::RGBA32).InitPixels(NetImgui::eTexFormat::kTexFmtRGBA8, 64, 64);
	for (int y(0); y < imTexInfo->mHeight; ++y)
	{
		uint8_t* pPixRow = &imTexInfo->mPixels[y*imTexInfo->mWidth*4];
		for (int x(0); x < imTexInfo->mWidth; ++x){
			pPixRow[x*4 + 0] = LinearToGamma8(float(x) / imTexInfo->mWidth);
			pPixRow[x*4 + 1] = 0x00;
			pPixRow[x*4 + 2] = 0x00;
			pPixRow[x*4 + 3] = 0xFF;
		}
	}
	imTexInfo->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EUserTex::White) = TexInfo("White", "Simple white texture");
	imTexInfo = &GetTexResource(EUserTex::White).InitPixels(NetImgui::eTexFormat::kTexFmtRGBA8, 8, 8);
	memset(&imTexInfo->mPixels[0], 0xFFFFFFFF, imTexInfo->GetSizeInBytes());
	imTexInfo->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EUserTex::CreateDestroy) = TexInfo("Texture", "Destroy", "Create and Destroy a color texture (just like de Dear Imgui entry)");
	imTexInfo = &GetTexResource(EUserTex::CreateDestroy).InitPixels(NetImgui::eTexFormat::kTexFmtRGBA8, 64, 64);
	ImTextureData& ImSource = GetTexResource(EImguiTex::CreateDestroy).mImTexData; //Re-using the same pixels values as ImguiRes entry
	memcpy(&imTexInfo->mPixels[0], ImSource.GetPixels(), ImSource.GetSizeInBytes());
	imTexInfo->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EUserTex::Alpha8) = TexInfo("Alpha8", "Showcase a single channel support.\nNote: It currently ony works on NetImgui, not on local display.");
	GetTexInfo(EUserTex::Alpha8).mRemoteOnly = true;
	imTexInfo = &GetTexResource(EUserTex::Alpha8).InitPixels(NetImgui::eTexFormat::kTexFmtA8, 64, 64);
	for (int y(0); y < imTexInfo->mHeight; ++y)
	{
		uint8_t* pPixRow = &imTexInfo->mPixels[y*imTexInfo->mWidth];
		for (int x(0); x < imTexInfo->mWidth; ++x){
			pPixRow[x] = LinearToGamma8(float(x) / imTexInfo->mWidth);;
		}
	}
	imTexInfo->Create();

	//-------------------------------------------------------------------------
	GetTexInfo(EUserTex::DoubleSend) = TexInfo("Double Update", "Test", "Test creating same texture with different data twice one frame.\nTexture should stay the same.\nThis test ensure the Client and Server code handles it properly");
	GetTexInfo(EUserTex::DoubleSend).mRemoteOnly = true;
	imTexInfo = &GetTexResource(EUserTex::DoubleSend).InitPixels(NetImgui::eTexFormat::kTexFmtRGBA8, 64, 64);
	for (int y(0); y < imTexInfo->mHeight; ++y)
	{
		uint8_t* pPixRow = &imTexInfo->mPixels[y*imTexInfo->mWidth*4];
		for (int x(0); x < imTexInfo->mWidth; ++x){
			pPixRow[x*4 + 0] = 0x00;
			pPixRow[x*4 + 1] = 0x00;
			pPixRow[x*4 + 2] = LinearToGamma8(float(x) / imTexInfo->mWidth);
			pPixRow[x*4 + 3] = 0xFF;
		}
	}
	imTexInfo->Create();

	//-------------------------------------------------------------------------
#if TEXTURE_CUSTOM_SAMPLE
	GetTexInfo(EUserTex::CustomA) = TexInfo("CustomA", "Currently Unsupported");
	GetTexInfo(EUserTex::CustomA).mRemoteOnly = true;
	imTexInfo = &GetTexResource(EUserTex::CustomA).InitPixels(NetImgui::eTexFormat::kTexFmtA8, 64, 1);
	//=================================================================================================
	// Note: Example of user adding their own custom texture format
	//		 It needs to be handled by adding the code in NetImguiServer 
	//		 inside the 'ProcessTexture_Custom' function.
	//		 If user change the Server function, then this sample code won't 
	//		 be compatible anymore. 
	#if 0
		CustomTextureDataA customTextrueData;
		customTextrueData.m_ColorStart	= ImColor(0.2f,0.2f,1.f,1.f);
		customTextrueData.m_ColorEnd	= ImColor(1.f,0.2f,0.2f,1.f);
		constexpr uint32_t Width		= 8;
		constexpr uint32_t Height		= 8;
		uint8_t pixelData[Width * Height * 4]={};
		TextureCreate(pixelData, Width, Height, mCustomTextureView[static_cast<int>(eTexFmt)]);	// For local display
		NetImgui::SendDataTexture(TextureCastFromPtr(mCustomTextureView[static_cast<int>(eTexFmt)]), &customTextrueData, Width, Height, eTexFmt, sizeof(CustomTextureDataA));	// For remote display
	#endif

	GetTexInfo(EUserTex::CustomB) = TexInfo("CustomB", "Currently Unsupported");
	GetTexInfo(EUserTex::CustomB).mRemoteOnly = true;
	imTexInfo = &GetTexResource(EUserTex::CustomB).InitPixels(NetImgui::eTexFormat::kTexFmtA8, 64, 1);
#endif

	//-------------------------------------------------------------------------
	GetTexInfo(EUserTex::Invalid) = TexInfo("Invalid", "Test being able to use an invalid Texture ID on NetImgui, without any issue (would crash on local display)");
	GetTexInfo(EUserTex::Invalid).mRemoteOnly = true;
}

//==================================================================================================
// Utility method to show all informated related to a single texture entry
//-------------------------------------------------------------------------------------------------
// @return: True when the associated action button has been clicked
bool SampleTextures::DrawTextureInfo(const TexInfo& TextureInfo)
//==================================================================================================
{	
	constexpr float kBtnWidth	= 150.f;
	constexpr float kTexWidth 	= 64.f;
	constexpr float kTexHeight	= 48.f;
	bool BtnPress				= false;
	if(TextureInfo.mName[0] != 0)
	{
		ImGui::PushID(&TextureInfo);

		//---------------------------------
		ImGui::TableNextColumn();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4,4));
		if( ImGui::BeginChild("Frame", ImVec2(kTexWidth,kTexHeight+8), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::Image(TextureInfo.mTexRef, ImGui::GetContentRegionAvail(), ImVec2(0, 0), ImVec2(1, 1));
			ImGui::SetItemTooltip("%s", TextureInfo.mDescription);
		}
		ImGui::PopStyleVar();
		ImGui::EndChild();

		//---------------------------------
		ImGui::TableNextColumn();
		if( TextureInfo.mAction[0] != 0 )
		{
			BtnPress = ImGui::Button(TextureInfo.mAction, ImVec2(kBtnWidth,0));
		}

		//---------------------------------
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(TextureInfo.mName);
		ImGui::SetItemTooltip("%s", TextureInfo.mDescription);
		ImGui::PopID();
	}
	return BtnPress;
}

//=================================================================================================
// Demonstrate the ability to create and destroy a texture
// There's no need to manually signal the NetImgui Server about it, it is automically detected
void SampleTextures::DemoImActionCreateDestroy()
//=================================================================================================
{
	TexResImgui& texInfo = GetTexResource(EImguiTex::CreateDestroy);
	if (texInfo.IsValid())
	{
		texInfo.MarkForDestroy();
		GetTexInfo(EImguiTex::CreateDestroy).mAction = "Create";
	}
	else
	{
		texInfo.Create();
		GetTexInfo(EImguiTex::CreateDestroy).mAction = "Destroy";
	}
}

//=================================================================================================
// Demontrate the ability to partially update a texture
// In this sample, we send 4 partial updates in one frame
//
// Only the partially updated data gets transmitted to the NetImgui Server and there's no
// need to manually signal the server about, it is automatically detected
void SampleTextures::DemoImActionPartialUpdate()
//=================================================================================================
{
	TexResImgui& TexEntry 	= GetTexResource(EImguiTex::Update);
	ImTextureData& texData 	= TexEntry.mImTexData;
	constexpr uint32_t kUpdateCount = 4;
	constexpr uint32_t kOffsetXY[kUpdateCount][2]={{0,0}, {1,0}, {1,1}, {0,1}};
	constexpr uint32_t kColors[kUpdateCount]={0xFFFFFFFF, 0xFF0000FF, 0xFF00FF00, 0xFFFF0000}; // ABGR format
	int updateX, updateY;
	int updateW = texData.Width/4;
	int updateH = texData.Height/4;
	for(uint32_t i(0); i<kUpdateCount; ++i)
	{
		uint32_t NewColor 	= kColors[(mActionPartialUpdateCount+kUpdateCount-i)%kUpdateCount];
		updateX				= texData.Width/2	+ kOffsetXY[i][0] * updateW;
		updateY				= texData.Height/2	+ kOffsetXY[i][1] * updateH;
		for (int y = 0; y < updateH; ++y)
		{
			uint32_t* pPixelRow = reinterpret_cast<uint32_t*>(texData.GetPixelsAt(updateX, updateY+y));
			for (int x = 0; x < updateW; ++x){
				pPixelRow[x] = NewColor;
			}
		}
		TexEntry.MarkUpdated(updateX, updateY, updateW, updateH);
	}
	mActionPartialUpdateCount++;
}

//=================================================================================================
// Demonstrate the ability to create and destroy a texture
// There's no need to manually signal the NetImgui Server about it, it is automically detected
void SampleTextures::DemoUserActionCreateDestroy()
//=================================================================================================
{
	TexResUser& texInfo = GetTexResource(EUserTex::CreateDestroy);
	if (texInfo.IsValid())
	{
		texInfo.MarkForDestroy();
		GetTexInfo(EUserTex::CreateDestroy).mAction = "Create";
	}
	else
	{
		texInfo.Create();
		GetTexInfo(EUserTex::CreateDestroy).mAction = "Destroy";
	}
}


//=================================================================================================
// Test case of sending 2 texture update commands to NetImgui Server, in the same frame
// This is to potential problems with the library
void SampleTextures::DemoUserActionDoubleSend()
//=================================================================================================
{
	TexResUser& texInfoRGBA 	= GetTexResource(EUserTex::RGBA32);
	TexResUser& texInfoDouble	= GetTexResource(EUserTex::DoubleSend);
	
	// Re-used the pixel info of another texture with the DoulbeSend TextureID, 
	// then re-send the original data. This happens consecutively in the same frame 
	// and texture appearance should stay the same
	NetImgui::SendDataTexture(texInfoDouble.mTextureId, texInfoRGBA.mPixels.Data, texInfoRGBA.mWidth, texInfoRGBA.mHeight, texInfoRGBA.mFormat);
	NetImgui::SendDataTexture(texInfoDouble.mTextureId, texInfoDouble.mPixels.Data, texInfoDouble.mWidth, texInfoDouble.mHeight, texInfoDouble.mFormat);
}

//=================================================================================================
// Note: This is a 2nd example of user adding their own texture format.
//		 This one send the current time, and a sin wave is image generated.
//		 It needs to be handled by adding the code in NetImguiServer 
//		 inside the 'ProcessTexture_Custom' function.
//		 If user change the Server function, then this sample code won't 
//		 be compatible anymore. 
void SampleTextures::UpdateCustomTexture()
//=================================================================================================
{
#if TEXTURE_CUSTOM_SAMPLE
	#if 0			
	CustomTextureDataB customTextureData;
	customTextureData.m_TimeUS = static_cast<uint64_t>(ImGui::GetTime() * 1000.f * 1000.f);
	NetImgui::SendDataTexture(ImTextureID(0x02020202), &customTextureData, 128, 64, NetImgui::eTexFormat::kTexFmtCustom, sizeof(customTextureData));

	ImGui::NewLine();
	ImGui::TextUnformatted("Custom Texture");
	ImGui::ImageWithBg(ImTextureID(0x02020202), ImVec2(128, 64), ImVec2(0, 0), ImVec2(1, 1), tint_col, border_col);
	ImGui::SameLine();
	ImGui::TextWrapped("Server regenerate this texture every frame using a custom texture update and a time parameter. This behavior is custom implemented by library user on both the Client and Server codebase.");
	#endif
#endif
}


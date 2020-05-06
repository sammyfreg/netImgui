
namespace NetImgui { namespace Internal { namespace Client {


ClientTexture::~ClientTexture()
{
	SafeFree(mpCmdTexture);
}

void ClientTexture::Set( CmdTexture* pCmdTexture )
{
	SafeFree(mpCmdTexture);
	mpCmdTexture	= pCmdTexture;
	mbSent			= pCmdTexture == nullptr;
}

bool ClientTexture::IsValid()const
{
	return mpCmdTexture != nullptr;
}
	
}}} // namespace NetImgui::Internal::Client

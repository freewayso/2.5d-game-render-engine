
#ifndef ___SHELL_T_H__
#define ___SHELL_T_H__
#include "../represent_common/common.h"
#include "../represent_common/ifile.h"
#include "../represent_common/vlist.h"
#include "../represent_common/timer.h"
#include "../represent_common/media.h"
#include "bitmap_media.h"
#include "media_center.h"
#include "spr_media.h"
#include "jpg_media.h"
#include "font_media.h"
#include "ishell.h"
#include "global_config.h"

class KShell : public IKShell
{
public:
	//direct draw enviroment
	LPDIRECTDRAW		m_lpDD;
	LPDIRECTDRAWSURFACE m_lpDDSPrimary;
	LPDIRECTDRAWSURFACE m_lpDDSCanvas;
	LPDIRECTDRAWCLIPPER m_lpDDClipper;
	KPOINT				m_cScreenSize;
	KPOINT				m_cCanvasSize;
	BOOL				m_bFullScreen;
	BOOL				m_bFakeFullScreen;
	HWND				m_hWnd;
	BOOL				m_bRGB565;
	DWORD				m_wMask16;
	DWORD				m_dwMask32;
	BOOL				m_bGpuLost;

	//camera
	int					m_nLeft;
	int					m_nTop;

	//font map
	map<int, DWORD>		m_mapFont;		//id(size) -> handle

	// Replay ...Add by Lucien
	IJXReplay*			m_pJxPlayer;		// 剑侠录播器
	int					m_nFrameCounter;	// 帧计数器
	int					m_nReplayerState;	// 录播器状态

	//frame control
	bool	Create(HWND hWnd, int nWidth, int nHeight, bool& bFullScreen, const KRepresentConfig* pRepresentConfig);
	bool	Reset(int nWidth, int nHeight, bool bFullScreen, bool bNotAdjustStyle = true);
	void	Release();
	bool	RepresentBegin(int bClear, DWORD Color);
	void	RepresentEnd();

	bool	OverLoad()
	{ 
		DWORD dwSpace = g_dwMediaBytes/1024/1024;
		if (dwSpace < g_cGlobalConfig.m_dwMediaSpaceLimit)
		{
			return false;
		}
		return true;
	}

	//camera
	void	LookAt(int nX, int nY, int nZ, int nAdj)
	{
		if (NULL != m_pJxPlayer)
		{
			m_pJxPlayer->RECjxr(
				m_nFrameCounter, 
				enum_RepreFun_LookAt, 
				"xyz", nX, nY, nZ);
		}

		m_nLeft = nX - m_cCanvasSize.x / 2;
		m_nTop  = nY / 2 - ((nZ * 887) >> 10) - m_cCanvasSize.y / 2;
	}
	void	ViewPortCoordToSpaceCoord(int& nX, int& nY, int nZ)
	{
		nX = nX + m_nLeft;
		nY = (nY + m_nTop + ((nZ * 887) >> 10)) * 2;
	}
	void	SpaceToScreen(int& nX, int& nY, int nZ)
	{
		nX = nX - m_nLeft;
		nY = nY / 2 - m_nTop - ((nZ * 887) >> 10);
	}

	//font & text
	bool	CreateAFont(LPCSTR pszFontFile, DWORD CharaSet, int nId);
	void	OutputText(int nFontId, const WCHAR* psText, int nCount = -1, int nX = (-2147483647 - 1), int nY = (-2147483647 - 1), DWORD Color = 0xff000000, int nLineWidth = 0, int nZ = -32767, DWORD BorderColor = 0, const RECT* pClipRect = 0);
	int		OutputRichText(int nFontId, void* pParam, const WCHAR* psText, int nCount = -1, int nLineWidth = 0, const RECT* pClipRect = 0);

	//image info
	bool	GetImageParam(LPCSTR pszImage, void* pImageData, DWORD nType, bool no_cache = false);
	bool	GetImageFrameParam(LPCSTR pszImage, int nFrame, KPOINT* pOffset, KPOINT* pSize, DWORD nType, bool no_cache = false);
	int		GetImagePixelAlpha(LPCSTR pszImage, int nFrame, int nX, int nY, int nType);

	//image lib
	void	ReleaseAllImages() {}

	//dynamic bitmaps
	DWORD	CreateImage(LPCSTR pszName, int nWidth, int nHeight, DWORD nType);
	void	ClearImageData(LPCSTR pszImage, DWORD uImage, DWORD nImagePosition);
	void*	GetBitmapDataBuffer(LPCSTR pszImage, void* pInfo);
	void	ReleaseBitmapDataBuffer(LPCSTR pszImage, void* pBuffer);
	bool	ImageNeedReDraw(char* szFileName, DWORD& uImage, DWORD& nPos, int& bImageExist) {bImageExist = true; return false;}

	//draw primitives
	void	DrawPrimitives(int nPrimitiveCount, void* pPrimitives, DWORD uGenre, int bSinglePlaneCoord, KPRIMITIVE_INDEX_LIST** pStandBy = 0);
	void	DrawPrimitivesOnImage(int nPrimitiveCount, void* pPrimitives, DWORD uGenre, LPCSTR pszImage, DWORD uImage, DWORD& nImagePosition, int bForceDrawFlag = false);

	//jpg loader
	void*	GetJpgImage(const char cszName[], unsigned uRGBMask16 = ((unsigned)-1));
	void	ReleaseImage(void* pImage);

	//screen capture
	bool	SaveScreenToFile(LPCSTR pszName, DWORD eType, DWORD nQuality);

	// 设置每帧加载器的加载量
	// nNumOfTask 每帧加载媒体的个数
	VOID SetTaskLimit(UINT uNumOfTask)
	{
		g_cGlobalConfig.m_dwPerFrameTaskCountLimit = uNumOfTask;
		KMedia::SetTaskLimit(g_cGlobalConfig.m_dwPerFrameTaskCountLimit);
	}

	// 设置媒体缓存空间上限
	// uSize 缓存上限 （单位M）
	VOID SetMediaSpaceLimit(UINT uSize){g_cGlobalConfig.m_dwMediaSpaceLimit = uSize;}

	// 设置媒体缓存时间
	// uSize 缓存时间 （单位秒）
	VOID SetMediaTimeLimit(UINT uSize){g_cGlobalConfig.m_dwMediaTimeLimit = uSize;}

	//not used...
	void*	Get3DDevice() {return 0;}
	DWORD	SetAdjustColorList(DWORD* puColorList, DWORD uCount) {return 0;}
	int		LocateRichText(int nX, int nY, int nFontId, void* pParam, LPCSTR psText, int nCount = -1, int nLineWidth = 0) {return 0;}
	void	ReleaseAFont(int nId) {}
	void	FreeImage(LPCSTR pszImage) {}
	void	FreeAllImage() {}
	HRESULT ConverSpr(char* pFileName, char* pFileNameTo, int nType) {return S_OK;}
	void	SetImageStoreBalanceParam(int nNumImage, DWORD uCheckPoint = 1000) {}
	void	LookAtEx(D3DXVECTOR3& vecCamera, D3DXVECTOR3& vecLookAt) {}
	bool	CopyDeviceImageToImage(LPCSTR pszName, int nDeviceX, int nDeviceY, int nImageX, int nImageY, int nWidth, int nHeight) {return true;}
	void	AddLight(KLight Light) {}
	int		AdviseRepresent(void*) {return 0;}
	int		UnAdviseRepresent(void*) {return 0;}
	int		GetRepresentParam(DWORD lCommand, int& lParam, DWORD& uParam) 
	{
		enum
		{
			_cmd_alpha_level_ = 100,
		};

		switch( lCommand )
		{
		case _cmd_alpha_level_ :
			lParam = g_sSprConfig.m_dwAlphaLevel;
			break;
		default:
			break;
		}
		return 0;
	}
	int		SetRepresentParam(DWORD lCommand, int lParam, DWORD uParam) 
	{
		enum
		{
			_cmd_alpha_level_ = 100,
		};

		switch( lCommand )
		{
		case _cmd_alpha_level_ :
			{
				if( g_sSprConfig.m_dwAlphaLevel != lParam )
				{
					CollectGarbage( 0, 0 );
					g_sSprConfig.m_dwAlphaLevel = lParam;
				}
			}
			break;
		default:
			break;
		}
		return 0;
	}
	void*	CreateRepresentObject(DWORD uGenre, LPCSTR pObjectName, int nParam1, int nParam2) {return 0;}
	void*	Create3DEffectObject(LPCSTR szFileName) {return 0;}
	void*	Create3DEffectObjectEx(LPCSTR szFileName) {return 0;}
	void	ReleaseImage(char* pszImage) {}
	int		PreLoad(DWORD uType, const char cszName[], int nReserve) {return 0;}

	//Replay ...Add by Lucien
	//设置当前录像播放器 
	void SetJxPlayer(IJXReplay* pJxPlayer)
	{
		m_pJxPlayer = pJxPlayer;
	}
	// 设置当前是第几帧
	void SetCurFrame(int nCurFrame)
	{
		m_nFrameCounter = nCurFrame;
	}
	// 设置当前是第几帧, 和当前录像机额状态（之所以要重载这个函数是为了避免玩家在不能及时更新时会导致出错的情况）
	void SetCurFrame(int nCurFrame, int nJxReplayerState)
	{
		m_nFrameCounter = nCurFrame;
		m_nReplayerState = nJxReplayerState;
	}
	//zjq add 09.2.9	暴露ddraw和dds给外部使用
#ifdef _SNDA_IGW_
	LPDIRECTDRAW GetIDirectDraw()
	{
		return m_lpDD;
	}
	LPDIRECTDRAWSURFACE GetIDirectDrawSurface()
	{
		return m_lpDDSCanvas;
	}
	D3DPRESENT_PARAMETERS* GetD3DRepresentParam()
	{
		return NULL;
	}
#endif
	//end add

};

enum
{
	emPRIMITIVE_TYPE_PIC			= 3,
	emPRIMITIVE_TYPE_PIC_PART		= 4,
	emPRIMITIVE_TYPE_PIC_PART_FOUR	= 5,
	emPRIMITIVE_TYPE_POINT			= 0,
	emPRIMITIVE_TYPE_LINE			= 1,
	emPRIMITIVE_TYPE_RECT_FRAME		= 2,
	emPRIMITIVE_TYPE_RECT			= 8,
};

enum KE_RENDER_STYLE
{
	emRENDER_STYLE_ALPHA_MODULATE_ALPHA_BLENDING = 1 << 0,
	emRENDER_STYLE_ALPHA_COLOR_MODULATE_ALPHA_BLENDING = 1 << 3,
	emRENDER_STYLE_ALPHA_BLENDING = 1 << 6,
	emRENDER_STYLE_COPY = 1 << 1,
	emRENDER_STYLE_SCREEN = 3,
};

enum KE_REF_METHOD
{
	emREF_METHOD_TOP_LEFT		= 0,
	emREF_METHOD_CENTER			= 1,
	emREF_METHOD_FRAME_TOP_LEFT	= 2,
};

enum KE_PIC_TYPE
{
	emPIC_TYPE_JPG_OR_LOCKABLE,
	emPIC_TYPE_SPR,
	emPIC_TYPE_RT,
};

struct KPicCmd
{
	D3DXVECTOR3		sPos1;
	D3DXVECTOR3		sPos2;
	DWORD			dwColor;
	BYTE			byRenderStyle;
	BYTE			byRefMethod;
	DWORD			dwType;
	char			szFileName[128];
	char			szSubstrituteFileName[128];
	DWORD			dwHandle1;
	DWORD			dwHandle2;
	short			wFrameIndex;
	DWORD			dwExchangeColor;
};

struct KPicCmdPart : public KPicCmd
{
	KPOINT			cTopLeft;
	KPOINT			cBottomRight;
};

struct KPicCmdPartFour : public KPicCmdPart
{
	D3DXVECTOR3			sPos3;
	D3DXVECTOR3			sPos4;
};


#endif
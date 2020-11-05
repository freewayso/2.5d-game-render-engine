/* -------------------------------------------------------------------------
//	文件名		：	_shell_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	shell的具体实现，整个渲染部分的外壳，对外提供各种功能接口
//
// -----------------------------------------------------------------------*/
#ifndef ___SHELL_T_H__
#define ___SHELL_T_H__
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "../represent_common/common.h"
#include "ishell.h"
#include "igpu.h"
#include "gpu.h"
#include "gpu_helper.h"
#include "../represent_common/ifile.h"
#include "../represent_common/vlist.h"
#include "../represent_common/timer.h"
#include "../represent_common/media.h"
#include "media_center.h"
#include "spr_media.h"
#include "jpg_media.h"
#include "rt_media.h"
#include "lockable_media.h"
#include "font_media.h"
#include "kernel_center.h"
#include "kernel_picture.h"
#include "kernel_rect.h"
#include "kernel_text.h"
#include "kernel_line.h"
#include "ishell.h"
#include "global_config.h"


enum
{
	emPRIMITIVE_TYPE_PIC			= 3,
	emPRIMITIVE_TYPE_PIC_PART		= 4,
	emPRIMITIVE_TYPE_PIC_PART_FOUR	= 5,
	emPRIMITIVE_TYPE_RECT			= 8,
	emPRIMITIVE_TYPE_LINE			= 1,
	emPRIMITIVE_TYPE_RECT_FRAME		= 2,
};

extern DWORD g_dwCallTimes1; // tmp: spr绘制次数

class KShell : public IKShell
{
public:
	//d3d stuff
	bool				m_bRedrawGround;
	bool				m_bGpuLost;
	D3DDISPLAYMODE		m_sDesktopMode;

	//camera
	INT					m_nLeft;
	INT					m_nTop;

	//font map
	map<INT, DWORD>		m_mapFont;	//id(size) -> handle

	// Replay ...Add by Lucien
	IJXReplay*			m_pJxPlayer;		// 剑侠录播器
	INT					m_nFrameCounter;	// 帧计数器
	INT					m_nReplayerState;	// 录播器状态

	//frame control
	bool	Create(HWND hWnd, INT nWidth, INT nHeight, bool& bFullScreen, const KRepresentConfig* pRepresentConfig);
	bool	Reset(INT nWidth, INT nHeight, bool bFullScreen, bool bNotAdjustStyle = true);
	void	Release();
	bool	RepresentBegin(INT bClear, DWORD Color);
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
	void	LookAt(INT nX, INT nY, INT nZ, INT nAdj)
	{
		if (NULL != m_pJxPlayer)
		{
			m_pJxPlayer->RECjxr(
				m_nFrameCounter, 
				enum_RepreFun_LookAt, 
				"xyz", nX, nY, nZ);
		}

		m_nLeft = nX - Gpu()->GetPresentationParameters()->dwBackbufferWidth / 2;
		m_nTop  = nY / 2 - ((nZ * 887) >> 10) - Gpu()->GetPresentationParameters()->dwBackbufferHeight / 2;
	}
	void	ViewPortCoordToSpaceCoord(INT& nX, INT& nY, INT nZ)
	{
		nX = nX + m_nLeft;
		nY = (nY + m_nTop + ((nZ * 887) >> 10)) * 2;
	}
	void	SpaceToScreen(INT& nX, INT& nY, INT nZ)
	{
		nX = nX - m_nLeft;
		nY = nY / 2 - m_nTop - ((nZ * 887) >> 10);
	}

	//font & text
	bool	CreateAFont(LPCSTR pszFontFile, DWORD CharaSet, INT nId);
	void	OutputText(INT nFontId, const WCHAR* psText, INT nCount = -1, INT nX = (-2147483647 - 1), INT nY = (-2147483647 - 1), DWORD Color = 0xff000000, INT nLineWidth = 0, INT nZ = -32767, DWORD BorderColor = 0, const RECT* pClipRect = 0);
	INT		OutputRichText(INT nFontId, void* pParam, const WCHAR* psText, INT nCount = -1, INT nLineWidth = 0, const RECT* pClipRect = 0);

	//image info
	bool	GetImageParam(LPCSTR pszImage, void* pImageData, DWORD nType, bool bNoCache = false);
	bool	GetImageFrameParam(LPCSTR pszImage, INT nFrame, KPOINT* pOffset, KPOINT* pSize, DWORD nType, bool bNoCache = false);
	INT		GetImagePixelAlpha(LPCSTR pszImage, INT nFrame, INT nX, INT nY, INT nType);

	//image lib
	void	ReleaseAllImages() {}

	//dynamic bitmaps
	DWORD	CreateImage(LPCSTR pszName, INT nWidth, INT nHeight, DWORD nType);
	void	ClearImageData(LPCSTR pszImage, DWORD uImage, DWORD nImagePosition);
	void*	GetBitmapDataBuffer(LPCSTR pszImage, void* pInfo);
	void	ReleaseBitmapDataBuffer(LPCSTR pszImage, void* pBuffer);
	bool	ImageNeedReDraw(char* szFileName, DWORD& uImage, DWORD& nPos, INT& bImageExist) {bImageExist = true; return m_bRedrawGround;}

	//draw primitives
	void	DrawPrimitives(INT nPrimitiveCount, void* pPrimitives, DWORD uGenre, INT bSinglePlaneCoord, KPRIMITIVE_INDEX_LIST** pStandby = 0);
	void	DrawPrimitivesOnImage(INT nPrimitiveCount, void* pPrimitives, DWORD uGenre, LPCSTR pszImage, DWORD uImage, DWORD& nImagePosition, INT bForceDrawFlag = false);

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
	VOID SetMediaSpaceLimit(UINT uSize){ g_cGlobalConfig.m_dwMediaSpaceLimit = uSize; }
	// 设置媒体缓存时间
	// uSize 缓存时间 （单位秒）
	VOID SetMediaTimeLimit(UINT uSize){ g_cGlobalConfig.m_dwMediaTimeLimit = uSize; }

	//not used...
	void*	Get3DDevice() {return g_Gpu.m_pDevice;}				//zjq mod 9.3.9		增加接口方便给IGW使用
	DWORD	SetAdjustColorList(DWORD* puColorList, DWORD uCount) {return 0;}
	INT		LocateRichText(INT nX, INT nY, INT nFontId, void* pParam, LPCSTR psText, INT nCount = -1, INT nLineWidth = 0) {return 0;}
	void	ReleaseAFont(INT nId) {}
	void	FreeImage(LPCSTR pszImage) {}
	void	FreeAllImage() {}
	HRESULT ConverSpr(char* pFileName, char* pFileNameTo, INT nType) {return S_OK;}
	void	SetImageStoreBalanceParam(INT nNumImage, DWORD uCheckPoint = 1000) {}
	void	LookAtEx(D3DXVECTOR3& vecCamera, D3DXVECTOR3& vecLookAt) {}
	bool	CopyDeviceImageToImage(LPCSTR pszName, INT nDeviceX, INT nDeviceY, INT nImageX, INT nImageY, INT nWidth, INT nHeight) {return true;}
	void	AddLight(KLight Light) {}
	INT		AdviseRepresent(void*) {return 0;}
	INT		UnAdviseRepresent(void*) {return 0;}
	INT		GetRepresentParam(DWORD lCommand, INT& lParam, DWORD& uParam);
	INT		SetRepresentParam(DWORD lCommand, INT lParam, DWORD uParam);
	void*	CreateRepresentObject(DWORD uGenre, LPCSTR pObjectName, INT nParam1, INT nParam2) {return 0;}
	void*	Create3DEffectObject(LPCSTR szFileName) {return 0;}
	void*	Create3DEffectObjectEx(LPCSTR szFileName) {return 0;}
	void	ReleaseImage(char* pszImage) {}
	INT		PreLoad(DWORD uType, const char cszName[], INT nReserve) {return 0;}

	//Replay ...Add by Lucien
	//设置当前录像播放器 
	void SetJxPlayer(IJXReplay* pJxPlayer)
	{
		m_pJxPlayer = pJxPlayer;
	}

	// 设置当前是第几帧
	void SetCurFrame(INT nCurFrame)
	{
		m_nFrameCounter = nCurFrame;
	}
	// 设置当前是第几帧, 和当前录像机额状态（之所以要重载这个函数是为了避免玩家在不能及时更新时会导致出错的情况）
	void SetCurFrame(INT nCurFrame, INT nJxReplayerState)
	{
		m_nFrameCounter = nCurFrame;
		m_nReplayerState = nJxReplayerState;
	}
	//zjq add 09.2.9	暴露ddraw和dds给外部使用
#ifdef _SNDA_IGW_
	LPDIRECTDRAW GetIDirectDraw()
	{
		return NULL;
	}
	LPDIRECTDRAWSURFACE GetIDirectDrawSurface()
	{
		return NULL;
	}
	D3DPRESENT_PARAMETERS* GetD3DRepresentParam()
	{
		return &(g_Gpu.m_d3dPresentParameters);
	}
#endif
	//end add

};

bool CreateMultiquadIndexBuffer(DWORD quad_count, LPD3DIB* out);
bool CheckFont(LPCSTR typeface, DWORD char_set, HWND hwnd);
bool RectIntersect(const RECT* rc1, const RECT* rc2, RECT* out);
//screen capture
bool SaveToBmpFileInMemory_8888(LPD3DBUFFER* out, void* pBitmap, INT nPitch, INT nWidth, INT nHeight);
bool SaveToBmpFile_8888(LPCSTR lpFileName, void* pBitmap, INT nPitch, INT nWidth, INT nHeight);
//jepg encoder
INT GetEncoderClsid(const wchar_t* format, CLSID* pClsid);
bool SaveToJpgFile_8888(LPCSTR lpFileName, void* pBitmap, INT nPitch, INT nWidth, INT nHeight, DWORD nQuality);
#endif

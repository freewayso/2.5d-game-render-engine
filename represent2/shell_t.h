/* -------------------------------------------------------------------------
//	文件名		：	shell_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	shell的基类定义，整个渲染部分的外壳，对外提供各种功能接口
//
// -----------------------------------------------------------------------*/
#ifndef __SHELL_H__
#define __SHELL_H__
#include "common.h"
//shell
struct KPRIMITIVE_INDEX
{
	UINT	uImage;		// 图像ID
	INT		nFrame;		// 图像帧序号
	INT		nRenderStyle;
};

struct KPRIMITIVE_INDEX_LIST
{
	KPRIMITIVE_INDEX		sIndex;	// 图元索引数据
	KPRIMITIVE_INDEX_LIST*	pNext;
};

struct shell_t
{
	struct KLight
	{
		int		m_nX,m_nY,m_nZ;
		DWORD   m_dwColor;
		int	    m_nRadius;
	};
	virtual void	Release() = 0;
	virtual bool	Create(HWND hWnd, int nWidth, int nHeight, bool& bFullScreen) = 0;
	virtual bool	Reset(int nWidth, int nHeight, bool bFullScreen, bool bNotAdjustStyle = true) = 0;
	virtual void*	Get3DDevice() = 0;
	virtual bool	CreateAFont(const char* pszFontFile, DWORD CharaSet, int nId) = 0;
	virtual void	OutputText(int nFontId, const WCHAR* psText, int nCount = -1, int nX = (-2147483647 - 1), int nY = (-2147483647 - 1), DWORD Color = 0xff000000, int nLineWidth = 0, int nZ = -32767, DWORD BorderColor = 0, const RECT* pClipRect = 0) = 0;
	virtual DWORD	SetAdjustColorList(DWORD* puColorList, DWORD uCount) = 0;
	virtual int		OutputRichText(int nFontId, void* pParam, const WCHAR* psText, int nCount = -1, int nLineWidth = 0, const RECT* pClipRect = 0) = 0;
	virtual int		LocateRichText(int nX, int nY, int nFontId, void* pParam, const char* psText, int nCount = -1, int nLineWidth = 0) = 0;
	virtual void	ReleaseAFont(int nId) = 0;
	virtual DWORD	CreateImage(const char* pszName, int nWidth, int nHeight, DWORD nType) = 0;
	virtual void	FreeImage(const char* pszImage) = 0;
	virtual void	FreeAllImage() = 0;
	virtual void*	GetBitmapDataBuffer(const char* pszImage, void* pInfo) = 0;
	virtual void	ReleaseBitmapDataBuffer(const char* pszImage, void* pBuffer) = 0;
	virtual bool	GetImageParam(const char* pszImage, void* pImageData, DWORD nType, bool no_cache = false) = 0;
	virtual bool	GetImageFrameParam(const char* pszImage, int nFrame, KPOINT* pOffset, KPOINT* pSize, DWORD nType, bool no_cache = false) = 0;
	virtual int		GetImagePixelAlpha(const char* pszImage, int nFrame, int nX, int nY, int nType) = 0;
	virtual HRESULT ConverSpr(char* pFileName, char* pFileNameTo, int nType) = 0;
	virtual void	SetImageStoreBalanceParam(int nNumImage, DWORD uCheckPoint = 1000) = 0;
	virtual void	DrawPrimitives(int nPrimitiveCount, void* pPrimitives, DWORD uGenre, int bSinglePlaneCoord, KPRIMITIVE_INDEX_LIST** pStandBy = 0) = 0;
	virtual void	DrawPrimitivesOnImage(int nPrimitiveCount, void* pPrimitives, DWORD uGenre, const char* pszImage, DWORD uImage, DWORD& nImagePosition, int bForceDrawFlag = false) = 0;
	virtual void	ClearImageData(const char* pszImage, DWORD uImage, DWORD nImagePosition) = 0;
	virtual bool	ImageNeedReDraw(char* szFileName, DWORD& uImage, DWORD& nPos, int& bImageExist) = 0;
	virtual void	LookAt(int nX, int nY, int nZ, int nAdj) = 0;
	virtual void	LookAtEx(D3DXVECTOR3& vecCamera, D3DXVECTOR3& vecLookAt) = 0;
	virtual bool	CopyDeviceImageToImage(const char* pszName, int nDeviceX, int nDeviceY, int nImageX, int nImageY, int nWidth, int nHeight) = 0;
	virtual bool	RepresentBegin(int bClear, DWORD Color) = 0;
	virtual void	RepresentEnd() = 0;
	virtual void	AddLight(KLight Light) = 0;
	virtual void	ViewPortCoordToSpaceCoord(int& nX,	int& nY, int nZ) = 0;
	virtual int		AdviseRepresent(void*) = 0;
	virtual int		UnAdviseRepresent(void*) = 0;
	virtual bool	SaveScreenToFile(const char* pszName, DWORD eType, DWORD nQuality) = 0;
	virtual int		GetRepresentParam(DWORD lCommand, int& lParam, DWORD& uParam) = 0;
	virtual int		SetRepresentParam(DWORD lCommand, int lParam, DWORD uParam) = 0;
	virtual void*	CreateRepresentObject(DWORD uGenre, const char* pObjectName, int nParam1, int nParam2) = 0;
	virtual void*	Create3DEffectObject(const char* szFileName) = 0;
	virtual void*	Create3DEffectObjectEx(const char* szFileName) = 0;
	virtual void*	GetJpgImage(const char cszName[], unsigned uRGBMask16 = ((unsigned)-1)) = 0;
	virtual void	ReleaseImage(void* pImage) = 0;
	virtual void	ReleaseImage(char* pszImage) = 0;
	virtual int		PreLoad(DWORD uType, const char cszName[], int nReserve) = 0;
	virtual void	ReleaseAllImages() = 0;
	// 设置当前录像播放器 
	virtual void	SetJxPlayer(IJXReplay* pJxPlayer) = 0;
	// 设置当前是第几帧
	virtual void	SetCurFrame(int nCurFrame) = 0;
	// 设置当前是第几帧, 和当前录像机额状态（之所以要重载这个函数是为了避免玩家在不能及时更新时会导致出错的情况）
	virtual void	SetCurFrame(int nCurFrame, int nJxReplayerState) = 0;
	//zjq add 09.2.9	暴露ddraw和dds给外部使用
#ifdef _SNDA_IGW_
	virtual LPDIRECTDRAW GetIDirectDraw() = 0;
	virtual LPDIRECTDRAWSURFACE GetIDirectDrawSurface() = 0;
	virtual D3DPRESENT_PARAMETERS* GetD3DRepresentParam() = 0;
#endif
	//end add
};

#endif
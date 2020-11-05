/* -------------------------------------------------------------------------
//	文件名		：	spr_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	spr_media_t定义
//
// -----------------------------------------------------------------------*/
#ifndef __SPR_MEDIA_H__
#define __SPR_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "../represent_common/ifile.h"
#include "sprite.h"
#include "media_center.h"
#include "spr_frame_media.h"

struct KSprInitParam
{
	BOOL	bRGB565;
	DWORD	dwAlphaLevel;
};

struct KSprMedia : public KMedia, public KSprite
{
	//src
	char				m_szFilename[128];

	//loaded
	BOOL				m_bPacked;
	LPD3DBUFFER			m_pFiledata;			//fragment 0 for packed, whole file for unpacked
	LPD3DBUFFER			m_pOffsetTableBuf;	//fragment 1 for packed, null for unpacked

	//processed (= produced)
	//	BOOL			packed;
	KPOINT				m_cSize;
	KPOINT				m_cCenter;
	DWORD				m_dwColorCount;
	DWORD				m_dwExchangeColorCount;
	LPD3DBUFFER			m_pPalBuf;
	LPD3DBUFFER			m_pExchangePalBuf;
	DWORD				m_dwBlendStyle;
	DWORD				m_dwDirectionCount;
	DWORD				m_dwInterval;
	vector<KMedia*>		m_vecFrame;

	KSprMedia( LPCSTR pszFileName );

	//media
	void	Load();
	void	Process();
	void	Produce() {}
	void	PostProcess() {}
	void	DiscardVideoObjects() {}
	void	Clear();
	void	Destroy() {Clear(); delete this;}
	void*	Product() {return (!m_vecFrame.empty()) ? (KSprite*)this : 0;}

	//sprite 
	const KPOINT*	GetSize()									{return &m_cSize;}
	const KPOINT*	GetCenter()									{return &m_cCenter;}
	DWORD		GetFrameCount()									{return (DWORD)m_vecFrame.size();}
	DWORD		GetColorCount()									{return m_dwColorCount;}
	DWORD		GetExchangeColorCount()							{return m_dwExchangeColorCount;}
	void*		GetPalette16bit()								{return m_pPalBuf ? m_pPalBuf->GetBufferPointer() : 0;}
	void*		GetExchangePalette24bit()						{return m_pExchangePalBuf ? m_pExchangePalBuf->GetBufferPointer() : 0;}
	DWORD		GetBlendStyle()									{return m_dwBlendStyle;}
	DWORD		GetDirectionCount()								{return m_dwDirectionCount;}
	DWORD		GetInterval()									{return m_dwInterval;}
	BOOL		QueryFrame(DWORD dwIndex, BOOL bWait, void** ppFrame)	{return (dwIndex < m_vecFrame.size()) ? m_vecFrame[dwIndex]->RequestProduct(m_bPacked ? bWait : true, ppFrame) : false;}
};

BOOL InitSprCreator(void* pArg);
void ShutdownSprCreator();

KMedia* CreateSpr(LPCSTR pszIdentifier, void* pArg);
void Pal24ToPal16(void* pPal24, void* pPal16, int nColors, BOOL bRGB565);
#endif
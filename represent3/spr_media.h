/* -------------------------------------------------------------------------
//	文件名		：	spr_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	spr_media_t定义
//
// -----------------------------------------------------------------------*/
#ifndef __SPR_MEDIA_H__
#define __SPR_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "spr_frame_media.h"

struct KSprInitParam
{
	BOOL	bUseCompressedTextureFormat;
	BOOL	bUse16bitColor;
	BOOL	bEnableConditionalNonPow2;
	DWORD	dwMipmap;
};

class KSprMedia : public KMedia, public KSprite
{
public:
	//src
	CHAR				m_szFilename[128];
	DWORD				m_dwExchangeColor;
	vector<DWORD>		m_vecFrameHandle;
	BOOL				m_bFrameGenerated;

	//loaded
	BOOL				m_bPacked;
	LPD3DBUFFER			m_pFiledata;		//fragment 0 for packed, whole file for unpacked
	LPD3DBUFFER			m_pOffsetTableBuf;	//fragment 1 for packed, null for unpacked

	//processed (= produced)
	KPOINT				m_cSize;
	KPOINT				m_cCenter;
	DWORD				m_dwExchangeColorCount;
	DWORD				m_dwBlendStyle;
	DWORD				m_dwDirectionCount;
	DWORD				m_dwInterval;
	vector<KMedia*>		m_vecFrame;

	//ctor
	KSprMedia(LPCSTR pszFileName, DWORD dwExchangeColor);

	//media_t
	void	Load();
	void	Process();
	void	Produce() {}
	void	PostProcess();
	void	DiscardVideoObjects() {}
	void	Clear();
	void	Destroy() {SAFE_RELEASE(m_pFiledata);	SAFE_RELEASE(m_pOffsetTableBuf); delete this;}
	void*	Product() {return (!m_vecFrame.empty()) ? (KSprite*)this : 0;}

	//sprite_t 
	const KPOINT*	GetSize()			{return &m_cSize;}
	const KPOINT*	GetCenter()			{return &m_cCenter;}
	DWORD		GetExchangeColorCount()	{return m_dwExchangeColorCount;}
	DWORD		GetBlendStyle()			{return m_dwBlendStyle;}
	DWORD		GetDirectionCount()		{return m_dwDirectionCount;}
	DWORD		GetInterval()			{return m_dwInterval;}
	DWORD		GetFrameCount()			{return (DWORD)m_vecFrame.size();}
	BOOL		QueryFrame(DWORD dwMyHandle, DWORD dwIndex, BOOL bWait, void** ppFrame) {BOOL ret = QueryProduct(m_vecFrameHandle[dwIndex], bWait, ppFrame); QueryProductWithoutReturn(dwMyHandle); return ret;}
};

BOOL InitSprCreator(void* pArg);

void ShutdownSprCreator();

KMedia* CreateSpr(LPCSTR pszIdentifier, void* pArg);

WORD x888To4444(BYTE a, BYTE r, BYTE g, BYTE b);

#endif
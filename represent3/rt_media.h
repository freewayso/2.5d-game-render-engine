/* -------------------------------------------------------------------------
//	文件名		：	rt_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	rt_media_t的定义
//
// -----------------------------------------------------------------------*/
#ifndef __RT_MEDIA_H__
#define __RT_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "../represent_common/ifile.h"
//media - rt
struct KRTConfig
{
	D3DFORMAT	eFmt;
	DWORD		dwMipmap;
	BOOL		bEnableConditionalNonPow2;
};

extern KRTConfig g_sRTConfig;

struct KRTInitParam
{
	BOOL	bUse16bitColor;
	BOOL	bEnableConditionalNonPow2;
	DWORD	dwMipmap;
};

class KRTMedia : public KMedia, public KSprite, public KSpriteFrame
{
public:
	//src
	KPOINT			m_cSize;
	LPD3DTEXTURE	m_pBackupTexture;

	//processed (= produced)
	KPOINT			m_cTextureSize;
	D3DXVECTOR2		m_sUVScale;

	//video objects
	LPD3DTEXTURE	m_pTexture;

	//ctor
	KRTMedia(const KPOINT* pSize);

	//media_t
	void	Load() {}
	void	Process();
	void	Produce() {}
	void	PostProcess() {}
	void	DiscardVideoObjects() {SAFE_RELEASE(m_pTexture);}
	void	Clear();
	void	Destroy() {SAFE_RELEASE(m_pBackupTexture); SAFE_RELEASE(m_pTexture); delete this;}
	void*	Product() {return (KSprite*)this;}

	//sprite_t
	const KPOINT*		GetSize()					{return &m_cSize;}
	const KPOINT*		GetCenter()				{return &g_PointZero;}
	DWORD			GetExchangeColorCount()	{return 0;}
	DWORD			GetBlendStyle()			{return 0;}
	DWORD			GetDirectionCount()		{return 1;}
	DWORD			GetInterval()				{return 1;}
	DWORD			GetFrameCount()			{return 1;}
	BOOL			QueryFrame(DWORD dwMyHandle, DWORD dwIndex, BOOL bWait, void** ppFrame) {*ppFrame = (KSpriteFrame*)this; return TRUE;}

	//sprite_frame_t
	const KPOINT*		GetFrameSize()	{return &m_cSize;}
	const KPOINT*		GetFrameOffset()	{return &g_PointZero;}
	const KPOINT*		GetTextureSize()	{return &m_cTextureSize;}
	LPD3DTEXTURE	GetTexture();
	const D3DXVECTOR2*	GetUVScale()		{return &m_sUVScale;}
};

KMedia* CreateRT(LPCSTR identifier, void* arg);

BOOL InitRTCreator(void* arg);

void ShutdownRTCreator();

#endif
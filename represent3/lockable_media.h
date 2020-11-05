/* -------------------------------------------------------------------------
//	文件名		：	lockable_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	lockable_media_t和lockable_frame_t的定义
//
// -----------------------------------------------------------------------*/
#ifndef __LOCKABLE_MEDIA_H__
#define __LOCKABLE_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "../represent_common/ifile.h"
//media - lockable
struct KLockableConfig
{
	D3DFORMAT	eFmt;
	BOOL		bEnableConditionalNonPow2;
};
extern KLockableConfig g_sLockableConfig;

struct KLockableInitParam
{
	BOOL bEnableConditionalNonPow2;
};

struct KLockableFrame : public KSpriteFrame
{
	virtual void*	Lock(DWORD* pPitch) = 0;
	virtual void	UnLock() = 0;
};

class KLockableMedia : public KMedia, public KSprite, public KLockableFrame
{
public:
	//src 
	KPOINT			m_cSize;
	KPOINT			m_cTextureSize;
	D3DXVECTOR2		m_sUVScale;
	LPD3DTEXTURE	m_pTexture;

	//ctor
	KLockableMedia( const KPOINT* pSize );

	//media_t
	void	Load() {}
	void	Process() {}
	void	Produce() {}
	void	PostProcess() {}
	void	DiscardVideoObjects() {}
	void	Clear() {}
	void	Destroy();
	void*	Product() {return m_pTexture ? (KSprite*)this : 0;}

	//sprite_t
	const KPOINT*	GetSize()				{return &m_cSize;}
	const KPOINT*	GetCenter()				{return &g_PointZero;}
	DWORD			GetExchangeColorCount()	{return 0;}
	DWORD			GetBlendStyle()			{return 0;}
	DWORD			GetDirectionCount()		{return 1;}
	DWORD			GetInterval()				{return 1;}
	DWORD			GetFrameCount()			{return 1;}
	BOOL			QueryFrame(DWORD dwMyHandle, DWORD dwIndex, BOOL bWait, void** ppFrame) {*ppFrame = (KSpriteFrame*)this; return true;}

	//frame
	const KPOINT*		GetFrameSize()	{return &m_cSize;}
	const KPOINT*		GetFrameOffset()	{return &g_PointZero;}
	const KPOINT*		GetTextureSize()	{return &m_cTextureSize;}
	LPD3DTEXTURE		GetTexture()		{return m_pTexture;}
	const D3DXVECTOR2*	GetUVScale()		{return &m_sUVScale;}
	void*				Lock(DWORD* pPitch);
	void				UnLock();
};

KMedia* CreateLockable(LPCSTR identifier, void* arg);

BOOL InitLockableCreator(void* arg);

void ShutdownLockableCreator();

#endif
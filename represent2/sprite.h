/* -------------------------------------------------------------------------
//	文件名		：	sprite.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-25 11:11:11
//	功能描述	：	sprite_t与sprite_frame_t的定义
//
// -----------------------------------------------------------------------*/
#ifndef __SPRITE__
#define __SPRITE__
#include "../represent_common/common.h"

struct KSprite
{
	virtual const KPOINT*	GetSize() = 0;
	virtual const KPOINT*	GetCenter() = 0;
	virtual DWORD		GetFrameCount() = 0;
	virtual DWORD		GetColorCount() = 0;
	virtual DWORD		GetExchangeColorCount() = 0;
	virtual void*		GetPalette16bit() = 0;
	virtual void*		GetExchangePalette24bit() = 0;
	virtual DWORD		GetBlendStyle() = 0;
	virtual DWORD		GetDirectionCount() = 0;
	virtual DWORD		GetInterval() = 0;
	virtual BOOL		QueryFrame(DWORD dwIndex, BOOL bWait, void** ppFrame) = 0;
};

struct KSpriteFrame
{
	virtual const KPOINT*		GetFrameSize() = 0;
	virtual const KPOINT*		GetFrameOffset() = 0;
	virtual void*				GetFrameData() = 0;
};

#endif
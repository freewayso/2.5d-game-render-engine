/* -------------------------------------------------------------------------
//	文件名		：	sprite.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	sprite_t与sprite_frame_t的定义
//
// -----------------------------------------------------------------------*/
#ifndef __SPRITE__
#define __SPRITE__
#include "../represent_common/common.h"
class KSprite
{
public:
	virtual const KPOINT*	GetSize() = 0;
	virtual const KPOINT*	GetCenter() = 0;
	virtual DWORD		GetFrameCount() = 0;
	virtual DWORD		GetExchangeColorCount() = 0;
	virtual DWORD		GetBlendStyle() = 0;
	virtual DWORD		GetDirectionCount() = 0;
	virtual DWORD		GetInterval() = 0;
	virtual BOOL		QueryFrame(DWORD myhandle, DWORD index, BOOL wait, void** frame) = 0;
};

class KSpriteFrame
{
public:
	virtual const KPOINT*		GetFrameSize() = 0;
	virtual const KPOINT*		GetFrameOffset() = 0;
	virtual const KPOINT*		GetTextureSize() = 0;
	virtual LPD3DTEXTURE		GetTexture() = 0;
	virtual const D3DXVECTOR2*	GetUVScale() = 0;
};

//inline int	GetColorDt(int& nColor)
//{
//	int ndt = 0;
//	if (nColor < 0)
//	{
//		ndt = nColor;
//		nColor = 0;
//	}
//	if (nColor > 255)
//	{
//		ndt = nColor - 255;
//		nColor = 255;
//	}
//	return ndt;
//}
//

#endif
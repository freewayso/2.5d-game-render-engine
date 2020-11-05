/* -------------------------------------------------------------------------
//	文件名		：	bitmap_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	bitmap_t定义
//
// -----------------------------------------------------------------------*/
#ifndef __BITMAP_H__
#define __BITMAP_H__
#include "../represent_common/common.h"
struct IKBitmap_t
{
	virtual const KPOINT* GetSize() = 0;
	virtual void*		GetData() = 0;	
	virtual DWORD		GetFormat() = 0;
};
#endif
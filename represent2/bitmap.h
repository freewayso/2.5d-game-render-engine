/* -------------------------------------------------------------------------
//	�ļ���		��	bitmap_t.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-26 11:11:11
//	��������	��	bitmap_t����
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
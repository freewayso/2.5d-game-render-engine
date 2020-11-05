/* -------------------------------------------------------------------------
//	文件名		：	bitmap_media_t.CPP
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	bitmap_media_t定义
//
// -----------------------------------------------------------------------*/
#include "bitmap_media.h"
//media - bitmap
BOOL InitBitmapCreator(void* arg) {return true;}
void ShutdownBitmapCreator() {}

KMedia* CreateBitmap(LPCSTR identifier, void* arg)
{
	KCreateBitmapParam* param = (KCreateBitmapParam*)arg;
	return new KBitmapMedia(&param->m_pSize, param->m_dwFormat);
}

/* -------------------------------------------------------------------------
//	�ļ���		��	bitmap_media_t.CPP
//	������		��	fenghewen
//	����ʱ��	��	2009-11-26 11:11:11
//	��������	��	bitmap_media_t����
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

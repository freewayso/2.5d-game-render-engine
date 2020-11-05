/* -------------------------------------------------------------------------
//	�ļ���		��	jpglib.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-20 11:11:11
//	��������	��	jpg��ӿڷ�װ
//
// -----------------------------------------------------------------------*/
//jpglib.lib
#ifndef __JPGLIB_H__
#define __JPGLIB_H__
#include "../represent_common/common.h"
extern "C" {
	typedef struct
	{
		int nMode;
		int nWidth;
		int nHeight;
	} JPEG_INFO;
	BOOL	jpeg_decode_init(BOOL bRGB555, BOOL bMMXCPU);
	BOOL	jpeg_decode_info(BYTE* pJpgBuf, JPEG_INFO* pInfo);
	BOOL	jpeg_decode_data(WORD* pBmpBuf, JPEG_INFO* pInfo);
	BOOL	jpeg_decode_dataEx(WORD* pBmpBuf, INT pitch, JPEG_INFO* pInfo);
}

void InitJpglib();
void ShutdownJpglib();
void EnterJpglib();
void LeaveJpglib();

#endif
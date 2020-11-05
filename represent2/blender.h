/* -------------------------------------------------------------------------
//	�ļ���		��	blender.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-26 11:11:11
//	��������	��	��ɫ�桢��ɫ�㷨���塢��ɫ��ص�ת��
//
// -----------------------------------------------------------------------*/
#ifndef __BLENDER_H__
#define __BLENDER_H__
#include "../represent_common/common.h"

extern __int64 g_n64ColorMask;

// ��ɫ��
void ModulatePalette(WORD* pDst, WORD* pSrc, DWORD dwCount, DWORD dwColor, BOOL bRGB565);

int	GetColorDt(int& nColor);

void GenerateExchangeColorPalette(WORD* pDst, COLORCOUNT* pSrc, DWORD dwCount, DWORD dwColor, BOOL bRGB565);

//color functions
WORD Color32To16( DWORD dwColor32, BOOL bRGB565 );

// ��ɫ�㷨
WORD AlphaBlend(WORD wDst, WORD wSrc, BYTE byAlpha, BOOL bRGB565);

void ScreenBlend1Pixel(void* pDest, void* pColor, void* pAlpha );

void ScreenBlend2Pixel(void* pDest, void* pColor, void* pAlpha );

void ScreenBlend4Pixel(void*  pDest, void*  pColor, void*  pAlpha );

inline WORD ScreenBlend(WORD wDst, WORD wSrc, WORD wSrcAlpha, BOOL bRGB565)
{
	WORD wSrcR, wSrcG, wSrcB;
	if( !bRGB565 )
	{
		wSrcR = (wSrc & 0x7c00) >> 10;
		wSrcG = (wSrc & 0x03e0) >> 5;
		wSrcB = (wSrc & 0x001f);
	}
	else
	{
		wSrcR = (wSrc & 0xf800) >> 11;
		wSrcG = (wSrc & 0x07c0) >> 6;
		wSrcB = (wSrc & 0x001f);
	}

	wSrcR = wSrcR * wSrcAlpha / 255;
	wSrcG = wSrcG * wSrcAlpha / 255;
	wSrcB = wSrcB * wSrcAlpha / 255;

	WORD wDstR, wDstG, wDstB;
	if( !bRGB565 )
	{
		wDstR = (wDst & 0x7c00) >> 10;
		wDstG = (wDst & 0x03e0) >> 5;
		wDstB = (wDst & 0x001f);
	}
	else
	{
		wDstR = (wDst & 0xf800) >> 11;
		wDstG = (wDst & 0x07c0) >> 6;
		wDstB = (wDst & 0x001f);
	}

	WORD wDstAMask = 0;

	WORD wAnsR, wAnsG, wAnsB;
	wAnsR = (wSrcR  + wDstR  - wSrcR * wDstR / 0x1f) & 0x1f;
	wAnsG = (wSrcG  + wDstG  - wSrcG * wDstG / 0x1f) & 0x1f;
	wAnsB = (wSrcB  + wDstB  - wSrcB * wDstB / 0x1f) & 0x1f;

	return (!bRGB565) ? (wDstAMask | (wAnsR << 10) | (wAnsG << 5) | wAnsB) : (wDstAMask | (wAnsR << 11) | (wAnsG << 6) | wAnsB);
}

#endif
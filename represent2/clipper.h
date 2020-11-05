/* -------------------------------------------------------------------------
//	文件名		：	clipper.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	裁剪器
//
// -----------------------------------------------------------------------*/
#ifndef __CLIPPER_H__
#define __CLIPPER_H__
#include "../represent_common/common.h"


struct KRClipperInfo
{
	INT			nX;			// 裁减后的X坐标
	INT			nY;			// 裁减后的Y坐标
	INT			nWidth;		// 裁减后的宽度
	INT			nHeight;		// 裁减后的高度
	INT			nLeft;		// 上边界裁剪量
	INT			nTop;		// 左边界裁剪量
	INT			nRight;		// 右边界裁剪量
};

//功能：计算剪切范围
//参数：nX,nY,nSrcWidth,nSrcHeight -> 给出源范围
//		nDestWidth,nDestHeight -> 目标的范围限定
//		pClipper -> 存储剪切计算的结果
static INT  RIO_ClipCopyRect(INT nX, INT nY, INT nSrcWidth, INT nSrcHeight, INT nDestWidth, INT nDestHeight, KRClipperInfo* pClipper);

//功能：覆盖形式复制spr图形到缓冲区，不做alpha计算。
VOID RIO_CopySprToBuffer(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
//功能：覆盖形式复制spr图形到缓冲区，不做alpha计算。
VOID RIO_CopySprToAlphaBuffer(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY);
//功能：带alpha计算地复制spr图形到缓冲区
VOID RIO_CopySprToBufferAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
inline WORD RIO_AlphaBlend(WORD wDst, WORD wSrc, BYTE byAlpha, BOOL bRGB565)
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

	WORD wDstR, wDstG, wDstB;
	wDstR = (wDst & 0x7c00) >> 10;
	wDstG = (wDst & 0x03e0) >> 5;
	wDstB = (wDst & 0x001f);

	WORD wDstAMask = 0;

	WORD wAnsR, wAnsG, wAnsB;
	wAnsR = ((wSrcR * byAlpha + wDstR * (255 - byAlpha)) / 255) & 0x1f;
	wAnsG = ((wSrcG * byAlpha + wDstG * (255 - byAlpha)) / 255) & 0x1f;
	wAnsB = ((wSrcB * byAlpha + wDstB * (255 - byAlpha)) / 255) & 0x1f;

	return wDstAMask | (wAnsR << 10) | (wAnsG << 5) | wAnsB;
}
//功能：带alpha计算地复制spr图形到带透明模式的缓冲区
VOID RIO_CopySprToAlphaBufferAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
//功能：三阶alpha形式地复制spr图形到缓冲区
VOID RIO_CopyBitmap16ToBuffer(LPVOID pBitmap, INT nBmpWidth, INT nBmpHeight, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY);
//功能：覆盖形式复制16位位图到缓冲区
VOID RIO_CopySprToBuffer3LevelAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
//功能：覆盖形式复制16位位图到带透明模式的缓冲区
VOID RIO_CopySprToAlphaBuffer3LevelAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);

#endif

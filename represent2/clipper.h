/* -------------------------------------------------------------------------
//	�ļ���		��	clipper.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-26 11:11:11
//	��������	��	�ü���
//
// -----------------------------------------------------------------------*/
#ifndef __CLIPPER_H__
#define __CLIPPER_H__
#include "../represent_common/common.h"


struct KRClipperInfo
{
	INT			nX;			// �ü����X����
	INT			nY;			// �ü����Y����
	INT			nWidth;		// �ü���Ŀ��
	INT			nHeight;		// �ü���ĸ߶�
	INT			nLeft;		// �ϱ߽�ü���
	INT			nTop;		// ��߽�ü���
	INT			nRight;		// �ұ߽�ü���
};

//���ܣ�������з�Χ
//������nX,nY,nSrcWidth,nSrcHeight -> ����Դ��Χ
//		nDestWidth,nDestHeight -> Ŀ��ķ�Χ�޶�
//		pClipper -> �洢���м���Ľ��
static INT  RIO_ClipCopyRect(INT nX, INT nY, INT nSrcWidth, INT nSrcHeight, INT nDestWidth, INT nDestHeight, KRClipperInfo* pClipper);

//���ܣ�������ʽ����sprͼ�ε�������������alpha���㡣
VOID RIO_CopySprToBuffer(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
//���ܣ�������ʽ����sprͼ�ε�������������alpha���㡣
VOID RIO_CopySprToAlphaBuffer(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY);
//���ܣ���alpha����ظ���sprͼ�ε�������
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
//���ܣ���alpha����ظ���sprͼ�ε���͸��ģʽ�Ļ�����
VOID RIO_CopySprToAlphaBufferAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
//���ܣ�����alpha��ʽ�ظ���sprͼ�ε�������
VOID RIO_CopyBitmap16ToBuffer(LPVOID pBitmap, INT nBmpWidth, INT nBmpHeight, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY);
//���ܣ�������ʽ����16λλͼ��������
VOID RIO_CopySprToBuffer3LevelAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);
//���ܣ�������ʽ����16λλͼ����͸��ģʽ�Ļ�����
VOID RIO_CopySprToAlphaBuffer3LevelAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32);

#endif

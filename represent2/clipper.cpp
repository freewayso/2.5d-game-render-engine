/* -------------------------------------------------------------------------
//	文件名		：	clipper.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	裁剪器,计算剪切范围
//
// -----------------------------------------------------------------------*/
#include "clipper.h"
#include "render_helper.h"

//功能：计算剪切范围
//参数：nX,nY,nSrcWidth,nSrcHeight -> 给出源范围
//		nDestWidth,nDestHeight -> 目标的范围限定
//		pClipper -> 存储剪切计算的结果
static INT  RIO_ClipCopyRect(INT nX, INT nY, INT nSrcWidth, INT nSrcHeight, INT nDestWidth, INT nDestHeight, KRClipperInfo* pClipper)
{
	// 初始化裁减量
	pClipper->nX = nX;
	pClipper->nY = nY;
	pClipper->nWidth = nSrcWidth;
	pClipper->nHeight = nSrcHeight;
	pClipper->nTop = 0;
	pClipper->nLeft = 0;
	pClipper->nRight = 0;

	// 上边界裁减
	if (pClipper->nY < 0)
	{
		pClipper->nY = 0;
		pClipper->nTop = -nY;
		pClipper->nHeight += nY;
	}
	if (pClipper->nHeight <= 0)
		return 0;

	// 下边界裁减
	if (pClipper->nHeight > nDestHeight - pClipper->nY)
		pClipper->nHeight = nDestHeight - pClipper->nY;
	if (pClipper->nHeight <= 0)
		return 0;

	// 左边界裁减
	if (pClipper->nX < 0)
	{
		pClipper->nX = 0;
		pClipper->nLeft = -nX;
		pClipper->nWidth += nX;
	}
	if (pClipper->nWidth <= 0)
		return 0;

	// 右边界裁减
	if (pClipper->nWidth > nDestWidth - pClipper->nX)
	{
		pClipper->nRight = pClipper->nWidth + pClipper->nX - nDestWidth;
		pClipper->nWidth -= pClipper->nRight;
	}
	if (pClipper->nWidth <= 0)
		return 0;

	return 1;
}

VOID RIO_CopySprToBuffer(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	// pBuffer指向屏幕起点的偏移位置 (以字节计)
	INT nSprSkip = nSprWidth * Clipper.nTop + Clipper.nLeft;
	INT nSprSkipPerLine = Clipper.nLeft + Clipper.nRight;
	INT nBuffNextLine;

	__asm
	{
		//ebx = nPitch = nBufferWidth * 2
		mov		ebx, nBufferWidth
			add		ebx, ebx

			//使edi指向buffer绘制起点edi = (CHAR*)pBuffer + Clipper.y * nPitch + Clipper.x * 2;
			mov		edi, pBuffer
			mov		eax, Clipper.nY
			mul		ebx
			mov		edx, Clipper.nX
			add		eax, edx
			add		eax, edx
			add		edi, eax

			//nBuffNextLine = nPitch - Clipper.width * 2;
			mov		edx, Clipper.nWidth
			add		edx, edx
			sub		ebx, edx
			mov		nBuffNextLine, ebx

			//使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
			mov		esi, pSpr
			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
					or		ebx, ebx
					jnz		_SkipSpriteAheadContentLocalAlpha_
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					neg		edx
					jmp		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalAlpha_:
				{
					add		esi, eax
						sub		edx, eax
						jg		_SkipSpriteAheadContentLocalStart_
						add		esi, edx
						neg		edx
						jmp		_SkipSpriteAheadContentEnd_
				}
			}
		}
_SkipSpriteAheadContentEnd_:

		mov		eax, nSprSkipPerLine
			or		eax, eax
			jnz		_DrawPartLineSection_	//if (nSprSkipPerLine) goto _DrawPartLineSection_

			//_DrawFullLineSection_:
		{
			//因为sprite不会跨行压缩，则运行到此处edx必为0，如sprite会跨行压缩则_DrawFullLineSection_需改
_DrawFullLineSection_Line_:
		{
			mov		edx, Clipper.nWidth
_DrawFullLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.nHeight
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					sub		edx, eax
						mov		ecx, eax
						mov     ebx, pPalette
_DrawFullLineSection_CopyPixel_:
					{
						KD_RIO_COPY_PIXEL_USE_EAX
							loop	_DrawFullLineSection_CopyPixel_
					}
					or		edx, edx
						jnz		_DrawFullLineSection_LineLocal_

						add		edi, nBuffNextLine
						dec		Clipper.nHeight
						jnz		_DrawFullLineSection_Line_
						jmp		_EXIT_WAY_
				}
			}
		}
		}

_DrawPartLineSection_:
		{
			mov		eax, Clipper.nLeft
				or		eax, eax
				jz		_DrawPartLineSection_SkipRight_Line_

				mov		eax, Clipper.nRight
				or		eax, eax
				jz		_DrawPartLineSection_SkipLeft_Line_
		}

_DrawPartLineSection_Line_:
			{
				mov		eax, edx
					mov		edx, Clipper.nWidth
					or		eax, eax
					jnz		_DrawPartLineSection_LineLocal_CheckAlpha_

_DrawPartLineSection_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_

						add		edi, edx
						add		edi, edx
						neg		edx
				}

_DrawPartLineSection_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		eax, edx
							mov		edx, nSprSkipPerLine
							or		eax, eax
							jnz		_DrawPartLineSection_LineSkipLocal_CheckAlpha_
_DrawPartLineSection_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
							or		ebx, ebx
								jnz		_DrawPartLineSection_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_LineSkipLocal_
								neg		edx
								jmp		_DrawPartLineSection_Line_
_DrawPartLineSection_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_LineSkipLocal_
									add		esi, edx
									neg		edx
									jmp		_DrawPartLineSection_Line_
							}
						}
					}
_DrawPartLineSection_LineLocal_Alpha_:
					{
						sub		edx, eax
							jle		_DrawPartLineSection_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

							mov		ecx, eax
							mov     ebx, pPalette

_DrawPartLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_CopyPixel_
						}
						jmp		_DrawPartLineSection_LineLocal_
					}

_DrawPartLineSection_LineLocal_Alpha_Part_:
						{
							mov		ecx, eax
								add		eax, edx
								mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_Part_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_CopyPixel_Part_
							}

							dec		Clipper.nHeight
								jz		_EXIT_WAY_
								neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								jmp		_DrawPartLineSection_LineSkip_
						}
			}
_DrawPartLineSection_SkipLeft_Line_:
			{
				mov		eax, edx
					mov		edx, Clipper.nWidth
					or		eax, eax
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
_DrawPartLineSection_SkipLeft_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipLeft_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_
				}

_DrawPartLineSection_SkipLeft_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		edx, nSprSkipPerLine
_DrawPartLineSection_SkipLeft_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
								or		ebx, ebx
								jnz		_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
								neg		edx
								jmp		_DrawPartLineSection_SkipLeft_Line_
_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
									add		esi, edx
									neg		edx
									jmp		_DrawPartLineSection_SkipLeft_Line_
							}
						}
					}
_DrawPartLineSection_SkipLeft_LineLocal_Alpha_:
					{
						sub		edx, eax		;先把eax减了，这样後面就可以不需要保留eax了
							mov		ecx, eax						
							mov     ebx, pPalette
_DrawPartLineSection_SkipLeft_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_SkipLeft_CopyPixel_
						}
						or		edx, edx
							jnz		_DrawPartLineSection_SkipLeft_LineLocal_
							dec		Clipper.nHeight
							jg		_DrawPartLineSection_SkipLeft_LineSkip_
							jmp		_EXIT_WAY_
					}
			}

_DrawPartLineSection_SkipRight_Line_:
			{
				mov		edx, Clipper.nWidth
_DrawPartLineSection_SkipRight_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
						or		ebx, ebx
						jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipRight_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_

						add		edi, edx
						add		edi, edx
						neg		edx
				}

_DrawPartLineSection_SkipRight_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		eax, edx
							mov		edx, nSprSkipPerLine
							or		eax, eax
							jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_
_DrawPartLineSection_SkipRight_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_:
							or		ebx, ebx
								jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
								jmp		_DrawPartLineSection_SkipRight_Line_
_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
									jmp		_DrawPartLineSection_SkipRight_Line_
							}
						}
					}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_:
					{
						sub		edx, eax
							jle		_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

							mov		ecx, eax				
							mov     ebx, pPalette
_DrawPartLineSection_SkipRight_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_SkipRight_CopyPixel_
						}
						jmp		_DrawPartLineSection_SkipRight_LineLocal_
					}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
						{
							add		eax, edx
								mov		ecx, eax
								mov     ebx, pPalette
_DrawPartLineSection_SkipRight_CopyPixel_Part_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_SkipRight_CopyPixel_Part_
							}
							neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								dec		Clipper.nHeight
								jg		_DrawPartLineSection_SkipRight_LineSkip_
								jmp		_EXIT_WAY_
						}
			}
_EXIT_WAY_:
	}
}

//功能：覆盖形式复制spr图形到缓冲区，不做alpha计算。
VOID RIO_CopySprToAlphaBuffer(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	INT nSprSkip = nSprWidth * Clipper.nTop + Clipper.nLeft;
	INT nSprSkipPerLine = Clipper.nLeft + Clipper.nRight;
	INT nBuffNextLine;

	__asm
	{
		//ebx = nPitch = nBufferWidth * 2
		mov		ebx, nBufferWidth
			add		ebx, ebx

			//使edi指向buffer绘制起点edi = (CHAR*)pBuffer + Clipper.y * nPitch + Clipper.x * 2;
			mov		edi, pBuffer
			mov		eax, Clipper.nY
			mul		ebx
			mov		edx, Clipper.nX
			add		eax, edx
			add		eax, edx
			add		edi, eax

			//nBuffNextLine = nPitch - Clipper.width * 2;
			mov		edx, Clipper.nWidth
			add		edx, edx
			sub		ebx, edx
			mov		nBuffNextLine, ebx

			//使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
			mov		esi, pSpr
			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
					or		ebx, ebx
					jnz		_SkipSpriteAheadContentLocalAlpha_
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					neg		edx
					jmp		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalAlpha_:
				{
					add		esi, eax
						sub		edx, eax
						jg		_SkipSpriteAheadContentLocalStart_
						add		esi, edx
						neg		edx
						jmp		_SkipSpriteAheadContentEnd_
				}
			}
		}
_SkipSpriteAheadContentEnd_:

		mov		eax, nSprSkipPerLine
			or		eax, eax
			jnz		_DrawPartLineSection_	//if (nSprSkipPerLine) goto _DrawPartLineSection_

			//_DrawFullLineSection_:
		{
			//因为sprite不会跨行压缩，则运行到此处edx必为0，如sprite会跨行压缩则_DrawFullLineSection_需改
_DrawFullLineSection_Line_:
		{
			mov		edx, Clipper.nWidth
_DrawFullLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.nHeight
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					sub		edx, eax
						mov		ecx, eax
						mov     ebx, pPalette
_DrawFullLineSection_CopyPixel_:
					{
						KD_RIO_COPY_PIXEL_USE_EAX_1555
							loop	_DrawFullLineSection_CopyPixel_
					}
					or		edx, edx
						jnz		_DrawFullLineSection_LineLocal_

						add		edi, nBuffNextLine
						dec		Clipper.nHeight
						jnz		_DrawFullLineSection_Line_
						jmp		_EXIT_WAY_
				}
			}
		}
		}

_DrawPartLineSection_:
		{
			mov		eax, Clipper.nLeft
				or		eax, eax
				jz		_DrawPartLineSection_SkipRight_Line_

				mov		eax, Clipper.nRight
				or		eax, eax
				jz		_DrawPartLineSection_SkipLeft_Line_
		}

_DrawPartLineSection_Line_:
			{
				mov		eax, edx
					mov		edx, Clipper.nWidth
					or		eax, eax
					jnz		_DrawPartLineSection_LineLocal_CheckAlpha_

_DrawPartLineSection_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_

						add		edi, edx
						add		edi, edx
						neg		edx
				}

_DrawPartLineSection_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		eax, edx
							mov		edx, nSprSkipPerLine
							or		eax, eax
							jnz		_DrawPartLineSection_LineSkipLocal_CheckAlpha_
_DrawPartLineSection_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
							or		ebx, ebx
								jnz		_DrawPartLineSection_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_LineSkipLocal_
								neg		edx
								jmp		_DrawPartLineSection_Line_
_DrawPartLineSection_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_LineSkipLocal_
									add		esi, edx
									neg		edx
									jmp		_DrawPartLineSection_Line_
							}
						}
					}
_DrawPartLineSection_LineLocal_Alpha_:
					{
						sub		edx, eax
							jle		_DrawPartLineSection_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

							mov		ecx, eax
							mov     ebx, pPalette

_DrawPartLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX_1555
								loop	_DrawPartLineSection_CopyPixel_
						}
						jmp		_DrawPartLineSection_LineLocal_
					}

_DrawPartLineSection_LineLocal_Alpha_Part_:
						{
							mov		ecx, eax
								add		eax, edx
								mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_Part_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX_1555
									loop	_DrawPartLineSection_CopyPixel_Part_
							}

							dec		Clipper.nHeight
								jz		_EXIT_WAY_
								neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								jmp		_DrawPartLineSection_LineSkip_
						}
			}
_DrawPartLineSection_SkipLeft_Line_:
			{
				mov		eax, edx
					mov		edx, Clipper.nWidth
					or		eax, eax
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
_DrawPartLineSection_SkipLeft_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipLeft_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_
				}

_DrawPartLineSection_SkipLeft_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		edx, nSprSkipPerLine
_DrawPartLineSection_SkipLeft_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
								or		ebx, ebx
								jnz		_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
								neg		edx
								jmp		_DrawPartLineSection_SkipLeft_Line_
_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
									add		esi, edx
									neg		edx
									jmp		_DrawPartLineSection_SkipLeft_Line_
							}
						}
					}
_DrawPartLineSection_SkipLeft_LineLocal_Alpha_:
					{
						sub		edx, eax		;先把eax减了，这样後面就可以不需要保留eax了
							mov		ecx, eax						
							mov     ebx, pPalette
_DrawPartLineSection_SkipLeft_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX_1555
								loop	_DrawPartLineSection_SkipLeft_CopyPixel_
						}
						or		edx, edx
							jnz		_DrawPartLineSection_SkipLeft_LineLocal_
							dec		Clipper.nHeight
							jg		_DrawPartLineSection_SkipLeft_LineSkip_
							jmp		_EXIT_WAY_
					}
			}

_DrawPartLineSection_SkipRight_Line_:
			{
				mov		edx, Clipper.nWidth
_DrawPartLineSection_SkipRight_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
						or		ebx, ebx
						jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipRight_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_

						add		edi, edx
						add		edi, edx
						neg		edx
				}

_DrawPartLineSection_SkipRight_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		eax, edx
							mov		edx, nSprSkipPerLine
							or		eax, eax
							jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_
_DrawPartLineSection_SkipRight_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_:
							or		ebx, ebx
								jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
								jmp		_DrawPartLineSection_SkipRight_Line_
_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
									jmp		_DrawPartLineSection_SkipRight_Line_
							}
						}
					}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_:
					{
						sub		edx, eax
							jle		_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

							mov		ecx, eax				
							mov     ebx, pPalette
_DrawPartLineSection_SkipRight_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX_1555
								loop	_DrawPartLineSection_SkipRight_CopyPixel_
						}
						jmp		_DrawPartLineSection_SkipRight_LineLocal_
					}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
						{
							add		eax, edx
								mov		ecx, eax
								mov     ebx, pPalette
_DrawPartLineSection_SkipRight_CopyPixel_Part_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX_1555
									loop	_DrawPartLineSection_SkipRight_CopyPixel_Part_
							}
							neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								dec		Clipper.nHeight
								jg		_DrawPartLineSection_SkipRight_LineSkip_
								jmp		_EXIT_WAY_
						}
			}
_EXIT_WAY_:
	}
}

//功能：带alpha计算地复制spr图形到缓冲区
VOID RIO_CopySprToBufferAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	// pBuffer指向屏幕起点的偏移位置 (以字节计)
	INT nSprSkip = nSprWidth * Clipper.nTop + Clipper.nLeft;
	INT nSprSkipPerLine = Clipper.nLeft + Clipper.nRight;
	INT	 nBuffNextLine, nAlpha;

	__asm
	{
		//ebx = nPitch = nBufferWidth * 2
		mov		ebx, nBufferWidth
			add		ebx, ebx

			//使edi指向buffer绘制起点edi = (CHAR*)pBuffer + Clipper.y * nPitch + Clipper.x * 2;
			mov		edi, pBuffer
			mov		eax, Clipper.nY
			mul		ebx
			mov		edx, Clipper.nX
			add		eax, edx
			add		eax, edx
			add		edi, eax

			//nBuffNextLine = nPitch - Clipper.width * 2;
			mov		edx, Clipper.nWidth
			add		edx, edx
			sub		ebx, edx
			mov		nBuffNextLine, ebx

			//使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
			mov		esi, pSpr
			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
					or		ebx, ebx
					jnz		_SkipSpriteAheadContentLocalAlpha_
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					neg		edx
					jmp		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalAlpha_:
				{
					add		esi, eax
						sub		edx, eax
						jg		_SkipSpriteAheadContentLocalStart_
						add		esi, edx
						neg		edx
						jmp		_SkipSpriteAheadContentEnd_
				}
			}
		}
_SkipSpriteAheadContentEnd_:

		mov		eax, nSprSkipPerLine
			or		eax, eax
			jnz		_DrawPartLineSection_	//if (nSprSkipPerLine) goto _DrawPartLineSection_

			//_DrawFullLineSection_:
		{
			//因为sprite不会跨行压缩，则运行到此处edx必为0，如sprite会跨行压缩则_DrawFullLineSection_需改			
_DrawFullLineSection_Line_:
		{
			mov		edx, Clipper.nWidth
_DrawFullLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.nHeight
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					sub		edx, eax
						mov		ecx, eax

						cmp		ebx, 255
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
					{
						mov     ebx, pPalette
_DrawFullLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawFullLineSection_CopyPixel_
						}
						or		edx, edx
							jnz		_DrawFullLineSection_LineLocal_

							add		edi, nBuffNextLine
							dec		Clipper.nHeight
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
					}

_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							push	edx
								shr		ebx, 3
								mov		nAlpha, ebx
_DrawFullLineSection_HalfAlphaPixel_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
									loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							pop		edx
								or		edx, edx
								jnz		_DrawFullLineSection_LineLocal_

								add		edi, nBuffNextLine
								dec		Clipper.nHeight
								jnz		_DrawFullLineSection_Line_
								jmp		_EXIT_WAY_
						}
				}
			}
		}
		}

_DrawPartLineSection_:
		{
			mov		eax, Clipper.nLeft
				or		eax, eax
				jz		_DrawPartLineSection_SkipRight_Line_

				mov		eax, Clipper.nRight
				or		eax, eax
				jz		_DrawPartLineSection_SkipLeft_Line_
		}

_DrawPartLineSection_Line_:
			{
				mov		eax, edx
					mov		edx, Clipper.nWidth
					or		eax, eax
					jnz		_DrawPartLineSection_LineLocal_CheckAlpha_
_DrawPartLineSection_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_

						add		edi, edx
						add		edi, edx
						neg		edx
				}

_DrawPartLineSection_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		eax, edx
							mov		edx, nSprSkipPerLine
							or		eax, eax
							jnz		_DrawPartLineSection_LineSkipLocal_CheckAlpha_
_DrawPartLineSection_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
							or		ebx, ebx
								jnz		_DrawPartLineSection_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_LineSkipLocal_
								neg		edx
								jmp		_DrawPartLineSection_Line_
_DrawPartLineSection_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_LineSkipLocal_
									add		esi, edx
									neg		edx
									jmp		_DrawPartLineSection_Line_
							}
						}
					}
_DrawPartLineSection_LineLocal_Alpha_:
					{
						sub		edx, eax
							jle		_DrawPartLineSection_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

							mov		ecx, eax
							cmp		ebx, 255
							jl		_DrawPartLineSection_LineLocal_HalfAlpha_

							//_DrawPartLineSection_LineLocal_DirectCopy_:
						{
							mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_CopyPixel_
							}
							jmp		_DrawPartLineSection_LineLocal_
						}

_DrawPartLineSection_LineLocal_HalfAlpha_:
							{
								push	edx
									shr		ebx, 3
									mov		nAlpha, ebx
_DrawPartLineSection_HalfAlphaPixel_:
								{
									KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
										loop	_DrawPartLineSection_HalfAlphaPixel_
								}
								pop		edx
									jmp		_DrawPartLineSection_LineLocal_
							}
					}
_DrawPartLineSection_LineLocal_Alpha_Part_:
					{
						mov		ecx, eax
							add		eax, edx
							cmp		ebx, 255
							jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_

							//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
						{
							mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_Part_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_CopyPixel_Part_
							}

							dec		Clipper.nHeight
								jz		_EXIT_WAY_
								neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								jmp		_DrawPartLineSection_LineSkip_
						}

_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
							{
								push	edx
									shr		ebx, 3
									mov		nAlpha, ebx
_DrawPartLineSection_HalfAlphaPixel_Part_:
								{
									KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
										loop	_DrawPartLineSection_HalfAlphaPixel_Part_
								}
								pop		edx
									neg		edx
									mov		ebx, 128
									dec		Clipper.nHeight
									jg		_DrawPartLineSection_LineSkip_//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
									jmp		_EXIT_WAY_
							}
					}
			}

_DrawPartLineSection_SkipLeft_Line_:
			{
				mov		eax, edx
					mov		edx, Clipper.nWidth
					or		eax, eax
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
_DrawPartLineSection_SkipLeft_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipLeft_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_
				}

_DrawPartLineSection_SkipLeft_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		edx, nSprSkipPerLine
_DrawPartLineSection_SkipLeft_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
								or		ebx, ebx
								jnz		_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
								neg		edx
								jmp		_DrawPartLineSection_SkipLeft_Line_
_DrawPartLineSection_SkipLeft_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_SkipLeft_LineSkipLocal_
									add		esi, edx
									neg		edx
									jmp		_DrawPartLineSection_SkipLeft_Line_
							}
						}
					}
_DrawPartLineSection_SkipLeft_LineLocal_Alpha_:
					{
						sub		edx, eax		;先把eax减了，这样後面就可以不需要保留eax了
							mov		ecx, eax
							cmp		ebx, 255
							jl		_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_

							//_DrawPartLineSection_SkipLeft_LineLocal_DirectCopy_:
						{
							mov     ebx, pPalette
_DrawPartLineSection_SkipLeft_CopyPixel_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_SkipLeft_CopyPixel_
							}
							or		edx, edx
								jnz		_DrawPartLineSection_SkipLeft_LineLocal_
								dec		Clipper.nHeight
								jg		_DrawPartLineSection_SkipLeft_LineSkip_
								jmp		_EXIT_WAY_
						}

_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
							{
								push	edx
									shr		ebx, 3
									mov		nAlpha, ebx
_DrawPartLineSection_SkipLeft_HalfAlphaPixel_:
								{
									KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
										loop	_DrawPartLineSection_SkipLeft_HalfAlphaPixel_
								}
								pop		edx
									or		edx, edx
									jnz		_DrawPartLineSection_SkipLeft_LineLocal_
									dec		Clipper.nHeight
									jg		_DrawPartLineSection_SkipLeft_LineSkip_
									jmp		_EXIT_WAY_
							}
					}
			}

_DrawPartLineSection_SkipRight_Line_:
			{
				mov		edx, Clipper.nWidth
_DrawPartLineSection_SkipRight_LineLocal_:
				{
					KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
						or		ebx, ebx
						jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipRight_LineLocal_

						dec		Clipper.nHeight
						jz		_EXIT_WAY_

						add		edi, edx
						add		edi, edx
						neg		edx
				}

_DrawPartLineSection_SkipRight_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		eax, edx
							mov		edx, nSprSkipPerLine
							or		eax, eax
							jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_
_DrawPartLineSection_SkipRight_LineSkipLocal_:
						{
							KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_SkipRight_LineSkipLocal_CheckAlpha_:
							or		ebx, ebx
								jnz		_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_
								sub		edx, eax
								jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
								jmp		_DrawPartLineSection_SkipRight_Line_
_DrawPartLineSection_SkipRight_LineSkipLocal_Alpha_:
							{
								add		esi, eax
									sub		edx, eax
									jg		_DrawPartLineSection_SkipRight_LineSkipLocal_
									jmp		_DrawPartLineSection_SkipRight_Line_
							}
						}
					}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_:
					{
						sub		edx, eax
							jle		_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

							mov		ecx, eax
							cmp		ebx, 255
							jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_

							//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_:
						{
							mov     ebx, pPalette
_DrawPartLineSection_SkipRight_CopyPixel_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_SkipRight_CopyPixel_
							}
							jmp		_DrawPartLineSection_SkipRight_LineLocal_
						}

_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
							{
								push	edx
									shr		ebx, 3
									mov		nAlpha, ebx
_DrawPartLineSection_SkipRight_HalfAlphaPixel_:
								{
									KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
										loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_
								}
								pop		edx
									jmp		_DrawPartLineSection_SkipRight_LineLocal_
							}
					}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
					{
						add		eax, edx
							mov		ecx, eax
							cmp		ebx, 255
							jl		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_

							//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_Part_:
						{
							mov     ebx, pPalette
_DrawPartLineSection_SkipRight_CopyPixel_Part_:
							{
								KD_RIO_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_SkipRight_CopyPixel_Part_
							}
							neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								dec		Clipper.nHeight
								jg		_DrawPartLineSection_SkipRight_LineSkip_
								jmp		_EXIT_WAY_
						}

_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
							{
								push	edx
									shr		ebx, 3
									mov		nAlpha, ebx
_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_:
								{
									KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
										loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_
								}
								pop		edx
									neg		edx
									mov		ebx, 128
									dec		Clipper.nHeight
									jg		_DrawPartLineSection_SkipRight_LineSkip_//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
									jmp		_EXIT_WAY_
							}
					}
			}
_EXIT_WAY_:
	}
}

//功能：带alpha计算地复制spr图形到带透明模式的缓冲区
VOID RIO_CopySprToAlphaBufferAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	// pBuffer指向屏幕起点的偏移位置 (以字节计)
	INT nSprSkip = nSprWidth * Clipper.nTop + Clipper.nLeft;
	INT nSprSkipPerLine = Clipper.nLeft + Clipper.nRight;
	INT nBufSkip = nBufferWidth * Clipper.nY + Clipper.nX;
	INT nBufSkipPerLine = nBufferWidth - Clipper.nWidth;
	bool rgb565 = (dwRGBBitMask32 != 0x03e07c1f);
	WORD* dst = (WORD*)pBuffer;
	g_pPal = (WORD*)pPalette;
	g_pSection = (BYTE*)pSpr;
	g_nIndex = 0;

	//jmp
	_Jmp(nSprSkip);
	dst += nBufSkip;

	for( INT y = 0; y < Clipper.nHeight; y++ )
	{
		for( INT x = 0; x < Clipper.nWidth; x++ )
		{
			if( _Alpha() )	//read alpha
			{
				*dst = RIO_AlphaBlend( *dst, _Color(), _Alpha(), rgb565 );	//blend
			}

			//move to next pixel
			if( x < Clipper.nWidth - 1 )
			{
				_Jmp( 1 );
			}
			dst++;
		}

		if( y < Clipper.nHeight - 1 )
		{
			//move to next line
			_Jmp( nSprSkipPerLine + 1 );
			dst += nBufSkipPerLine;
		}
	}
	return;
}

//功能：三阶alpha形式地复制spr图形到缓冲区
VOID RIO_CopyBitmap16ToBuffer(LPVOID pBitmap, INT nBmpWidth, INT nBmpHeight, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nBmpWidth, nBmpHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;
	INT nBitmapOffset, nBuffNextLine;

	// 绘制函数的汇编代码
	__asm
	{
		//ebx = nPitch = nBufferWidth * 2
		mov		ebx, nBufferWidth
			add		ebx, ebx

			//使edi指向buffer绘制起点edi = (CHAR*)pBuffer + Clipper.y * nPitch + Clipper.x * 2;
			mov		edi, pBuffer
			mov		eax, Clipper.nY
			mul		ebx
			mov		edx, Clipper.nX
			add		eax, edx
			add		eax, edx
			add		edi, eax

			//esi指向图块数据绘制的开始位置
			mov		esi, pBitmap
			mov		eax, Clipper.nTop
			mul		nBmpWidth
			add		eax, Clipper.nLeft
			add		eax, eax
			add		esi, eax

			//计算缓冲区下一行的偏移=nPitch - Clipper.width * 2;
			mov		edx, Clipper.nWidth
			add		edx, edx
			sub		ebx, edx
			mov		nBuffNextLine, ebx

			// 计算位图下一行的偏移
			mov		eax, nBmpWidth
			sub		eax, Clipper.nWidth
			add		eax, eax

			mov		ebx, Clipper.nHeight
			mov		edx, Clipper.nWidth
			mov		ecx, edi
			sub		ecx, esi
			test	ecx, 2
			jz		_4BYTE_ALIGN_COPY_

			//_2BYTE_ALIGN_COPY_:
		{
_2BYTE_ALIGN_COPY_LINE_:
		{
			mov		ecx, edx
				rep		movsw
				add		edi, nBuffNextLine
				add     esi, eax
				dec		ebx
				jne		_2BYTE_ALIGN_COPY_LINE_
				jmp		_EXIT_WAY_
		}
		}

_4BYTE_ALIGN_COPY_:
		{
			mov		nBitmapOffset, eax
_4BYTE_ALIGN_COPY_LINE_:
			{
				mov		eax, edx
					mov		ecx, edi
					shr		ecx, 1
					and		ecx, 1
					sub		eax, ecx
					rep		movsw
					mov		ecx, eax
					shr		ecx, 1
					rep		movsd
					adc		ecx, ecx
					rep		movsw
					add		edi, nBuffNextLine
					add     esi, nBitmapOffset
					dec		ebx
					jne		_4BYTE_ALIGN_COPY_LINE_
					jmp		_EXIT_WAY_
			}
		}
_EXIT_WAY_:
	}
}

//功能：覆盖形式复制16位位图到缓冲区
VOID RIO_CopySprToBuffer3LevelAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;	
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	INT	nPitch = nBufferWidth + nBufferWidth;

	// pBuffer指向屏幕起点的偏移位置 (以字节计)
	pBuffer = (CHAR*)pBuffer + Clipper.nY * nPitch + Clipper.nX * 2;
	INT nBuffNextLine = nPitch - Clipper.nWidth * 2;// next line add
	INT nSprSkip = nSprWidth * Clipper.nTop + Clipper.nLeft;
	INT nSprSkipPerLine = Clipper.nLeft + Clipper.nRight;

	__asm
	{
		//使edi指向buffer绘制起点,使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
		mov		edi, pBuffer
			mov		esi, pSpr

			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
					or		ebx, ebx
					jnz		_SkipSpriteAheadContentLocalAlpha_
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					neg		edx
					jmp		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalAlpha_:
				{
					add		esi, eax
						sub		edx, eax
						jg		_SkipSpriteAheadContentLocalStart_
						add		esi, edx
						neg		edx
						jmp		_SkipSpriteAheadContentEnd_
				}
			}
		}
_SkipSpriteAheadContentEnd_:

		mov		eax, nSprSkipPerLine
			or		eax, eax
			jnz		_DrawPartLineSection_	//if (nSprSkipPerLine) goto _DrawPartLineSection_

			//_DrawFullLineSection_:
		{
			//因为sprite不会跨行压缩，则运行到此处edx必为0，如sprite会跨行压缩则_DrawFullLineSection_需改			
_DrawFullLineSection_Line_:
		{
			mov		edx, Clipper.nWidth
_DrawFullLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.nHeight
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					push	eax
						mov		ecx, eax

						cmp		ebx, 200
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
					{
						mov     ebx, pPalette
_DrawFullLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawFullLineSection_CopyPixel_
						}

						pop		eax
							sub		edx, eax
							jg		_DrawFullLineSection_LineLocal_

							add		edi, nBuffNextLine
							dec		Clipper.nHeight
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
					}

_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							push	edx							
_DrawFullLineSection_HalfAlphaPixel_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX
									loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							pop		edx
								pop		eax
								sub		edx, eax
								jg		_DrawFullLineSection_LineLocal_

								add		edi, nBuffNextLine
								dec		Clipper.nHeight
								jnz		_DrawFullLineSection_Line_
								jmp		_EXIT_WAY_
						}
				}
			}
		}
		}

_DrawPartLineSection_:
		{
_DrawPartLineSection_Line_:
		{
			mov		eax, edx
				mov		edx, Clipper.nWidth
				or		eax, eax
				jnz		_DrawPartLineSection_LineLocal_CheckAlpha_

_DrawPartLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineLocal_CheckAlpha_:
				or		ebx, ebx
					jnz		_DrawPartLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawPartLineSection_LineLocal_

					dec		Clipper.nHeight
					jz		_EXIT_WAY_

					add		edi, edx
					add		edi, edx
					neg		edx
			}

_DrawPartLineSection_LineSkip_:
				{
					add		edi, nBuffNextLine
						//跳过nSprSkipPerLine像素的sprite内容
						mov		eax, edx
						mov		edx, nSprSkipPerLine
						or		eax, eax
						jnz		_DrawPartLineSection_LineSkipLocal_CheckAlpha_

_DrawPartLineSection_LineSkipLocal_:
					{
						KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
						or		ebx, ebx
							jnz		_DrawPartLineSection_LineSkipLocal_Alpha_
							sub		edx, eax
							jg		_DrawPartLineSection_LineSkipLocal_
							neg		edx
							jmp		_DrawPartLineSection_Line_

_DrawPartLineSection_LineSkipLocal_Alpha_:
						{
							add		esi, eax
								sub		edx, eax
								jg		_DrawPartLineSection_LineSkipLocal_
								add		esi, edx
								neg		edx
								jmp		_DrawPartLineSection_Line_
						}
					}
				}

_DrawPartLineSection_LineLocal_Alpha_:
				{
					cmp		eax, edx
						jnl		_DrawPartLineSection_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

						push	eax
						mov		ecx, eax
						cmp		ebx, 200
						jl		_DrawPartLineSection_LineLocal_HalfAlpha_

						//_DrawPartLineSection_LineLocal_DirectCopy_:
					{
						mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_CopyPixel_
						}						
						pop		eax
							sub		edx, eax
							jmp		_DrawPartLineSection_LineLocal_
					}

_DrawPartLineSection_LineLocal_HalfAlpha_:
						{
							push	edx
_DrawPartLineSection_HalfAlphaPixel_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX
									loop	_DrawPartLineSection_HalfAlphaPixel_
							}
							pop		edx
								pop		eax
								sub		edx, eax
								jmp		_DrawPartLineSection_LineLocal_
						}
				}

_DrawPartLineSection_LineLocal_Alpha_Part_:
				{
					push	eax
						mov		ecx, edx
						cmp		ebx, 200
						jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_

						//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
					{
						mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_Part_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_CopyPixel_Part_
						}						
						pop		eax

							dec		Clipper.nHeight
							jz		_EXIT_WAY_

							sub		eax, edx
							mov		edx, eax
							mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
							jmp		_DrawPartLineSection_LineSkip_
					}

_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
						{
							push	edx
_DrawPartLineSection_HalfAlphaPixel_Part_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX
									loop	_DrawPartLineSection_HalfAlphaPixel_Part_
							}
							pop		edx
								pop		eax
								dec		Clipper.nHeight
								jz		_EXIT_WAY_
								sub		eax, edx
								mov		edx, eax
								mov		ebx, 128
								jmp		_DrawPartLineSection_LineSkip_//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
						}
				}
		}
		}
_EXIT_WAY_:
	}
}
//功能：覆盖形式复制16位位图到带透明模式的缓冲区
VOID RIO_CopySprToAlphaBuffer3LevelAlpha(LPVOID pSpr, INT nSprWidth, INT nSprHeight, LPVOID pPalette, LPVOID pBuffer, INT nBufferWidth, INT nBufferHeight, INT nX, INT nY, DWORD dwRGBBitMask32)
{
	// 对绘制区域进行裁剪
	KRClipperInfo Clipper;
	if (RIO_ClipCopyRect(nX, nY, nSprWidth, nSprHeight, nBufferWidth, nBufferHeight, &Clipper) == 0)
		return;

	INT	nPitch = nBufferWidth + nBufferWidth;

	// pBuffer指向屏幕起点的偏移位置 (以字节计)
	pBuffer = (CHAR*)pBuffer + Clipper.nY * nPitch + Clipper.nX * 2;
	INT nBuffNextLine = nPitch - Clipper.nWidth * 2;// next line add
	INT nSprSkip = nSprWidth * Clipper.nTop + Clipper.nLeft;
	INT nSprSkipPerLine = Clipper.nLeft + Clipper.nRight;

	__asm
	{
		//使edi指向buffer绘制起点,使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
		mov		edi, pBuffer
			mov		esi, pSpr

			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
					or		ebx, ebx
					jnz		_SkipSpriteAheadContentLocalAlpha_
					sub		edx, eax
					jg		_SkipSpriteAheadContentLocalStart_
					neg		edx
					jmp		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalAlpha_:
				{
					add		esi, eax
						sub		edx, eax
						jg		_SkipSpriteAheadContentLocalStart_
						add		esi, edx
						neg		edx
						jmp		_SkipSpriteAheadContentEnd_
				}
			}
		}
_SkipSpriteAheadContentEnd_:

		mov		eax, nSprSkipPerLine
			or		eax, eax
			jnz		_DrawPartLineSection_	//if (nSprSkipPerLine) goto _DrawPartLineSection_

			//_DrawFullLineSection_:
		{
			//因为sprite不会跨行压缩，则运行到此处edx必为0，如sprite会跨行压缩则_DrawFullLineSection_需改			
_DrawFullLineSection_Line_:
		{
			mov		edx, Clipper.nWidth
_DrawFullLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		Clipper.nHeight
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					push	eax
						mov		ecx, eax

						cmp		ebx, 200
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
					{
						mov     ebx, pPalette
_DrawFullLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX_1555
								loop	_DrawFullLineSection_CopyPixel_
						}

						pop		eax
							sub		edx, eax
							jg		_DrawFullLineSection_LineLocal_

							add		edi, nBuffNextLine
							dec		Clipper.nHeight
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
					}

_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							push	edx							
_DrawFullLineSection_HalfAlphaPixel_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX_1555
									loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							pop		edx
								pop		eax
								sub		edx, eax
								jg		_DrawFullLineSection_LineLocal_

								add		edi, nBuffNextLine
								dec		Clipper.nHeight
								jnz		_DrawFullLineSection_Line_
								jmp		_EXIT_WAY_
						}
				}
			}
		}
		}

_DrawPartLineSection_:
		{
_DrawPartLineSection_Line_:
		{
			mov		eax, edx
				mov		edx, Clipper.nWidth
				or		eax, eax
				jnz		_DrawPartLineSection_LineLocal_CheckAlpha_

_DrawPartLineSection_LineLocal_:
			{
				KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineLocal_CheckAlpha_:
				or		ebx, ebx
					jnz		_DrawPartLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawPartLineSection_LineLocal_

					dec		Clipper.nHeight
					jz		_EXIT_WAY_

					add		edi, edx
					add		edi, edx
					neg		edx
			}

_DrawPartLineSection_LineSkip_:
				{
					add		edi, nBuffNextLine
						//跳过nSprSkipPerLine像素的sprite内容
						mov		eax, edx
						mov		edx, nSprSkipPerLine
						or		eax, eax
						jnz		_DrawPartLineSection_LineSkipLocal_CheckAlpha_

_DrawPartLineSection_LineSkipLocal_:
					{
						KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineSkipLocal_CheckAlpha_:
						or		ebx, ebx
							jnz		_DrawPartLineSection_LineSkipLocal_Alpha_
							sub		edx, eax
							jg		_DrawPartLineSection_LineSkipLocal_
							neg		edx
							jmp		_DrawPartLineSection_Line_

_DrawPartLineSection_LineSkipLocal_Alpha_:
						{
							add		esi, eax
								sub		edx, eax
								jg		_DrawPartLineSection_LineSkipLocal_
								add		esi, edx
								neg		edx
								jmp		_DrawPartLineSection_Line_
						}
					}
				}

_DrawPartLineSection_LineLocal_Alpha_:
				{
					cmp		eax, edx
						jnl		_DrawPartLineSection_LineLocal_Alpha_Part_		//不能全画这eax个相同alpha值的像点，后面有点已经超出区域

						push	eax
						mov		ecx, eax
						cmp		ebx, 200
						jl		_DrawPartLineSection_LineLocal_HalfAlpha_

						//_DrawPartLineSection_LineLocal_DirectCopy_:
					{
						mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX_1555
								loop	_DrawPartLineSection_CopyPixel_
						}						
						pop		eax
							sub		edx, eax
							jmp		_DrawPartLineSection_LineLocal_
					}

_DrawPartLineSection_LineLocal_HalfAlpha_:
						{
							push	edx
_DrawPartLineSection_HalfAlphaPixel_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX_1555
									loop	_DrawPartLineSection_HalfAlphaPixel_
							}
							pop		edx
								pop		eax
								sub		edx, eax
								jmp		_DrawPartLineSection_LineLocal_
						}
				}

_DrawPartLineSection_LineLocal_Alpha_Part_:
				{
					push	eax
						mov		ecx, edx
						cmp		ebx, 200
						jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_

						//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
					{
						mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_Part_:
						{
							KD_RIO_COPY_PIXEL_USE_EAX_1555
								loop	_DrawPartLineSection_CopyPixel_Part_
						}						
						pop		eax

							dec		Clipper.nHeight
							jz		_EXIT_WAY_

							sub		eax, edx
							mov		edx, eax
							mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
							jmp		_DrawPartLineSection_LineSkip_
					}

_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
						{
							push	edx
_DrawPartLineSection_HalfAlphaPixel_Part_:
							{
								KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX_1555
									loop	_DrawPartLineSection_HalfAlphaPixel_Part_
							}
							pop		edx
								pop		eax
								dec		Clipper.nHeight
								jz		_EXIT_WAY_
								sub		eax, edx
								mov		edx, eax
								mov		ebx, 128
								jmp		_DrawPartLineSection_LineSkip_//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
						}
				}
		}
		}
_EXIT_WAY_:
	}
}

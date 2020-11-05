/* -------------------------------------------------------------------------
//	文件名		：	drawer.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	绘制器，封装DX的接口，提供文本和图片的绘制功能
//
// -----------------------------------------------------------------------*/
#ifndef __DRAWER_H__
#define __DRAWER_H__
#include "../represent_common/common.h"

extern DDSURFACEDESC  desc;

// 用于锁定渲染面
BOOL Lock( LPDIRECTDRAWSURFACE lpDDSCanvas );
// 用于解锁渲染面
void Unlock( LPDIRECTDRAWSURFACE lpDDSCanvas);
// 安装DirectDraw
BOOL SetupDirectDraw(HWND hWnd, INT nWidth, INT nHeight, BOOL bFullScreen, 
					 LPDIRECTDRAW* pOutDD, LPDIRECTDRAWSURFACE* pOutPrimarySurface, 
					 LPDIRECTDRAWSURFACE* pOutCanvas, LPDIRECTDRAWCLIPPER* pOutClipper, 
					 INT* pOutScreenWidth, INT* pOutScreenHeight, DWORD* pOutMask16, 
					 DWORD* pOutMask32, BOOL* pOutFakeFullscreen);

BOOL RectIntersect(const RECT* pRect1, const RECT* pRect2, RECT* pOut);

//font & text
void DrawCharWithBorder(INT nMask32, DWORD nColor, DWORD nBorderColor, 
						void* lpBuffer, INT nCanvasW, INT nCanvasH, 
						INT nPitch, INT nX, INT nY, 
						void* lpFont, INT nCharW, INT nCharH);

void DrawSysCharWithBorder(WORD wColor, WORD wBorderColor, 
						   void* pCanvas, INT nCanvasW, INT nCanvasH, 
						   INT nPitch, INT nX, INT nY, 
						   void* pCharData, INT nCharW, INT nCharH);

void DrawSysCharAntialiased(BOOL bRGB565, WORD wColor, void* pCanvas, INT nCanvasW, INT nCanvasH, 
							INT nPitch, INT nX, INT nY, void* pCharData, INT nCharW, INT nCharH);

//draw primitives
void DrawSpriteScreenBlendMMX( 
						 BYTE byInputAlpha, 
						 DWORD dwMask32, 
						 void* pBuffer, 
						 INT width, 
						 INT height, 
						 INT nPitch, 
						 INT nX, 
						 INT nY, 
						 void* pPalette, 
						 void* pSprite, 
						 INT nWidth, 
						 INT nHeight, 
						 const RECT* pSrcRect );

void DrawSpriteScreenBlend(BYTE byInputAlpha, DWORD dwMask32, void* pBuffer, 
						   INT width, INT height, INT nPitch, INT nX, INT nY, 
						   void* pPalette, void* pSprite, 
						   INT nWidth, INT nHeight, const RECT* pSrcRect);

void DrawSprite(void* pBuffer, INT width, INT height, INT nPitch, INT nX, INT nY, 
				void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* pSrcRect);

void DrawSpriteAlpha(INT nExAlpha, INT nMask32, void* pBuffer, 
					 INT width, INT height, INT nPitch, INT nX, INT nY, 
					 void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* pSrcRect);

void DrawSprite3LevelAlpha(INT nMask32, void* pBuffer, INT width, INT height, INT nPitch, INT nX, INT nY, 
						   void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* pSrcRect);

void DrawBitmap16(void* lpBuffer, INT width, INT height, INT nPitch, INT nX, INT nY, 
				  void* lpBitmap, INT nWidth, INT nHeight, const RECT* pSrcRect);

void DrawBitmap16AlphaMMX(
						  void* lpBuffer, 
						  INT width, 
						  INT height, 
						  INT nPitch, 
						  INT nX, 
						  INT nY, 
						  void* lpBitmap, 
						  INT nWidth, 
						  INT nHeight, 
						  const RECT* src_rect );

void DrawBitmap16Alpha(void* lpBuffer, INT width, INT height, INT nPitch, INT nX, INT nY, 
					   void* lpBitmap, INT nWidth, INT nHeight, const RECT* src_rect);

void DrawPixel(void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT nX, INT nY, INT nColor);

void DrawPixelAlpha(INT nMask32, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT nX, INT nY, INT nColor, INT nAlpha);

void DrawLine(void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT x1, INT y1, INT x2, INT y2, INT nColor);

void DrawLineAlpha(INT nMask32, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT x1, INT y1, INT x2, INT y2, INT nColor, INT nAlpha);

void DrawRectAlpha(INT nMask32, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT nX, INT nY, INT w, INT h, INT nColor, INT nAlpha);

#endif

/* -------------------------------------------------------------------------
//	文件名		：	drawer.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	绘制器，封装DX的接口，提供文本和图片的绘制功能
//
// -----------------------------------------------------------------------*/
#include "drawer.h"
#include "blender.h"
#include "render_helper.h"

WORD*	g_pPal = 0;
BYTE*	g_pSection = 0;
INT		g_nIndex = 0;
// 渲染SURFACE的实例
DDSURFACEDESC  desc;

// 用于锁定渲染SURFACE
BOOL Lock( LPDIRECTDRAWSURFACE lpDDSCanvas )
{
	if( desc.lpSurface == NULL || desc.lPitch == 0 )
	{
		desc.dwSize = sizeof( DDSURFACEDESC );
		if( FAILED( lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT , NULL ) ) )
		{
			return FALSE;
		}
		return TRUE;
	}
	return TRUE;

}

// 用于解锁渲染SURFACE
void Unlock( LPDIRECTDRAWSURFACE lpDDSCanvas)
{
	if( desc.lpSurface != NULL && desc.lPitch != 0 )
	{
		ZeroMemory( &desc, sizeof( DDSURFACEDESC ) );
		lpDDSCanvas->Unlock( NULL );
	}
}

// 安装DirectDraw
BOOL SetupDirectDraw(HWND hWnd, INT nWidth, INT nHeight, BOOL bFullScreen,
					 LPDIRECTDRAW* pOutDDraw, LPDIRECTDRAWSURFACE* pOutPrimarySurface,
					 LPDIRECTDRAWSURFACE* pOutCanvas, LPDIRECTDRAWCLIPPER* pOutClipper, 
					 INT* pOutScreenWidth, INT* pOutScreenHeight, DWORD* pOutMask16, 
					 DWORD* pOutMask32, BOOL* pOutFakeFullscreen)
{
	LPDIRECTDRAW		lpDD = 0;
	LPDIRECTDRAWSURFACE lpDDSPrimary = 0;
	LPDIRECTDRAWSURFACE lpDDSCanvas = 0;
	LPDIRECTDRAWCLIPPER lpDDClipper = 0;
	INT					nScreenWidth, nScreenHeight;
	DWORD				dwMask16, dwMask32;
	BOOL				bFakeFullscreen = FALSE;

	//create directdraw
	if( DirectDrawCreate( NULL, &lpDD, NULL ) != DD_OK )
		return FALSE;

	if( bFullScreen )
	{
		DDSURFACEDESC ddsd;
		ZeroMemory( &ddsd, sizeof(ddsd) );
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_ALL;
		lpDD->GetDisplayMode( &ddsd );

		if( ddsd.dwWidth == nWidth && ddsd.dwHeight == nHeight )
			bFakeFullscreen = TRUE;
	}

	//adjust window style
	RECT	rc = { 0, 0, nWidth, nHeight };

	if( bFullScreen )
	{
		if( bFakeFullscreen )
		{
			SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP );
			SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, nWidth, nHeight, SWP_NOACTIVATE );
		}
		else
			SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_POPUP );
	}
	else
	{
		SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX );
		AdjustWindowRectEx( &rc, GetWindowLong(hWnd, GWL_STYLE),	GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE) );
		SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE );

		RECT cNewRect;
		GetClientRect( hWnd, &cNewRect );
		if( nWidth != cNewRect.right - cNewRect.left || nHeight != cNewRect.bottom - cNewRect.top )
			return FALSE;
	}

	bFullScreen = bFullScreen && !bFakeFullscreen;

	//set coorperative level
	if( bFullScreen )
	{
		if( lpDD->SetCooperativeLevel( hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ) != DD_OK )
		{
			lpDD->Release();
			return FALSE;
		}
	}
	else
	{
		if( lpDD->SetCooperativeLevel( hWnd, DDSCL_NORMAL ) != DD_OK )
		{
			lpDD->Release();
			return FALSE;
		}
	}

	//set display mode
	if( bFullScreen )
	{
		if( lpDD->SetDisplayMode( nWidth, nHeight, 16 ) != DD_OK )
		{
			lpDD->Release();
			return FALSE;
		}
	}
	else
	{
		DDSURFACEDESC ddsd;
		ZeroMemory( &ddsd, sizeof(ddsd) );
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_ALL;
		lpDD->GetDisplayMode( &ddsd );
		if( ddsd.ddpfPixelFormat.dwRGBBitCount != 15 && ddsd.ddpfPixelFormat.dwRGBBitCount != 16 )
		{
			if( lpDD->SetDisplayMode( ddsd.dwWidth, ddsd.dwHeight, 16 ) != DD_OK )
			{
				lpDD->Release();
				return FALSE;
			}
		}
	}

	//get primary surface
	DDSURFACEDESC ddsd;
	memset( &ddsd, 0, sizeof(ddsd) );
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if( lpDD->CreateSurface( &ddsd, &lpDDSPrimary, NULL ) != DD_OK )
	{
		lpDD->Release();
		return FALSE;
	}

	//create & attatch clipper
	if( lpDD->CreateClipper( 0, &lpDDClipper, NULL ) != DD_OK )
	{
		lpDDSPrimary->Release();
		lpDD->Release();
		return FALSE;
	}

	lpDDClipper->SetHWnd( 0, hWnd );
	lpDDSPrimary->SetClipper( lpDDClipper );

	//create canvas
	memset( &ddsd, 0, sizeof(ddsd) );
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = nWidth;
	ddsd.dwHeight = nHeight;
	if( lpDD->CreateSurface( &ddsd, &lpDDSCanvas, NULL ) != DD_OK )
	{
		lpDDClipper->Release();
		lpDDSPrimary->Release();
		lpDD->Release();
		return FALSE;
	}

	//get screen size & mask16
	ZeroMemory( &ddsd, sizeof(ddsd) );
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_ALL;
	lpDD->GetDisplayMode( &ddsd );
	nScreenWidth = (INT)ddsd.dwWidth;
	nScreenHeight = (INT)ddsd.dwHeight;
	dwMask16 = ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask | ddsd.ddpfPixelFormat.dwBBitMask;
	dwMask32 = (dwMask16 == 0x7fff) ? 0x03e07c1f : 0x07e0f81f;

	//return results
	*pOutDDraw = lpDD;
	*pOutPrimarySurface = lpDDSPrimary;
	*pOutCanvas = lpDDSCanvas;
	*pOutScreenWidth = nScreenWidth;
	*pOutScreenHeight = nScreenHeight;
	*pOutMask16 = dwMask16;
	*pOutMask32 = dwMask32;
	*pOutFakeFullscreen = bFakeFullscreen;

	return TRUE;
}


BOOL RectIntersect(const RECT* rc1, const RECT* rc2, RECT* out)
{
	out->left = max( rc1->left, rc2->left );
	out->right = min( rc1->right, rc2->right );
	out->top = max( rc1->top, rc2->top );
	out->bottom = min( rc1->bottom, rc2->bottom );
	return (out->right > out->left) && (out->bottom > out->top);
}

//font & text
void DrawCharWithBorder(INT nMask32, DWORD nColor, DWORD nBorderColor, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT x, INT y, void* lpFont, INT char_w, INT char_h)
{
	if( !lpFont )
		return;

	RECT rc1 = {0, 0, canvas_w, canvas_h};
	RECT rc2 = {x, y, x + char_w, y + char_h};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x;
	INT clip_y = dest_rc.top - y;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;
	INT clip_right = char_w - clip_x - clip_width;

	x = dest_rc.left;
	y = dest_rc.top;

	INT nNextLine = nPitch - clip_width * 2;

	__asm
	{
		//---------------------------------------------------------------------------
		// 计算 EDI 指向屏幕起点的偏移量 (以字节计)
		// edi = nPitch * Clipper.y + nX * 2 + lpBuffer
		//---------------------------------------------------------------------------
		mov		eax, nPitch
			mov		ebx, y
			mul		ebx
			mov     ebx, x
			add		ebx, ebx
			add     eax, ebx
			mov 	edi, lpBuffer
			add		edi, eax
			//---------------------------------------------------------------------------
			// 初始化 ESI 指向图块数据起点 (跳过 Clipper.top 行图形数据)
			//---------------------------------------------------------------------------
			mov		esi, lpFont
			mov		ecx, clip_y
			or		ecx, ecx
			jz		loc_DrawFont_0011
			xor		eax, eax

loc_DrawFont_0008:

		mov		edx, char_w

loc_DrawFont_0009:

		mov     al, [esi]
		inc     esi
			and		al, 0x1f
			sub		edx, eax
			jg		loc_DrawFont_0009
			dec     ecx
			jg  	loc_DrawFont_0008
			//---------------------------------------------------------------------------
			// jump acorrd Clipper.left, Clipper.right
			//---------------------------------------------------------------------------
loc_DrawFont_0011:

		mov		eax, clip_x
			or		eax, eax
			jz		loc_DrawFont_0012
			jmp		loc_DrawFont_exit

loc_DrawFont_0012:

		mov		eax, clip_right
			or		eax, eax
			jz		loc_DrawFont_0100
			jmp		loc_DrawFont_exit
			//---------------------------------------------------------------------------
			// Clipper.left  == 0
			// Clipper.right == 0
			//---------------------------------------------------------------------------
loc_DrawFont_0100:

		mov		edx, clip_width

loc_DrawFont_0101:

		xor		eax, eax
			mov     al, [esi]
		inc     esi
			mov		ebx, eax
			shr		ebx, 5
			or		ebx, ebx
			jnz		loc_DrawFont_0102

			add		edi, eax
			add		edi, eax
			sub		edx, eax
			jg		loc_DrawFont_0101
			add		edi, nNextLine
			dec		clip_height
			jg		loc_DrawFont_0100
			jmp		loc_DrawFont_exit

loc_DrawFont_0102:
		and		eax, 0x1f
			mov		ecx, eax
			cmp		ebx, 7
			mov		ebx, ecx
			jl		DrawFrontWithBorder_DrawBorder

			//绘制字符点
			mov		eax, nColor
			rep		stosw

			sub		edx, ebx
			jg		loc_DrawFont_0101
			add		edi, nNextLine
			dec		clip_height
			jg		loc_DrawFont_0100
			jmp		loc_DrawFont_exit

DrawFrontWithBorder_DrawBorder:		//绘制字符的边缘
		mov		eax, nBorderColor
			rep		stosw

			sub		edx, ebx
			jg		loc_DrawFont_0101
			add		edi, nNextLine
			dec		clip_height
			jg		loc_DrawFont_0100
			jmp		loc_DrawFont_exit

loc_DrawFont_exit:
	}
}

void DrawSysCharWithBorder(WORD color, WORD border_color, void* canvas, INT canvas_w, INT canvas_h, INT pitch, INT x, INT y, void* char_data, INT char_w, INT char_h)
{
	if( !char_data )
		return;

	RECT rc1 = {0, 0, canvas_w, canvas_h};
	RECT rc2 = {x, y, x + char_w, y + char_h};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x;
	INT clip_y = dest_rc.top - y;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	BYTE* src = (BYTE*)char_data + clip_y * char_w + clip_x;
	WORD* dst = (WORD*)((BYTE*)canvas + y * pitch + x * 2);

	for( INT v = 0; v < clip_height; v++ )
	{
		for( INT u = 0; u < clip_width; u++ )
		{
			if( src[u] == 2 )	//body
			{
				dst[u] = color;
			}
			else if( src[u] == 1 )	//border
			{
				dst[u] = border_color;
			}
		}
		dst = (WORD*)((BYTE*)dst + pitch);
		src += char_w;
	}
}


void DrawSysCharAntialiased(BOOL rgb565, WORD color, void* canvas, INT canvas_w, INT canvas_h, INT pitch, INT x, INT y, void* char_data, INT char_w, INT char_h)
{
	if( !char_data )
		return;

	RECT rc1 = {0, 0, canvas_w, canvas_h};
	RECT rc2 = {x, y, x + char_w, y + char_h};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x;
	INT clip_y = dest_rc.top - y;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	BYTE* src = (BYTE*)char_data + clip_y * char_w + clip_x;
	WORD* dst = (WORD*)((BYTE*)canvas + y * pitch + x * 2);

	for( INT v = 0; v < clip_height; v++ )
	{
		for( INT u = 0; u < clip_width; u++ )
		{
			if( src[u] > 0 )
			{
				dst[u] = AlphaBlend( dst[u], color, src[u], rgb565 );
			}
		}
		dst = (WORD*)((BYTE*)dst + pitch);
		src += char_w;
	}
}

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
						 const RECT* pSrcRect )
{
	RECT sSrcRect;
	if( pSrcRect )
	{
		sSrcRect = *pSrcRect;
	}
	else
	{
		SetRect(&sSrcRect, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {nX, nY, nX + sSrcRect.right - sSrcRect.left, nY + sSrcRect.bottom - sSrcRect.top};

	RECT sDestRect;
	if( !RectIntersect( &rc1, &rc2, &sDestRect ) )
		return;

	INT nClipX = sDestRect.left - nX + sSrcRect.left;
	INT nClipY = sDestRect.top - nY + sSrcRect.top;
	INT nClipWidth = sDestRect.right - sDestRect.left;
	INT nClipHeight = sDestRect.bottom - sDestRect.top;

	nX = sDestRect.left;
	nY = sDestRect.top;

	INT nSprSkip = nWidth * nClipY + nClipX;
	INT nSprSkipPerLine = nWidth - nClipWidth;
	INT nClipRight = nWidth - nClipX - nClipWidth;
	INT nBufSkip = nPitch * nY + nX * 2;
	INT nBufSkipPerLine = nPitch - nClipWidth * 2;
	BOOL bRGB565 = (dwMask32 != 0x03e07c1f);
	WORD* pDst = (WORD*)pBuffer;
	g_pPal = (WORD*)pPalette;
	g_pSection = (BYTE*)pSprite;
	g_nIndex = 0;

	//jmp
	_Jmp(nSprSkip);
	pDst = (WORD*)((BYTE*)pDst + nBufSkip);


	INT nCmpValue = nClipWidth - 1;
	unsigned short src_color[4] = { 0 };
	unsigned short src_alpha[4] = { 0 };

	for( INT nY = 0; nY < nClipHeight; nY++ )
	{
		INT x = nClipWidth;
		while( x - 4 >= 0 )
		{
			if( _Alpha( ) )
			{
				src_color[ 0 ] = _Color( );
				src_alpha[ 0 ] = byInputAlpha + 1 ;
			}
			else
			{
				src_color[ 0 ] = 0;
				src_alpha[ 0 ] = 0;
			}
			_Jmp( 1 );


			if( _Alpha( ) )
			{
				src_color[ 1 ] = _Color( );
				src_alpha[ 1 ] = byInputAlpha + 1 ;
			}
			else
			{
				src_color[ 1 ] = 0;
				src_alpha[ 1 ] = 0;
			}
			_Jmp( 1 );

			if( _Alpha( ) )
			{
				src_color[ 2 ] = _Color( );
				src_alpha[ 2 ] = byInputAlpha + 1 ;
			}
			else
			{
				src_color[ 2 ] = 0;
				src_alpha[ 2 ] = 0;
			}
			_Jmp( 1 );

			if( _Alpha( ) )
			{
				src_color[ 3 ] = _Color( );
				src_alpha[ 3 ] = byInputAlpha + 1 ;
			}
			else
			{
				src_color[ 3 ] = 0;
				src_alpha[ 3 ] = 0;
			}
			if( x   - 4 > 0 )
			{
				_Jmp( 1 );
			}
			x -= 4;
			ScreenBlend4Pixel( pDst, src_color, src_alpha );
			pDst += 4;
		}
		if( x - 2 >= 0 )
		{
			if( _Alpha( ) )
			{
				src_color[ 0 ] = _Color( );
				src_alpha[ 0 ] = byInputAlpha;
			}
			else
			{
				src_color[ 0 ] = 0;
				src_alpha[ 0 ] = 0;
			}


			_Jmp( 1 );

			if( _Alpha( ) )
			{
				src_color[ 1 ] = _Color( );
				src_alpha[ 1 ] = byInputAlpha;
			}
			else
			{
				src_color[ 1 ] = 0;
				src_alpha[ 1 ] = 0;

			}
			if( x - 2  > 0)
				_Jmp( 1 );

			//			ScreenBlend2Pixel( dst, src_color, src_alpha );
			x -= 2;
			pDst += 2;
		}
		if( x - 1 >= 0 )
		{
			if( _Alpha( ) )
			{
				src_color[ 0 ] = _Color( );
				src_alpha[ 0 ] = byInputAlpha;
			}
			//			ScreenBlend1Pixel( dst, src_color, src_alpha );
			++pDst;
			x -= 1;
		}
		if( nY < nClipHeight - 1 )
		{
			//move to next line
			_Jmp( nSprSkipPerLine + 1 );
			pDst = (WORD*)((BYTE*)pDst + nBufSkipPerLine);
		}
	}
}

void DrawSpriteScreenBlend(BYTE byInputAlpha, DWORD dwMask32, void* pBuffer, INT width, INT height, INT nPitch, INT nX, INT nY, void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* pSrcRect)
{
	RECT sSrcRect;
	if( pSrcRect )
	{
		sSrcRect = *pSrcRect;
	}
	else
	{
		SetRect(&sSrcRect, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {nX, nY, nX + sSrcRect.right - sSrcRect.left, nY + sSrcRect.bottom - sSrcRect.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - nX + sSrcRect.left;
	INT clip_y = dest_rc.top - nY + sSrcRect.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	nX = dest_rc.left;
	nY = dest_rc.top;

	INT nSprSkip = nWidth * clip_y + clip_x;
	INT nSprSkipPerLine = nWidth - clip_width;
	INT clip_right = nWidth - clip_x - clip_width;
	INT nBufSkip = nPitch * nY + nX * 2;
	INT nBufSkipPerLine = nPitch - clip_width * 2;
	BOOL rgb565 = (dwMask32 != 0x03e07c1f);
	WORD* dst = (WORD*)pBuffer;
	g_pPal = (WORD*)pPalette;
	g_pSection = (BYTE*)pSprite;
	g_nIndex = 0;

	//jmp
	_Jmp(nSprSkip);
	dst = (WORD*)((BYTE*)dst + nBufSkip);

	for( INT nY = 0; nY < clip_height; nY++ )
	{
		for( INT x = 0; x < clip_width; x++ )
		{
			if( _Alpha() )	//read alpha
			{
				*dst = ScreenBlend( *dst, _Color(), byInputAlpha, rgb565 );	//blend
			}
			else
			{
				*dst = ScreenBlend( *dst, 0, 0, rgb565 );
			}
			//move to next pixel
			if( x < clip_width - 1 )
			{
				_Jmp( 1 );
			}
			dst++;
		}

		if( nY < clip_height - 1 )
		{
			//move to next line
			_Jmp( nSprSkipPerLine + 1 );
			dst = (WORD*)((BYTE*)dst + nBufSkipPerLine);
		}
	}
	return;
}

void DrawSprite(void* pBuffer, INT width, INT height, INT nPitch, INT x, INT y, void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* src_rect)
{
	RECT _src_rc;
	if( src_rect )
	{
		_src_rc = *src_rect;
	}
	else
	{
		SetRect(&_src_rc, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {x, y, x + _src_rc.right - _src_rc.left, y + _src_rc.bottom - _src_rc.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x + _src_rc.left;
	INT clip_y = dest_rc.top - y + _src_rc.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	pBuffer = (CHAR*)(pBuffer) + y * nPitch;
	INT nBuffNextLine = nPitch - clip_width * 2;// next line add
	INT nSprSkip = nWidth * clip_y + clip_x;
	INT nSprSkipPerLine = nWidth - clip_width;
	INT clip_right = nWidth - clip_x - clip_width;

	__asm
	{
		//使edi指向buffer绘制起点,	(以字节计)	
		mov		edi, pBuffer
			mov		eax, x
			add		edi, eax
			add		edi, eax

			//使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
			mov		esi, pSprite
			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
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
			mov		edx, clip_width
_DrawFullLineSection_LineLocal_:
			{
				KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		clip_height
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					sub		edx, eax
						mov		ecx, eax
						mov     ebx, pPalette
_DrawFullLineSection_CopyPixel_:
					{
						KD_COPY_PIXEL_USE_EAX
							loop	_DrawFullLineSection_CopyPixel_
					}
					or		edx, edx
						jnz		_DrawFullLineSection_LineLocal_

						add		edi, nBuffNextLine
						dec		clip_height
						jnz		_DrawFullLineSection_Line_
						jmp		_EXIT_WAY_
				}
			}
		}
		}

_DrawPartLineSection_:
		{
			mov		eax, clip_x
				or		eax, eax
				jz		_DrawPartLineSection_SkipRight_Line_

				mov		eax, clip_right
				or		eax, eax
				jz		_DrawPartLineSection_SkipLeft_Line_
		}

_DrawPartLineSection_Line_:
			{
				mov		eax, edx
					mov		edx, clip_width
					or		eax, eax
					jnz		_DrawPartLineSection_LineLocal_CheckAlpha_
_DrawPartLineSection_LineLocal_:
				{
					KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_LineLocal_

						dec		clip_height
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
							KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

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
							KD_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_CopyPixel_
						}
						jmp		_DrawPartLineSection_LineLocal_
					}
_DrawPartLineSection_LineLocal_Alpha_Part_:
						{
							add		eax, edx
								mov		ecx, eax
								mov     ebx, pPalette
_DrawPartLineSection_CopyPixel_Part_:
							{
								KD_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_CopyPixel_Part_
							}

							dec		clip_height
								jz		_EXIT_WAY_
								neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								jmp		_DrawPartLineSection_LineSkip_
						}
			}

_DrawPartLineSection_SkipLeft_Line_:
			{
				mov		eax, edx
					mov		edx, clip_width
					or		eax, eax
					jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
_DrawPartLineSection_SkipLeft_LineLocal_:
				{
					KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
					or		ebx, ebx
						jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipLeft_LineLocal_

						dec		clip_height
						jz		_EXIT_WAY_
				}

_DrawPartLineSection_SkipLeft_LineSkip_:
					{
						add		edi, nBuffNextLine
							//跳过nSprSkipPerLine像素的sprite内容
							mov		edx, nSprSkipPerLine
_DrawPartLineSection_SkipLeft_LineSkipLocal_:
						{
							KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
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
							KD_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_SkipLeft_CopyPixel_
						}
						or		edx, edx
							jnz		_DrawPartLineSection_SkipLeft_LineLocal_
							dec		clip_height
							jg		_DrawPartLineSection_SkipLeft_LineSkip_
							jmp		_EXIT_WAY_
					}
			}

_DrawPartLineSection_SkipRight_Line_:
			{
				mov		edx, clip_width
_DrawPartLineSection_SkipRight_LineLocal_:
				{
					KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
						or		ebx, ebx
						jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
						add		edi, eax
						add		edi, eax
						sub		edx, eax
						jg		_DrawPartLineSection_SkipRight_LineLocal_

						dec		clip_height
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
							KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

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
							KD_COPY_PIXEL_USE_EAX
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
								KD_COPY_PIXEL_USE_EAX
									loop	_DrawPartLineSection_SkipRight_CopyPixel_Part_
							}
							neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								dec		clip_height
								jg		_DrawPartLineSection_SkipRight_LineSkip_
								jmp		_EXIT_WAY_
						}
			}
_EXIT_WAY_:
	}
}

void DrawSpriteAlpha(INT nExAlpha, INT nMask32, void* pBuffer, INT width, INT height, INT nPitch, INT x, INT y, void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* src_rect)
{
	if( nExAlpha == 0 )
		return;

	RECT _src_rc;
	if( src_rect )
	{
		_src_rc = *src_rect;
	}
	else
	{
		SetRect(&_src_rc, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {x, y, x + _src_rc.right - _src_rc.left, y + _src_rc.bottom - _src_rc.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x + _src_rc.left;
	INT clip_y = dest_rc.top - y + _src_rc.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	if( nExAlpha >= 31 )
	{
		pBuffer = (CHAR*)(pBuffer) + y * nPitch;
		INT nBuffNextLine = nPitch - clip_width * 2;// next line add
		INT nSprSkip = nWidth * clip_y + clip_x;
		INT nSprSkipPerLine = nWidth - clip_width;
		INT clip_right = nWidth - clip_x - clip_width;
		INT	 nAlpha;

		__asm
		{
			mov     eax, pPalette
				movd    mm0, eax        // mm0: pPalette

				mov     eax, clip_width
				movd    mm1, eax        // mm1: Clipper.width

				mov     eax, nMask32
				movd    mm2, eax        // mm2: nMask32

				// mm3: nAlpha

				// mm4: temp use

				// mm7: push ecx, pop ecx
				// mm6: push edx, pop edx
				// mm5: push eax, pop eax


				//使edi指向buffer绘制起点,	(以字节计)	
				mov		edi, pBuffer
				mov		eax, x
				add		edi, eax
				add		edi, eax


				//使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
				mov		esi, pSprite

				//_SkipSpriteAheadContent_:
			{
				mov		edx, nSprSkip
					or		edx, edx
					jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
				{
					KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
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
				movd	edx, mm1    // mm1: Clipper.width
_DrawFullLineSection_LineLocal_:
				{
					KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

						or		ebx, ebx
						jnz     ignore_alpha_FullLineSection_LineLocal_Alpha
						//						jnz		_DrawFullLineSection_LineLocal_Alpha_
						lea     edi, [edi + eax * 2]
					sub		edx, eax
						jg		_DrawFullLineSection_LineLocal_

						add		edi, nBuffNextLine
						dec		clip_height
						jnz		_DrawFullLineSection_Line_
						jmp		_EXIT_WAY_
ignore_alpha_FullLineSection_LineLocal_Alpha:
					cmp    ebx,KD_IGNORE_ALPHA
						jg      _DrawFullLineSection_LineLocal_Alpha_
						lea     edi, [edi + eax * 2]
					add     esi, eax
						sub		edx, eax
						jg		_DrawFullLineSection_LineLocal_

						add		edi, nBuffNextLine
						dec		clip_height
						jnz		_DrawFullLineSection_Line_
						jmp		_EXIT_WAY_



_DrawFullLineSection_LineLocal_Alpha_:
					{
						sub		edx, eax
							mov		ecx, eax

							cmp		ebx, KD_ADD_ALPHA
							jl		_DrawFullLineSection_LineLocal_HalfAlpha_

							//_DrawFullLineSection_LineLocal_DirectCopy_:
						{
							movd     ebx, mm0   // mm0: pPalette

								sub ecx, 4
								jl  _DrawFullLineSection_CopyPixel_continue
_DrawFullLineSection_CopyPixel4_:
							{
								KD_COPY_4PIXEL_USE_EAX

									sub ecx, 4
									jg     _DrawFullLineSection_CopyPixel4_
							}
_DrawFullLineSection_CopyPixel_continue:
							add ecx, 4
								jz _DrawFullLineSection_CopyPixel_End 

_DrawFullLineSection_CopyPixel_:
							{
								KD_COPY_PIXEL_USE_EAX
									dec     ecx
									jnz     _DrawFullLineSection_CopyPixel_
							}
_DrawFullLineSection_CopyPixel_End:

							or		edx, edx
								jnz		_DrawFullLineSection_LineLocal_

								add		edi, nBuffNextLine
								dec		clip_height
								jnz		_DrawFullLineSection_Line_
								jmp		_EXIT_WAY_
						}

_DrawFullLineSection_LineLocal_HalfAlpha_:
							{
								movd    mm6, edx
									shr		ebx, 3
									movd    mm3, ebx    // mm3: nAlpha
_DrawFullLineSection_HalfAlphaPixel_:
								{
									KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
										loop	_DrawFullLineSection_HalfAlphaPixel_
								}
								movd    edx, mm6
									or		edx, edx
									jnz		_DrawFullLineSection_LineLocal_

									add		edi, nBuffNextLine
									dec		clip_height
									jnz		_DrawFullLineSection_Line_
									jmp		_EXIT_WAY_
							}
					}
				}
			}
			}

_DrawPartLineSection_:
			{
				mov		eax, clip_x
					or		eax, eax
					jz		_DrawPartLineSection_SkipRight_Line_

					mov		eax, clip_right
					or		eax, eax
					jz		_DrawPartLineSection_SkipLeft_Line_
			}

_DrawPartLineSection_Line_:
				{
					mov		eax, edx
						movd	edx, mm1    // mm1: Clipper.width
						or		eax, eax
						jnz		_DrawPartLineSection_LineLocal_CheckAlpha_
_DrawPartLineSection_LineLocal_:
					{
						KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_LineLocal_CheckAlpha_:
						or		ebx, ebx
							jnz		_DrawPartLineSection_LineLocal_Alpha_
							add		edi, eax
							add		edi, eax
							sub		edx, eax
							jg		_DrawPartLineSection_LineLocal_

							dec		clip_height
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
								KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

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
								cmp		ebx, KD_ADD_ALPHA
								jge		Full_Copy_PartLine
								cmp     ebx, KD_IGNORE_ALPHA
								jg      _DrawPartLineSection_LineLocal_HalfAlpha_
								add     edi, eax
								add     edi, eax
								add     esi, eax

								jmp _DrawPartLineSection_LineLocal_
Full_Copy_PartLine:

							//_DrawPartLineSection_LineLocal_DirectCopy_:
							{
								movd     ebx, mm0 // mm0: pPalette
_DrawPartLineSection_CopyPixel_:
								{
									KD_COPY_PIXEL_USE_EAX
										loop	_DrawPartLineSection_CopyPixel_
								}
								jmp		_DrawPartLineSection_LineLocal_
							}

_DrawPartLineSection_LineLocal_HalfAlpha_:
								{
									movd    mm6, edx
										shr		ebx, 3
										movd    mm3, ebx    // mm3: nAlpha
_DrawPartLineSection_HalfAlphaPixel_:
									{
										KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
											loop	_DrawPartLineSection_HalfAlphaPixel_
									}
									movd    edx, mm6
										jmp		_DrawPartLineSection_LineLocal_
								}
						}
_DrawPartLineSection_LineLocal_Alpha_Part_:
						{
							add		eax, edx
								mov		ecx, eax
								cmp		ebx, KD_ADD_ALPHA

								jge     Full_Copy_PartLine_Line_Part
								cmp     ebx, KD_IGNORE_ALPHA
								jg      _DrawPartLineSection_LineLocal_HalfAlpha_Part_
								add     edi,eax
								add     edi,eax
								add     esi,eax

								neg		edx
								dec		clip_height
								jg		_DrawPartLineSection_LineSkip_
								jmp		_EXIT_WAY_


Full_Copy_PartLine_Line_Part:
							//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
							{
								movd    ebx,  mm0   // mm0: pPalette
_DrawPartLineSection_CopyPixel_Part_:
								{
									KD_COPY_PIXEL_USE_EAX
										loop	_DrawPartLineSection_CopyPixel_Part_
								}

								dec		clip_height
									jz		_EXIT_WAY_
									neg		edx
									mov		ebx, 255
									jmp		_DrawPartLineSection_LineSkip_
							}

_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
								{
									movd    mm6, edx
										shr		ebx, 3
										movd    mm3, ebx    // mm3: nAlpha
_DrawPartLineSection_HalfAlphaPixel_Part_:
									{
										KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
											loop	_DrawPartLineSection_HalfAlphaPixel_Part_
									}
									movd    edx, mm6
										neg		edx
										mov		ebx, nAlpha
										shl		ebx, 3			//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
										add		ebx, 1
										dec		clip_height
										jg		_DrawPartLineSection_LineSkip_
										jmp		_EXIT_WAY_
								}
						}
				}

_DrawPartLineSection_SkipLeft_Line_:
				{
					mov		eax, edx
						movd	edx, mm1    // mm1: Clipper.width
						or		eax, eax
						jnz		_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_
_DrawPartLineSection_SkipLeft_LineLocal_:
					{
						KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
_DrawPartLineSection_SkipLeft_LineLocal_CheckAlpha_:
						or		ebx, ebx
							jnz		_DrawPartLineSection_SkipLeft_LineLocal_Alpha_
							add		edi, eax
							add		edi, eax
							sub		edx, eax
							jg		_DrawPartLineSection_SkipLeft_LineLocal_

							dec		clip_height
							jz		_EXIT_WAY_
					}

_DrawPartLineSection_SkipLeft_LineSkip_:
						{
							add		edi, nBuffNextLine
								//跳过nSprSkipPerLine像素的sprite内容
								mov		edx, nSprSkipPerLine
_DrawPartLineSection_SkipLeft_LineSkipLocal_:
							{
								KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
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
								cmp		ebx, KD_ADD_ALPHA
								jl		_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_

								//_DrawPartLineSection_SkipLeft_LineLocal_DirectCopy_:
							{
								movd    ebx, mm0    // mm0: pPalette
_DrawPartLineSection_SkipLeft_CopyPixel_:
								{
									KD_COPY_PIXEL_USE_EAX
										loop	_DrawPartLineSection_SkipLeft_CopyPixel_
								}
								or		edx, edx
									jnz		_DrawPartLineSection_SkipLeft_LineLocal_
									dec		clip_height
									jg		_DrawPartLineSection_SkipLeft_LineSkip_
									jmp		_EXIT_WAY_
							}

_DrawPartLineSection_SkipLeft_LineLocal_nAlpha_:
								{
									movd    mm6, edx
										shr		ebx, 3
										movd    mm3, ebx    // mm3: nAlpha
_DrawPartLineSection_SkipLeft_HalfAlphaPixel_:
									{
										KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
											loop	_DrawPartLineSection_SkipLeft_HalfAlphaPixel_
									}
									movd    edx, mm6
										or		edx, edx
										jnz		_DrawPartLineSection_SkipLeft_LineLocal_
										dec		clip_height
										jg		_DrawPartLineSection_SkipLeft_LineSkip_
										jmp		_EXIT_WAY_
								}
						}
				}

_DrawPartLineSection_SkipRight_Line_:
				{
					movd	edx, mm1    // mm1: Clipper.width
_DrawPartLineSection_SkipRight_LineLocal_:
					{
						KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
							or		ebx, ebx
							jnz		_DrawPartLineSection_SkipRight_LineLocal_Alpha_
							add		edi, eax
							add		edi, eax
							sub		edx, eax
							jg		_DrawPartLineSection_SkipRight_LineLocal_

							dec		clip_height
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
								KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

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
								cmp		ebx, KD_ADD_ALPHA

								jge		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Copy_Full_Pixel
								cmp     ebx, KD_IGNORE_ALPHA
								jg      _DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_
								add     edi, eax
								add     edi,eax
								add     esi,eax
								jmp		_DrawPartLineSection_SkipRight_LineLocal_
_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Copy_Full_Pixel:
							//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_:
							{
								movd    ebx, mm0    // mm0: pPalette
_DrawPartLineSection_SkipRight_CopyPixel_:
								{
									KD_COPY_PIXEL_USE_EAX
										loop	_DrawPartLineSection_SkipRight_CopyPixel_
								}
								jmp		_DrawPartLineSection_SkipRight_LineLocal_
							}

_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_:
								{
									movd    mm6, edx
										shr		ebx, 3
										movd    mm3, ebx    // mm3: nAlpha
_DrawPartLineSection_SkipRight_HalfAlphaPixel_:
									{
										KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
											loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_
									}
									movd	edx, mm6
										jmp		_DrawPartLineSection_SkipRight_LineLocal_
								}
						}
_DrawPartLineSection_SkipRight_LineLocal_Alpha_Part_:
						{
							add		eax, edx
								mov		ecx, eax
								cmp		ebx, KD_ADD_ALPHA
								jge		_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_Full_Pixel_CPY
								cmp    ebx, KD_IGNORE_ALPHA
								jg     _DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_
								add    edi,eax
								add     edi,eax
								add    esi, eax
								neg		edx
								mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								dec		clip_height
								jg		_DrawPartLineSection_SkipRight_LineSkip_
								jmp		_EXIT_WAY_
_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_Full_Pixel_CPY:
							//_DrawPartLineSection_SkipRight_LineLocal_DirectCopy_Part_:
							{
								movd    ebx, mm0 // mm0: pPalette
_DrawPartLineSection_SkipRight_CopyPixel_Part_:
								{
									KD_COPY_PIXEL_USE_EAX
										loop	_DrawPartLineSection_SkipRight_CopyPixel_Part_
								}
								neg		edx
									mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
									dec		clip_height
									jg		_DrawPartLineSection_SkipRight_LineSkip_
									jmp		_EXIT_WAY_
							}

_DrawPartLineSection_SkipRight_LineLocal_HalfAlpha_Part_:
								{
									movd    mm6, edx
										shr		ebx, 3
										movd    mm3, ebx    // mm3: nAlpha
_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_:
									{
										KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX
											loop	_DrawPartLineSection_SkipRight_HalfAlphaPixel_Part_
									}
									movd	edx, mm6
										neg		edx
										mov		ebx, 128
										dec		clip_height
										jg		_DrawPartLineSection_SkipRight_LineSkip_//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
										jmp		_EXIT_WAY_
								}
						}
				}
_EXIT_WAY_:
				emms
		}
	}
	else
	{
		WORD wAlpha = (WORD)nExAlpha;
		INT nNextLine = nPitch - nWidth * 2;// next line add
		INT clip_right = nWidth - clip_x - clip_width;
		INT nAlpha;

		// 绘制函数的汇编代码
		__asm
		{
			//---------------------------------------------------------------------------
			// 计算 EDI 指向屏幕起点的偏移量 (以字节计)
			// edi = pBuffer + nPitch * y + nX * 2;
			//---------------------------------------------------------------------------
			mov		eax, nPitch
				mov		ebx, y
				mul		ebx
				mov     ebx, x
				add		ebx, ebx
				add		eax, ebx
				mov		edi, pBuffer
				add		edi, eax
				//---------------------------------------------------------------------------
				// 初始化 ESI 指向图块数据起点 
				// (跳过clip_y行压缩图形数据)
				//---------------------------------------------------------------------------
				mov		esi, pSprite
				mov		ecx, clip_y
				or		ecx, ecx
				jz		loc_DrawSpriteAlpha_0011

loc_DrawSpriteAlpha_0008:

			mov		edx, nWidth

loc_DrawSpriteAlpha_0009:

			//		movzx	eax, BYTE ptr[esi]
			//		inc		esi
			//		movzx	ebx, BYTE ptr[esi]
			//		inc		esi
			//		use uv, change to below
			xor		eax, eax
				xor		ebx, ebx
				mov		al,	 BYTE ptr[esi]
			inc		esi
				mov		bl,  BYTE ptr[esi]
			inc		esi
				//		change	end
				or		ebx, ebx
				jnz		loc_DrawSpriteAlpha_0010
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0009
				dec     ecx
				jnz		loc_DrawSpriteAlpha_0008
				jmp		loc_DrawSpriteAlpha_0011

loc_DrawSpriteAlpha_0010:

			add		esi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0009
				dec     ecx
				jnz		loc_DrawSpriteAlpha_0008
				//---------------------------------------------------------------------------
				// 根据 clip_x, clip_right 分 4 种情况
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0011:

			mov		eax, clip_x
				or		eax, eax
				jnz		loc_DrawSpriteAlpha_0012
				mov		eax, clip_right
				or		eax, eax
				jnz		loc_DrawSpriteAlpha_0013
				jmp		loc_DrawSpriteAlpha_0100

loc_DrawSpriteAlpha_0012:

			mov		eax, clip_right
				or		eax, eax
				jnz		loc_DrawSpriteAlpha_0014
				jmp		loc_DrawSpriteAlpha_0200

loc_DrawSpriteAlpha_0013:

			jmp		loc_DrawSpriteAlpha_0300

loc_DrawSpriteAlpha_0014:

			jmp		loc_DrawSpriteAlpha_0400
				//---------------------------------------------------------------------------
				// 左边界裁剪量 == 0
				// 右边界裁剪量 == 0
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0100:

			mov		edx, clip_width

loc_DrawSpriteAlpha_0101:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				movzx	ebx, BYTE ptr[esi]
			inc		esi
				or		ebx, ebx
				jnz		loc_DrawSpriteAlpha_0102

				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0101
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0100
				jmp		loc_DrawSpriteAlpha_exit

loc_DrawSpriteAlpha_0102:
			push	eax
				push	edx
				mov		ax, wAlpha
				mul		bx
				shr		eax, 5
				mov		ebx, eax
				pop		edx
				pop		eax		
				jg		loc_lgzone
				mov		ebx, 0
loc_lgzone:
			cmp		ebx, 255
				jl		loc_DrawSpriteAlpha_0110
				push	eax
				push	edx
				mov		ecx, eax
				mov     ebx, pPalette

loc_DrawSpriteAlpha_0103:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov		dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec		ecx
				jnz		loc_DrawSpriteAlpha_0103

				pop		edx
				pop		eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0101
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0100
				jmp		loc_DrawSpriteAlpha_exit

loc_DrawSpriteAlpha_0110:

			push	eax
				push	edx
				mov		ecx, eax
				shr     ebx, 3
				mov		nAlpha, ebx

loc_DrawSpriteAlpha_0111:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_DrawSpriteAlpha_0111

				pop		edx
				pop		eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0101
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0100
				jmp		loc_DrawSpriteAlpha_exit

				//---------------------------------------------------------------------------
				// 左边界裁剪量 != 0
				// 右边界裁剪量 == 0
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0200:

			mov		edx, clip_x

loc_DrawSpriteAlpha_0201:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				movzx	ebx, BYTE ptr[esi]
			inc		esi
				or		ebx, ebx
				jnz		loc_DrawSpriteAlpha_0202
				//---------------------------------------------------------------------------
				// 处理nAlpha == 0 的像素 (左边界外)
				//---------------------------------------------------------------------------
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0201
				jz		loc_DrawSpriteAlpha_0203
				neg		edx
				mov		eax, edx
				mov		edx, clip_width
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0204
				add		edi, nNextLine
				dec		clip_height
				jg		loc_DrawSpriteAlpha_0200
				jmp		loc_DrawSpriteAlpha_exit
				//---------------------------------------------------------------------------
				// 处理nAlpha != 0 的像素 (左边界外)
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0202:

			add		esi, eax
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0201
				jz		loc_DrawSpriteAlpha_0203
				//---------------------------------------------------------------------------
				// 把多减的宽度补回来
				//---------------------------------------------------------------------------
				neg		edx
				sub		esi, edx
				sub		edi, edx
				sub		edi, edx

				cmp		ebx, 255
				jl		loc_DrawSpriteAlpha_0210

				push	eax
				push	edx
				mov		ecx, edx
				mov     ebx, pPalette

loc_DrawSpriteAlpha_Loop20:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec     ecx
				jg      loc_DrawSpriteAlpha_Loop20

				pop		edx
				pop		eax
				mov		ecx, edx
				mov		edx, clip_width
				sub		edx, ecx
				jg		loc_DrawSpriteAlpha_0204
				add		edi, nNextLine
				dec		clip_height
				jg		loc_DrawSpriteAlpha_0200
				jmp		loc_DrawSpriteAlpha_exit

loc_DrawSpriteAlpha_0210:

			push	eax
				push	edx
				mov		ecx, edx
				shr     ebx, 3
				mov		nAlpha, ebx

loc_DrawSpriteAlpha_0211:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_DrawSpriteAlpha_0211

				pop		edx
				pop		eax
				mov		ecx, edx
				mov		edx, clip_width
				sub		edx, ecx
				jg		loc_DrawSpriteAlpha_0204
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0200
				jmp		loc_DrawSpriteAlpha_exit

				//---------------------------------------------------------------------------
				// 已处理完剪裁区 下面的处理相对简单
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0203:

			mov		edx, clip_width

loc_DrawSpriteAlpha_0204:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				movzx	ebx, BYTE ptr[esi]
			inc		esi
				or		ebx, ebx
				jnz		loc_DrawSpriteAlpha_0206
				//---------------------------------------------------------------------------
				// 处理nAlpha == 0的像素 (左边界内)
				//---------------------------------------------------------------------------
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0204
				add		edi, nNextLine
				dec		clip_height
				jg		loc_DrawSpriteAlpha_0200
				jmp		loc_DrawSpriteAlpha_exit
				//---------------------------------------------------------------------------
				// 处理nAlpha != 0的像素 (左边界内)
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0206:

			cmp		ebx, 255
				jl		loc_DrawSpriteAlpha_0220

				push	eax
				push	edx
				mov		ecx, eax
				mov     ebx, pPalette

loc_DrawSpriteAlpha_Loop21:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec     ecx
				jg		loc_DrawSpriteAlpha_Loop21

				pop		edx
				pop		eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0204
				add		edi, nNextLine
				dec		clip_height
				jg		loc_DrawSpriteAlpha_0200
				jmp		loc_DrawSpriteAlpha_exit

loc_DrawSpriteAlpha_0220:

			push	eax
				push	edx
				mov		ecx, eax
				shr     ebx, 3
				mov		nAlpha, ebx

loc_DrawSpriteAlpha_0221:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_DrawSpriteAlpha_0221

				pop		edx
				pop		eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0204
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0200
				jmp		loc_DrawSpriteAlpha_exit

				//---------------------------------------------------------------------------
				// 左边界裁剪量 == 0
				// 右边界裁剪量 != 0
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0300:

			mov		edx, clip_width

loc_DrawSpriteAlpha_0301:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				movzx	ebx, BYTE ptr[esi]
			inc		esi
				or		ebx, ebx
				jnz		loc_DrawSpriteAlpha_0303
				//---------------------------------------------------------------------------
				// 处理 nAlpha == 0 的像素 (右边界内)
				//---------------------------------------------------------------------------
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0301
				neg		edx
				jmp		loc_DrawSpriteAlpha_0305
				//---------------------------------------------------------------------------
				// 处理 nAlpha != 0 的像素 (右边界内)
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0303:

			cmp		edx, eax
				jl		loc_DrawSpriteAlpha_0304

				cmp		ebx, 255
				jl		loc_DrawSpriteAlpha_0310

				push	eax
				push	edx
				mov		ecx, eax
				mov     ebx, pPalette

loc_DrawSpriteAlpha_Loop30:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec     ecx
				jg      loc_DrawSpriteAlpha_Loop30

				pop		edx
				pop		eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0301
				neg		edx
				jmp		loc_DrawSpriteAlpha_0305

loc_DrawSpriteAlpha_0310:

			push	eax
				push	edx
				mov		ecx, eax
				shr     ebx, 3
				mov		nAlpha, ebx

loc_DrawSpriteAlpha_0311:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_DrawSpriteAlpha_0311

				pop		edx
				pop		eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0301
				neg		edx
				jmp		loc_DrawSpriteAlpha_0305

				//---------------------------------------------------------------------------
				// 连续点的个数 (eax) > 裁减后的宽度 (edx)
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0304:

			cmp		ebx, 255
				jl		loc_DrawSpriteAlpha_0320

				push	eax
				push	edx
				mov		ecx, edx
				mov     ebx, pPalette

loc_DrawSpriteAlpha_Loop31:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec     ecx
				jg      loc_DrawSpriteAlpha_Loop31

				pop		edx
				pop		eax
				sub		eax, edx
				mov		edx, eax
				add		esi, eax
				add		edi, eax
				add		edi, eax
				jmp		loc_DrawSpriteAlpha_0305

loc_DrawSpriteAlpha_0320:

			push	eax
				push	edx
				mov		ecx, edx
				shr     ebx, 3
				mov		nAlpha, ebx

loc_DrawSpriteAlpha_0321:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_DrawSpriteAlpha_0321

				pop		edx
				pop		eax
				sub		eax, edx
				mov		edx, eax
				add		esi, eax
				add		edi, eax
				add		edi, eax
				jmp		loc_DrawSpriteAlpha_0305

				//---------------------------------------------------------------------------
				// 处理超过了右边界的部分, edx = 超过右边界部分的长度
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0305:

			mov		eax, edx
				mov		edx, clip_right
				sub		edx, eax
				jle		loc_DrawSpriteAlpha_0308

loc_DrawSpriteAlpha_0306:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				movzx	ebx, BYTE ptr[esi]
			inc		esi
				or		ebx, ebx
				jnz		loc_DrawSpriteAlpha_0307
				//---------------------------------------------------------------------------
				// 处理 nAlpha == 0 的像素 (右边界外)
				//---------------------------------------------------------------------------
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0306
				jmp		loc_DrawSpriteAlpha_0308
				//---------------------------------------------------------------------------
				// 处理 nAlpha != 0 的像素 (右边界外)
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0307:

			add		esi, eax
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jg		loc_DrawSpriteAlpha_0306

loc_DrawSpriteAlpha_0308:

			add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0300
				jmp		loc_DrawSpriteAlpha_exit

				//---------------------------------------------------------------------------
				// 左边界裁剪量 != 0
				// 右边界裁剪量 != 0
				//---------------------------------------------------------------------------
loc_DrawSpriteAlpha_0400:		// Line Begin

			mov		edx, clip_x

loc_Draw_GetLength:						// edx 记录该行压缩数据到裁剪左端的长度，可能是负值

			movzx	eax, BYTE ptr[esi]		// 取出压缩数据的长度
			inc		esi
				movzx	ebx, BYTE ptr[esi]		// 取出Alpha值
			inc		esi
				cmp		edx, eax
				jge		loc_Draw_AllLeft		// edx >= eax, 所有数据都在左边界外
				mov		ecx, clip_width		// ecx 得到Clipper宽度
				add		ecx, edx				// ecx = Clipper宽度 + 压缩数据左端被剪长度
				cmp		ecx, 0
				jle		loc_Draw_AllRight		// ecx <= 0，所有数据都在右边界外
				sub		ecx, eax				// 比较压缩数据长度和 ecx ，ecx小于0的话，ecx值为该段右端裁剪长度
				jge		loc_Draw_GetLength_0	// ecx >= eax 说明右端无裁剪
				cmp		edx, 0
				jl		loc_Draw_RightClip		// 左端有裁剪，右端也有
				jmp		loc_Draw_AllClip
loc_Draw_GetLength_0:
			cmp		edx, 0
				jl		loc_Draw_NoClip			// 左右都没裁剪
				jmp		loc_Draw_LeftClip
				//---------------------------------------------------------------------------
				// 全在左端外
				//---------------------------------------------------------------------------
loc_Draw_AllLeft:
			or		ebx, ebx
				jnz		loc_Draw_AllLeft_1
				//loc_Draw_AllLeft_0:	// alpha == 0
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jmp		loc_Draw_GetLength
loc_Draw_AllLeft_1: // alpha != 0
			add		edi, eax
				add		edi, eax
				add		esi, eax
				sub		edx, eax
				jmp		loc_Draw_GetLength
				//---------------------------------------------------------------------------
				// 全在右端外
				//---------------------------------------------------------------------------
loc_Draw_AllRight:
			or		ebx, ebx
				jnz		loc_Draw_AllRight_1
				//loc_Draw_AllRight_0:
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				mov		ebx, edx
				add		ebx, clip_width
				add		ebx, clip_right
				cmp		ebx, 0
				jl		loc_Draw_GetLength
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0400	// 行结束，下一行开始
				jmp		loc_DrawSpriteAlpha_exit		
loc_Draw_AllRight_1:
			add		edi, eax
				add		edi, eax
				add		esi, eax
				sub		edx, eax
				mov		ebx, edx
				add		ebx, clip_width
				add		ebx, clip_right
				cmp		ebx, 0
				jl		loc_Draw_GetLength
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0400	// 行结束，下一行开始
				jmp		loc_DrawSpriteAlpha_exit
				//---------------------------------------------------------------------------
				// 处理左右端都不用裁剪的压缩段
				//---------------------------------------------------------------------------
loc_Draw_NoClip:
			or		ebx, ebx
				jnz		loc_Draw_NoClip_1
				//loc_Draw_NoClip_0:
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jmp		loc_Draw_GetLength
loc_Draw_NoClip_1:
			cmp		ebx, 255
				jl		loc_Draw_NoClip_Alpha
				push	eax
				push	edx
				mov		ecx, eax
				mov		ebx, pPalette

loc_Draw_NoClip_Copy:
			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov		dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec		ecx
				jnz		loc_Draw_NoClip_Copy

				pop		edx
				pop		eax
				sub		edx, eax
				jmp		loc_Draw_GetLength

loc_Draw_NoClip_Alpha:
			push	eax
				push	edx
				mov		ecx, eax
				shr     ebx, 3
				mov		nAlpha, ebx

loc_Draw_NoClip_Alpha_LOOP:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_Draw_NoClip_Alpha_LOOP

				pop		edx
				pop		eax
				sub		edx, eax
				jmp		loc_Draw_GetLength
				//---------------------------------------------------------------------------
				// 处理左右端同时要裁剪的压缩段
				//---------------------------------------------------------------------------
loc_Draw_AllClip:
			or		ebx, ebx				// 设置标志位
				jnz		loc_Draw_AllClip_1		// Alpha值不为零的处理
				//loc_Draw_AllClip_0:
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				neg		ecx
				cmp		ecx, clip_right
				jl		loc_Draw_GetLength		// Spr该行没完，接着处理
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0400// 行结束，下一行开始
				jmp		loc_DrawSpriteAlpha_exit
loc_Draw_AllClip_1:
			add		edi, eax
				add		edi, eax
				add		esi, eax
				sub		edx, eax				// edx - eax < 0

				add		edi, edx				// 补回前面多减的部分
				add		edi, edx				// edi和esi指向实际要
				add		esi, edx				// 绘制的部分

				cmp		ebx, 255
				jl		loc_Draw_AllClip_Alpha
				push	eax
				push	edx
				push	ecx
				mov		ecx, clip_width		// 前后都被裁剪，所以绘制长度为clip_width
				mov		ebx, pPalette

loc_Draw_AllClip_Copy:
			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec     ecx
				jnz		loc_Draw_AllClip_Copy

				pop		ecx
				pop		edx
				pop		eax
				jmp		loc_Draw_AllClip_End

loc_Draw_AllClip_Alpha:
			push	eax
				push	edx
				push	ecx
				mov		ecx, clip_width
				shr     ebx, 3
				mov		nAlpha, ebx

loc_Draw_AllClip_Alpha_LOOP:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_Draw_AllClip_Alpha_LOOP

				pop		ecx
				pop		edx
				pop		eax
loc_Draw_AllClip_End:
			neg		ecx
				add		edi, ecx				// 把edi、esi指针指向下一段
				add		edi, ecx
				add		esi, ecx
				cmp		ecx, clip_right
				jl		loc_Draw_GetLength		// Spr该行没完，接着处理
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0400// 行结束，下一行开始
				jmp		loc_DrawSpriteAlpha_exit
				//---------------------------------------------------------------------------
				// 处理只有左端要裁剪的压缩段
				//---------------------------------------------------------------------------
loc_Draw_LeftClip:
			or		ebx, ebx
				jnz		loc_Draw_LeftClip_1

				//loc_Draw_LeftClip_0:
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				jmp		loc_Draw_GetLength
loc_Draw_LeftClip_1:
			add		edi, eax
				add		edi, eax
				add		esi, eax
				sub		edx, eax
				add		edi, edx
				add		edi, edx
				add		esi, edx

				cmp		ebx, 255
				jl		loc_Draw_LeftClip_Alpha
				push	eax
				push	edx
				mov		ecx, edx
				neg		ecx
				mov     ebx, pPalette

loc_Draw_LeftClip_Copy:

			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec     ecx
				jg      loc_Draw_LeftClip_Copy

				pop		edx
				pop		eax
				jmp		loc_Draw_GetLength

loc_Draw_LeftClip_Alpha:
			push	eax
				push	edx
				mov		ecx, edx
				neg		ecx
				shr     ebx, 3
				mov		nAlpha, ebx

loc_Draw_LeftClip_Alpha_LOOP:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_Draw_LeftClip_Alpha_LOOP

				pop		edx
				pop		eax
				jmp		loc_Draw_GetLength
				//---------------------------------------------------------------------------
				// 处理只有右端要裁剪的压缩段
				//---------------------------------------------------------------------------
loc_Draw_RightClip:
			or		ebx, ebx
				jnz		loc_Draw_RightClip_1

				//loc_Draw_RightClip_0:
				add		edi, eax
				add		edi, eax
				sub		edx, eax
				neg		ecx
				cmp		ecx, clip_right
				jl		loc_Draw_GetLength
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0400	// 行结束，下一行开始
				jmp		loc_DrawSpriteAlpha_exit

loc_Draw_RightClip_1:
			sub		edx, eax
				cmp		ebx, 255
				jl		loc_Draw_RightClip_Alpha
				push	eax
				push	edx
				push	ecx
				add		ecx, eax					// 得到实际绘制的长度
				mov		ebx, pPalette

loc_Draw_RightClip_Copy:
			movzx	eax, BYTE ptr[esi]
			inc		esi
				mov		dx, [ebx + eax * 2]
			mov		[edi], dx
				inc		edi
				inc		edi
				dec		ecx
				jnz		loc_Draw_RightClip_Copy

				pop		ecx
				pop		edx
				pop		eax
				jmp		loc_Draw_RightClip_End

loc_Draw_RightClip_Alpha:
			add		edi, eax
				add		edi, eax
				add		esi, eax
				jmp		loc_Draw_RightClip_End
				push	eax
				push	edx
				push	ecx
				add		ecx, eax
				shr     ebx, 3
				mov		nAlpha, ebx

loc_Draw_RightClip_Alpha_LOOP:

			push	ecx
				mov     ebx, pPalette

				movzx	eax, BYTE ptr[esi]
			inc		esi
				mov     cx, [ebx + eax * 2]    // ecx = ...rgb
			mov		ax, cx                 // eax = ...rgb
				shl		eax, 16                // eax = rgb...
				mov		ax, cx                 // eax = rgbrgb
				and		eax, nMask32           // eax = .g.r.b
				mov		cx, [edi]              // ecx = ...rgb
			mov		bx, cx                 // ebx = ...rgb
				shl		ebx, 16                // ebx = rgb...
				mov		bx, cx                 // ebx = rgbrgb
				and		ebx, nMask32           // ebx = .g.r.b
				mov		ecx, nAlpha            // ecx = alpha
				mul		ecx                    // eax:edx = eax*ecx
				neg		ecx                    // ecx = -alpha
				add		ecx, 32                // ecx = 32 - alpha
				xchg	eax, ebx               // exchange eax,ebx
				mul		ecx                    // eax = eax * (32 - alpha)
				add		eax, ebx               // eax = eax + ebx
				shr		eax, 5                 // c = (c1 * alpha + c2 * (32 - alpha)) / 32
				and     eax, nMask32           // eax = .g.r.b
				mov     cx, ax                 // ecx = ...r.b
				shr     eax, 16                // eax = ....g.
				or      ax, cx                 // eax = ...rgb

				mov		[edi], ax
				inc		edi
				inc		edi
				pop		ecx
				dec		ecx
				jnz		loc_Draw_RightClip_Alpha_LOOP

				pop		ecx
				pop		edx
				pop		eax

loc_Draw_RightClip_End:
			neg		ecx
				add		edi, ecx				// 把edi、esi指针指向下一段
				add		edi, ecx
				add		esi, ecx
				cmp		ecx, clip_right
				jl		loc_Draw_GetLength		// Spr该行没完，接着处理
				add		edi, nNextLine
				dec		clip_height
				jnz		loc_DrawSpriteAlpha_0400// 行结束，下一行开始
				jmp		loc_DrawSpriteAlpha_exit

loc_DrawSpriteAlpha_exit:
		}
	}
}

void DrawSprite3LevelAlpha(INT nMask32, void* pBuffer, INT width, INT height, INT nPitch, INT x, INT y, void* pPalette, void* pSprite, INT nWidth, INT nHeight, const RECT* pSrcRect)
{
	RECT sSrcRect;
	if( pSrcRect )
	{
		sSrcRect = *pSrcRect;
	}
	else
	{
		SetRect(&sSrcRect, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {x, y, x + sSrcRect.right - sSrcRect.left, y + sSrcRect.bottom - sSrcRect.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x + sSrcRect.left;
	INT clip_y = dest_rc.top - y + sSrcRect.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	pBuffer = (BYTE*)(pBuffer) + y * nPitch + x * 2;
	INT nBuffNextLine = nPitch - clip_width * 2;// next line add
	INT nSprSkip = nWidth * clip_y + clip_x;
	INT nSprSkipPerLine = nWidth - clip_width;
	INT clip_right = nWidth - clip_x - clip_width;

	__asm
	{
		mov     eax, pPalette
			movd    mm0, eax        // mm0: pPalette

			mov     eax, clip_width
			movd    mm1, eax        // mm1: Clipper.width

			mov     eax, nMask32
			movd    mm2, eax        // mm2: nMask32

			// mm3: nAlpha
			// mm4: 32 - nAlpha

			// mm7: push ecx, pop ecx
			// mm6: push edx, pop edx
			// mm5: push eax, pop eax

			//使edi指向canvas绘制起点,使esi指向图块数据起点,(跳过nSprSkip个像点的图形数据)
			mov		edi, pBuffer
			mov		esi, pSprite

			//_SkipSpriteAheadContent_:
		{
			mov		edx, nSprSkip
				or		edx, edx
				jz		_SkipSpriteAheadContentEnd_

_SkipSpriteAheadContentLocalStart_:
			{
				KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX
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
			movd	edx, mm1    // mm1: Clipper.width
_DrawFullLineSection_LineLocal_:
			{
				KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

					or		ebx, ebx
					jnz		_DrawFullLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawFullLineSection_LineLocal_

					add		edi, nBuffNextLine
					dec		clip_height
					jnz		_DrawFullLineSection_Line_
					jmp		_EXIT_WAY_

_DrawFullLineSection_LineLocal_Alpha_:
				{
					movd    mm5, eax
						mov		ecx, eax

						cmp		ebx, 200
						jl		_DrawFullLineSection_LineLocal_HalfAlpha_

						//_DrawFullLineSection_LineLocal_DirectCopy_:
					{
						movd    ebx, mm0    // mm0: pPalette
_DrawFullLineSection_CopyPixel_:
						{
							KD_COPY_PIXEL_USE_EAX
								loop	_DrawFullLineSection_CopyPixel_
						}

						movd    eax, mm5
							sub		edx, eax
							jg		_DrawFullLineSection_LineLocal_

							add		edi, nBuffNextLine
							dec		clip_height
							jnz		_DrawFullLineSection_Line_
							jmp		_EXIT_WAY_
					}

_DrawFullLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
_DrawFullLineSection_HalfAlphaPixel_:
							{
								KD_MIX_2_PIXEL_COLOR_USE_EABDX
									loop	_DrawFullLineSection_HalfAlphaPixel_
							}
							movd	edx, mm6
								movd    eax, mm5
								sub		edx, eax
								jg		_DrawFullLineSection_LineLocal_

								add		edi, nBuffNextLine
								dec		clip_height
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
				movd	edx, mm1    // mm1: Clipper.width
				or		eax, eax
				jnz		_DrawPartLineSection_LineLocal_CheckAlpha_

_DrawPartLineSection_LineLocal_:
			{
				KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

_DrawPartLineSection_LineLocal_CheckAlpha_:
				or		ebx, ebx
					jnz		_DrawPartLineSection_LineLocal_Alpha_
					add		edi, eax
					add		edi, eax
					sub		edx, eax
					jg		_DrawPartLineSection_LineLocal_

					dec		clip_height
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
						KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX

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

						movd	mm5, eax
						mov		ecx, eax
						cmp		ebx, 200
						jl		_DrawPartLineSection_LineLocal_HalfAlpha_

						//_DrawPartLineSection_LineLocal_DirectCopy_:
					{
						movd    ebx, mm0    // mm0: pPalette
_DrawPartLineSection_CopyPixel_:
						{
							KD_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_CopyPixel_
						}						
						movd    eax, mm5
							sub		edx, eax
							jmp		_DrawPartLineSection_LineLocal_
					}

_DrawPartLineSection_LineLocal_HalfAlpha_:
						{
							movd    mm6, edx
_DrawPartLineSection_HalfAlphaPixel_:
							{
								KD_MIX_2_PIXEL_COLOR_USE_EABDX
									loop	_DrawPartLineSection_HalfAlphaPixel_
							}
							movd	edx, mm6
								movd    eax, mm5
								sub		edx, eax
								jmp		_DrawPartLineSection_LineLocal_
						}
				}

_DrawPartLineSection_LineLocal_Alpha_Part_:
				{
					movd    mm5, eax
						mov		ecx, edx
						cmp		ebx, 200
						jl		_DrawPartLineSection_LineLocal_HalfAlpha_Part_

						//_DrawPartLineSection_LineLocal_DirectCopy_Part_:
					{
						movd    ebx, mm0    // mm0: pPalette
_DrawPartLineSection_CopyPixel_Part_:
						{
							KD_COPY_PIXEL_USE_EAX
								loop	_DrawPartLineSection_CopyPixel_Part_
						}						
						movd    eax, mm5

							dec		clip_height
							jz		_EXIT_WAY_

							sub		eax, edx
							mov		edx, eax
							mov		ebx, 255	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
							jmp		_DrawPartLineSection_LineSkip_
					}

_DrawPartLineSection_LineLocal_HalfAlpha_Part_:
						{
							movd    mm6, edx
_DrawPartLineSection_HalfAlphaPixel_Part_:
							{
								KD_MIX_2_PIXEL_COLOR_USE_EABDX
									loop	_DrawPartLineSection_HalfAlphaPixel_Part_
							}
							movd	edx, mm6
								movd    eax, mm5
								dec		clip_height
								jz		_EXIT_WAY_
								sub		eax, edx
								mov		edx, eax
								mov		ebx, 128	//如果想要确切的原ebx(alpha)值可以在前头push ebx，此处pop获得
								jmp		_DrawPartLineSection_LineSkip_
						}
				}
		}
		}
_EXIT_WAY_:
		emms
	}
}

void DrawBitmap16(void* lpBuffer, INT width, INT height, INT nPitch, INT x, INT y, void* lpBitmap, INT nWidth, INT nHeight, const RECT* src_rect)
{
	RECT _src_rc;
	if( src_rect )
	{
		_src_rc = *src_rect;
	}
	else
	{
		SetRect(&_src_rc, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {x, y, x + _src_rc.right - _src_rc.left, y + _src_rc.bottom - _src_rc.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x + _src_rc.left;
	INT clip_y = dest_rc.top - y + _src_rc.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	// 计算屏幕下一行的偏移
	INT ScreenOffset = nPitch - clip_width * 2;

	// 计算位图下一行的偏移
	INT BitmapOffset = nWidth * 2 - clip_width * 2;

	__asm
	{
		//---------------------------------------------------------------------------
		//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
		//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
		//---------------------------------------------------------------------------
		mov		eax, nPitch
			mov		ebx, y
			mul		ebx
			mov     ebx, x
			add		ebx, ebx
			add     eax, ebx
			mov		edi, lpBuffer
			add		edi, eax
			//---------------------------------------------------------------------------
			//  初始化 ESI 指向图块数据起点 (跳过 Clipper.top 行图形数据)
			//  esi += (nWidth * Clipper.top + Clipper.left) * 2
			//---------------------------------------------------------------------------
			mov		ecx, clip_y
			mov		eax, nWidth
			mul     ecx
			add     eax, clip_x
			add		eax, eax
			mov		esi, lpBitmap
			add     esi, eax
			//---------------------------------------------------------------------------
			// 以一次4个点的方式来绘制位图
			//---------------------------------------------------------------------------
			mov		edx, clip_height
			mov		ebx, clip_width
			mov		eax, 8

loc_DrawBitmap16mmx_0001:

		mov		ecx, ebx
			shr		ecx, 2
			jz      loc_DrawBitmap16mmx_0003

loc_DrawBitmap16mmx_0002:

		movq	mm0, [esi]
		add		esi, eax
			movq	[edi], mm0
			add		edi, eax
			dec		ecx
			jnz		loc_DrawBitmap16mmx_0002


loc_DrawBitmap16mmx_0003:
		mov		ecx, ebx
			and		ecx, 3
			rep		movsw
			add     esi, BitmapOffset
			add		edi, ScreenOffset
			dec		edx
			jnz		loc_DrawBitmap16mmx_0001
			emms
	}
}


void DrawBitmap16AlphaMMX(
						  void* lpBuffer, 
						  INT width, 
						  INT height, 
						  INT nPitch, 
						  INT x, 
						  INT y, 
						  void* lpBitmap, 
						  INT nWidth, 
						  INT nHeight, 
						  const RECT* src_rect )
{
	RECT _src_rc;
	if( src_rect )
	{
		_src_rc = *src_rect;
	}
	else
	{
		SetRect(&_src_rc, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {x, y, x + _src_rc.right - _src_rc.left, y + _src_rc.bottom - _src_rc.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x + _src_rc.left;
	INT clip_y = dest_rc.top - y + _src_rc.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	// 计算屏幕下一行的偏移
	INT ScreenOffset = nPitch - clip_width * 2;

	// 计算位图下一行的偏移
	INT BitmapOffset = nWidth * 2 - clip_width * 2;

	__asm
	{
		movq mm7, g_n64ColorMask
			//---------------------------------------------------------------------------
			//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
			//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
			//---------------------------------------------------------------------------
			mov		eax, nPitch
			mov		ebx, y
			mul		ebx
			mov     ebx, x
			add		ebx, ebx
			add     eax, ebx
			mov		edi, lpBuffer
			add		edi, eax
			//---------------------------------------------------------------------------
			//  初始化 ESI 指向图块数据起点 (跳过 Clipper.top 行图形数据)
			//  esi += (nWidth * Clipper.top + Clipper.left) * 2
			//---------------------------------------------------------------------------
			mov		ecx, clip_y
			mov		eax, nWidth
			mul     ecx
			add     eax, clip_x
			add		eax, eax
			mov		esi, lpBitmap
			add     esi, eax

			//------------------------------------------------------------------------
			//edx :高度
			//ebx : 宽度
			mov		edx, clip_height
			mov		ebx, clip_width
DrawLineStart:
		//循环计数
		mov		ebx, clip_width
			mov    ecx, ebx
DrawWith4PixelStart:
		sub  ecx, 4
			jb   DrawWith2PixelStart
			//////MMX///////////
			movq mm0, [esi]  // 源指针
		movq mm1, [edi]  //目标指针
		movq mm6, mm0     
			psrlw mm6,15    //alpha
			movq mm2, mm0
			psrlw mm2,5     //make rg
			psllw mm2,6
			pand  mm0,mm7  //B
			por   mm0,mm2  //转换成功
			psubw mm1,mm0  // A*(D-S) + S
			pmullw mm1,mm6
			paddw  mm1,mm0
			movq   [edi],mm1
			add    edi, 8 //
			add    esi, 8
			////////////////////
			jmp DrawWith4PixelStart
DrawWith2PixelStart:
		add ecx, 4
DrawWith2PixelStart1:
		sub ecx, 2
			jb  DrawWith1Pixel
			mov eax, [ dword ptr esi]
		movd mm0, eax
			mov eax, [ dword ptr edi ]
		movd mm1,eax
			movq mm6, mm0     
			psrlw mm6,15    //alpha
			movq mm2, mm0
			psrlw mm2,5     //make rg
			psllw mm2,6
			pand  mm0,mm7  //B
			por   mm0,mm2  //转换成功
			psubw mm1,mm0  // A*(D-S) + S
			pmullw mm1,mm6
			paddw  mm1,mm0
			movd   eax,mm1
			mov [dword ptr edi ], eax
			add esi, 4
			add edi,4
			jmp DrawWith2PixelStart1
DrawWith1Pixel:
		add ecx,2
			sub ecx,1
			jb  LineEnd
			//判断最高位(透明位)是否有值，有就是透明
			mov		ax, 0x8000
			and		ax, [esi]

		//不为0转移到绘画一点完成
		jnz		Pixel1End
			//移位，把1555格式变为565格式
			//取5-14位(1555的左边那个55)
			mov		ax, 0x7fe0
			and		ax, [esi]
		//把5-14位变为6-15位(把1555的最左边那个55变为565的最左边那个56)
		shl		ax, 1
			//取原图形点的0-4位
			shl		ebx, 16
			mov		bx, 0x1f
			and		bx, [esi]
		add		ax, bx
			shr		ebx, 16
			mov		[edi], ax
Pixel1End:
		add		esi, 2
			add		edi, 2
LineEnd :
		add	esi, BitmapOffset   //,,,,,,,,,,,,,,,
			add	edi, ScreenOffset
			dec	edx
			jnz DrawLineStart

			emms	
	}
}
void DrawBitmap16Alpha(void* lpBuffer, INT width, INT height, INT nPitch, INT x, INT y, void* lpBitmap, INT nWidth, INT nHeight, const RECT* src_rect)
{
	RECT _src_rc;
	if( src_rect )
	{
		_src_rc = *src_rect;
	}
	else
	{
		SetRect(&_src_rc, 0, 0, nWidth, nHeight);
	}

	RECT rc1 = {0, 0, width, height};
	RECT rc2 = {x, y, x + _src_rc.right - _src_rc.left, y + _src_rc.bottom - _src_rc.top};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	INT clip_x = dest_rc.left - x + _src_rc.left;
	INT clip_y = dest_rc.top - y + _src_rc.top;
	INT clip_width = dest_rc.right - dest_rc.left;
	INT clip_height = dest_rc.bottom - dest_rc.top;

	x = dest_rc.left;
	y = dest_rc.top;

	// 计算屏幕下一行的偏移
	INT ScreenOffset = nPitch - clip_width * 2;

	// 计算位图下一行的偏移
	INT BitmapOffset = nWidth * 2 - clip_width * 2;

	// 绘制函数的汇编代码
	__asm
	{
		//---------------------------------------------------------------------------
		//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
		//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
		//---------------------------------------------------------------------------
		mov		eax, nPitch
			mov		ebx, y
			mul		ebx
			mov     ebx, x
			add		ebx, ebx
			add     eax, ebx
			mov		edi, lpBuffer
			add		edi, eax
			//---------------------------------------------------------------------------
			//  初始化 ESI 指向图块数据起点 (跳过 Clipper.top 行图形数据)
			//  esi += (nWidth * Clipper.top + Clipper.left) * 2
			//---------------------------------------------------------------------------
			mov		ecx, clip_y
			mov		eax, nWidth
			mul     ecx
			add     eax, clip_x
			add		eax, eax
			mov		esi, lpBitmap
			add     esi, eax
			//---------------------------------------------------------------------------
			// 以一次1个点的方式来绘制位图，遇到Alpha位为1的点就忽略不绘制
			//---------------------------------------------------------------------------
			mov		edx, clip_height
			mov		ebx, clip_width

loc_DrawBitmap16Alpha_Init:
		mov		ecx, ebx

loc_DrawBitmap16Alpha_Paint_Line:

		/*判断最高位(透明位)是否有值，有就是透明*/
		mov		ax, 0x8000
			and		ax, [esi]
		/**/
		/*不为0转移到绘画一点完成*/
		jnz		Paint_Point_Finish
			/**/
			/*移位，把1555格式变为565格式*/
			//取5-14位(1555的左边那个55)
			mov		ax, 0x7fe0
			and		ax, [esi]
		//把5-14位变为6-15位(把1555的最左边那个55变为565的最左边那个56)
		shl		ax, 1
			//取原图形点的0-4位
			shl		ebx, 16
			mov		bx, 0x1f
			and		bx, [esi]
		add		ax, bx
			shr		ebx, 16
			/*移位完成，1555变为了565*/
			/*绘制一个点*/
			mov		[edi], ax
			/**/
Paint_Point_Finish:
		add		esi, 2
			add		edi, 2
			dec		ecx
			jnz		loc_DrawBitmap16Alpha_Paint_Line

			//loc_DrawBitmap16Alpha_Next_Line:
			add		esi, BitmapOffset
			add		edi, ScreenOffset
			dec		edx
			jnz		loc_DrawBitmap16Alpha_Init
	}
}

void DrawPixel(void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT nX, INT nY, INT nColor)
{
	__asm
	{
		mov		eax, nY
			cmp		eax, 0
			jl		loc_PutPixel_exit
			cmp		eax, canvas_h
			jge		loc_PutPixel_exit

			mov		ebx, nX
			cmp		ebx, 0
			jl		loc_PutPixel_exit
			cmp		ebx, canvas_w
			jge		loc_PutPixel_exit

			mov		ecx, nPitch
			mul		ecx
			add		eax, ebx
			add		eax, ebx
			mov		edi, lpBuffer
			add		edi, eax
			mov		eax, nColor
			mov		[edi], ax

loc_PutPixel_exit:
	}
}

void DrawPixelAlpha(INT nMask32, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT nX, INT nY, INT nColor, INT nAlpha)
{
	__asm
	{
		mov     eax, nY
			cmp		eax, 0
			jl		loc_PutPixelAlpha_exit
			cmp		eax, canvas_h
			jge		loc_PutPixelAlpha_exit

			mov		ebx, nX
			cmp		ebx, 0
			jl		loc_PutPixelAlpha_exit
			cmp		ebx, canvas_w
			jge		loc_PutPixelAlpha_exit

			mov		ecx, nPitch
			mul		ecx
			add     eax, ebx
			add     eax, ebx
			mov		edi, lpBuffer
			add     edi, eax

			mov     ax, [edi]
		mov		bx, ax
			sal		eax, 16
			mov		ax, bx
			and		eax, nMask32

			mov		ecx, nColor
			mov		bx, cx
			sal		ecx, 16
			mov		cx, bx
			and		ecx, nMask32

			mov		ebx, nAlpha
			mul		ebx
			neg		ebx
			add		ebx, 0x20
			xchg	eax, ecx
			mul		ebx
			add		eax, ecx
			sar		eax, 5

			and		eax, nMask32
			mov		bx, ax
			sar		eax, 16
			or		ax, bx
			mov     [edi], ax

loc_PutPixelAlpha_exit:
	}
}

void DrawLine(void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT x1, INT y1, INT x2, INT y2, INT nColor)
{
	INT d, x, y, ax, ay, sx, sy, dx, dy;

	dx = x2 - x1;
	ax = abs(dx) << 1;
	sx = (dx > 0) ? 1 : -1;

	dy = y2 - y1;
	ay = abs(dy) << 1;
	sy = (dy > 0) ? 1 : -1;

	x  = x1;
	y  = y1;

	if (ax > ay) 
	{
		d = ay - (ax >> 1);
		while (x != x2)
		{
			DrawPixel(lpBuffer, canvas_w, canvas_h, nPitch, x, y, nColor);
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - (ay >> 1);
		while (y != y2)
		{
			DrawPixel(lpBuffer, canvas_w, canvas_h, nPitch, x, y, nColor);
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
	DrawPixel(lpBuffer, canvas_w, canvas_h, nPitch, x, y, nColor);
}

void DrawLineAlpha(INT nMask32, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT x1, INT y1, INT x2, INT y2, INT nColor, INT nAlpha)
{
	INT d, x, y, ax, ay, sx, sy, dx, dy;

	dx = x2 - x1;
	ax = abs(dx) << 1;
	sx = (dx > 0) ? 1 : -1;

	dy = y2 - y1;
	ay = abs(dy) << 1;
	sy = (dy > 0) ? 1 : -1;

	x  = x1;
	y  = y1;

	if (ax > ay) 
	{
		d = ay - (ax >> 1);
		while (x != x2)
		{
			DrawPixelAlpha(nMask32, lpBuffer, canvas_w, canvas_h, nPitch, x, y, nColor, nAlpha);
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - (ay >> 1);
		while (y != y2)
		{
			DrawPixelAlpha(nMask32, lpBuffer, canvas_w, canvas_h, nPitch, x, y, nColor, nAlpha);
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
	DrawPixelAlpha(nMask32, lpBuffer, canvas_w, canvas_h, nPitch, x, y, nColor, nAlpha);
}

void DrawRectAlpha(INT nMask32, void* lpBuffer, INT canvas_w, INT canvas_h, INT nPitch, INT x, INT y, INT w, INT h, INT nColor, INT nAlpha)
{
	RECT rc1 = {0, 0, canvas_w, canvas_h};
	RECT rc2 = {x, y, x + w, y + h};

	RECT dest_rc;
	if( !RectIntersect( &rc1, &rc2, &dest_rc ) )
		return;

	x = dest_rc.left;
	y = dest_rc.top;
	w = dest_rc.right - dest_rc.left;
	h = dest_rc.bottom - dest_rc.top;

	INT ScreenOffset = nPitch - w * 2;

	__asm
	{
		//---------------------------------------------------------------------------
		//  计算 EDI 指向屏幕起点的偏移量 (以字节计)
		//  edi = (nPitch*Clipper.y + nX)*2 + lpBuffer
		//---------------------------------------------------------------------------
		mov		eax, nPitch
			mov		ebx, y
			mul		ebx
			mov     ebx, x
			add		ebx, ebx
			add     eax, ebx
			mov		edi, lpBuffer
			add		edi, eax

			mov     esi, nMask32    // esi:  nMask32

			mov		ecx, nColor
			mov		bx, cx
			sal		ecx, 16
			mov		cx, bx
			and		ecx, esi        // esi:  nMask32
			movd    mm1, ecx        // mm1: nMaskColor

			mov     ecx, nAlpha
			mov     eax, 0x20
			movd    mm3, ecx        // mm3: nAlpha
			sub     eax, ecx
			movd    ecx, mm1        // mm1: nMaskColor
			imul    eax, ecx      
			movd    mm1, eax        // mm1: nMaskColor * (32 - nAlpha)

			mov		ecx, h

			movd    mm6, w

			//---------------------------------------------------------------------------
			//  color tranfer
			//---------------------------------------------------------------------------

loc_ClearAlpha_0001:
		movd    mm7, ecx 
			movd	edx, mm6        // Clipper.width

loc_ClearAlpha_0002:
		mov     ax, [edi]
		mov		cx, ax
			sal		eax, 16
			movd	ebx, mm3        // mm3: nAlpha
			mov		ax, cx
			and		eax, esi        // esi:  nMask32 

			imul	eax, ebx
			movd    ecx, mm1          // mm1: nMaskColor * (32 - nAlpha)
			add		eax, ecx
			sar		eax, 5
			and		eax, esi         // esi:  nMask32

			mov		bx, ax
			sar		eax, 16
			add     edi, 2 
			or		ax, bx
			dec		edx
			mov     [edi - 2], ax
			jnz		loc_ClearAlpha_0002


			movd    ecx, mm7
			add		edi, ScreenOffset
			dec		ecx
			jnz		loc_ClearAlpha_0001
			emms
	}
}

/* -------------------------------------------------------------------------
//	文件名		：	kernel_text.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	字体、文本的绘制处理
//
// -----------------------------------------------------------------------*/
#ifndef __KERNEL_TEXT_H__
#define __KERNEL_TEXT_H__
#include "../represent_common/common.h"
//kernel - text
extern BOOL g_bScissorTestEnable;
extern RECT g_sScissorRect;
extern BOOL g_bForceDrawPic;
extern RECT g_sClipOverwriteRect;
extern BOOL g_bClipOverwrite;

enum
{
	emTEXT_CMD_COLOR_TAG		= 1<<1,
	emTEXT_CMD_INLINE_PICTURE	= 1<<2,
	emTEXT_CMD_UNDERLINE_TAG	= 1<<3,
};

struct KTextCmd
{
	DWORD	dwFontHandle;
	//	byte*	text;
	const wchar_t* pwchText;	//modify by wdb, 客户端统一使用unicode,显示字符串
	DWORD	dwBytes;
	KPOINT	cPos;
	INT		nLineWidthLimit; 
	DWORD	dwStartLine;
	DWORD	dwEndLine;
	INT		nLineInterSpace;
	INT		nCharInterSpace;
	DWORD	dwColor;
	DWORD	dwBorderColor;
	float	fInlinePicScaling;
	DWORD	dwOptions;
	INT     nFontId;	//字体大小，用于排版
};

struct KSymbolDrawable
{
	WORD	wCode;
	KPOINT	cPos;
	KPOINT	cSize;
	WORD	dwColor;
	WORD	dwBorderColor;
	BYTE	byCtrlId;
	BOOL	bUnderLine;
};

//font & text

void WriteText( BOOL bRgb565, KTextCmd* pCmd, LPDIRECTDRAWSURFACE pCanvas );

BOOL CheckFont(LPCSTR pszTypeface, DWORD dwCharSet, HWND hWnd);

#endif
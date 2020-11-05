/* -------------------------------------------------------------------------
//	文件名		：	kernel_text.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_text的定义,文本的定义
//
// -----------------------------------------------------------------------*/
#ifndef __KERNEL_TEXT_H__
#define __KERNEL_TEXT_H__
#include "../represent_common/common.h"
#include "font_media.h"
//kernel - text
//cmd
enum
{
	emTEXT_CMD_COLOR_TAG		= 1<<0,
	emTEXT_CMD_INLINE_PICTURE	= 1<<1,
	emTEXT_CMD_UNDERLINE_TAG	= 1<<2,
};

struct KTextCmd
{
	DWORD	dwFontHandle;
	//	byte*	text;
	const WCHAR* pwchText; //modify by wdb
	DWORD	dwBytes;
	KPOINT	cPos;
	INT		nLineWidthLimit; 
	DWORD	dwStartLine;
	DWORD	dwEndLine;
	INT		nLineInterSpace;
	INT		nCharInterSpace;
	DWORD	dwColor;
	DWORD	dwBorderColor;
	FLOAT	fInlinePicScaling;
	DWORD	dwOptions;
	INT     nFontId;	//字体大小，用于排版
};

//drawable
struct KSymbolDrawable
{
	WORD	wCode;
	KPOINT	cPos;
	KPOINT	cSize;
	DWORD	dwColor;
	DWORD	dwBorderColor;
	BYTE	byCtrlId;
	BOOL	bUnderLine;
};

//buf
extern vector<KSymbolDrawable>	g_vecSymbolDrawable;
extern vector<BOOL>				g_vecIsIcon;
extern vector<DWORD>			g_vecCharIndex;
extern vector<DWORD>			g_vecIconIndex;
extern KFont*					g_pCurFont;

//gpu objects
extern LPD3DIB				g_pTextMultiquadIndexBuffer;
extern DWORD				g_dwCharBodySrs;
extern DWORD				g_dwCharBorderSrs;
extern DWORD				g_dwCharBorderSrsEx;
extern DWORD				g_dwCharBodySrsAntialiased;
extern BOOL					g_bUseColor2;

//init
struct KTextConfig
{
	BOOL	bWordWrap;
	BOOL	bTextJustification;
};

extern KTextConfig g_sTextConfig;

struct KTextKernelInitParam
{
	LPD3DIB	pMultiquadIb;
	BOOL	bWordWrap;
	BOOL	bTextJustification;
};

BOOL InitTextKernel(void* pArg);
void ShutdownTextKernel();

//batch
struct KLineCmd;
extern vector<KLineCmd> g_vecUnderLineCmd;

void BatchTextCmd(void* pCmd, void* pArg);

//flush
extern KSymbolDrawable*	g_pIconSortVecBase;

inline INT IconSortCmpFunc(const void* pArg1, const void* pArg2) 
{
	return (g_pIconSortVecBase[*((DWORD*)pArg1)].wCode > g_pIconSortVecBase[*((DWORD*)pArg2)].wCode);
}

//flush text
extern BOOL	g_bScissorTestEnable;
extern RECT	g_sScissorRect;
void FlushTextCharBuf();
void FlushTextIconBuf();
void FlushTextBuf();
#endif

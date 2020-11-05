/* -------------------------------------------------------------------------
//	�ļ���		��	kernel_text.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-23 11:11:11
//	��������	��	���塢�ı��Ļ��ƴ���
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
	const wchar_t* pwchText;	//modify by wdb, �ͻ���ͳһʹ��unicode,��ʾ�ַ���
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
	INT     nFontId;	//�����С�������Ű�
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
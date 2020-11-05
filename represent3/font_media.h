/* -------------------------------------------------------------------------
//	文件名		：	lockable_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	font_t和font_media_t的定义
//
// -----------------------------------------------------------------------*/
#ifndef __FONT_MEDIA_H__
#define __FONT_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "../represent_common/ifile.h"

//media - font
struct KFontConfig
{
	D3DFORMAT	eFmt;
	BOOL		bSingleByteCharSet;
	BOOL		bEnableConditionalNonPow2;
	DWORD		dwFontTextureSize;
};

extern KFontConfig g_sFontConfig;

struct FontInitParam
{
	BOOL	bSingleByteCharSet;
	BOOL	bEnableConditionalNonPow2;
	DWORD	dwFontTextureSize;
};

class KFont
{
public:
	virtual INT				GetBorderWidth() = 0;
	virtual BOOL			GetAntialiased() = 0;
	virtual INT				GetCharHeight() = 0;
	virtual INT				GetCharWidth(WORD dwCode) = 0;
	virtual BOOL			WriteChar(WORD dwCode, const KPOINT* pSize, D3DXVECTOR2* pUV1, D3DXVECTOR2* pUV2) = 0;
	virtual LPD3DTEXTURE	Submit() = 0;
};

void DrawCharWithBorder(DWORD nColor, DWORD nBorderColor, void* lpBuffer, 
						INT nCanvasW, INT nCanvasH, INT nPitch, INT nX, INT nY, 
						void* lpFont, INT nCharW, INT nCharH);

class FontMedia : public KMedia, public KFont
{
public:
	//src
	BOOL						m_bSysFont;

	CHAR						m_szSrcTypeFace[32];	//sys
	INT							m_nSrcHeight;			//sys
	DWORD						m_dwSrcCharset;			//sys
	DWORD						m_dwSrcWeight;			//sys
	BOOL						m_bSrcItalic;			//sys
	BOOL						m_bSrcAntialiased;		//sys
	INT							m_nSrcBorderWidth;		//sys

	CHAR						m_szFileName[128];		//non-sys
	BOOL						m_bBig5;				//non-sys

	//loaded
	LPD3DBUFFER					m_pFileData;			//non-sys

	//processed
	//	buffer_t					filedata;			//non-sys
	INT							m_nCharWidth;			//non-sys
	//	INT							char_height;		//non-sys
	DWORD						m_dwCharCount;			//non-sys
	DWORD*						m_pOffsetTablePtr;		//non-sys
	BYTE*						m_pData;				//non-sys	
	//	INT							max_char_width;		//non-sys

	//produced
	WORD*						m_pCharBitmap;		//non-sys

	HDC							m_hMemdc;			//sys
	HBITMAP						m_hMembmp;			//sys
	HFONT						m_hFont;			//sys
	INT							m_nCharHeight;		//sys
	INT							m_nMaxCharWidth;	//sys
	INT							m_nHalfCharWidth;	//sys

	KPOINT						m_cTextureSize;
	LPD3DTEXTURE				m_pTexture;

	struct KMemCharInfo
	{
		WORD	wCode;
		KPOINT	cUV;
		KPOINT	cSize;
	};
	typedef map<WORD, DWORD>	MEMCHARMAP;

	KVList<KMemCharInfo>		m_vlstMemCharList;
	MEMCHARMAP					m_mapMemCharMap;
	BOOL						m_bHasFree;
	KPOINT						m_cNextFree;
	DWORD						m_dwLastInUse;
	vector<DWORD>				m_vecDirty;

	//ctor
	FontMedia(LPCSTR pszTypeFace, INT nHeight, DWORD dwCharset, DWORD dwWeight, BOOL bItalic, BOOL bAntialiased, INT nBorderWidthForAntialiased);	//sys
	FontMedia(LPCSTR pszFileName, BOOL bBig5);	//non-sys

	//media
	void	Load();
	void	Process();
	void	Produce();
	void	PostProcess() {}
	void	DiscardVideoObjects() {}
	void	Clear();
	void	Destroy() {Clear(); delete this;}
	void*	Product() {return m_pTexture ? (KFont*)this : 0;}

	//font
	INT		GetBorderWidth() {return (m_bSysFont && m_bSrcAntialiased) ? m_nSrcBorderWidth : 1;}
	BOOL	GetAntialiased() {return m_bSysFont ? m_bSrcAntialiased : FALSE;}
	INT		GetCharHeight() {return m_nCharHeight;}
	INT		GetCharWidth(WORD dwCode) 
	{
		//	return m_bSysFont ? (_IsDBCSDoubleByteChar(code) ? max_char_width : half_char_width) : char_width;

		if (m_bSysFont)
		{
			//为优化，对大陆版本做特殊处理，兼容原有模式
			if (JXTextRender::GetLocaleRegion() == emKPRODUCT_REGION_ZH_CN ||
				JXTextRender::GetLocaleRegion() == emKPRODUCT_REGION_ZH_TW)
			{
				const UnicodeRangeProp* pUsrItem = JXTextRender::GetUnicodeRange(dwCode);
				if (pUsrItem->nClass != SC_Ascii)
					return m_nMaxCharWidth;
				else
					return m_nHalfCharWidth;
			}
			else
			{
				return JXTextRender::GetMaxAdvance(m_nSrcHeight);	
			}
		}
		return m_nCharWidth;
	}
	BOOL	WriteChar(WORD dwCode, const KPOINT* pSize, D3DXVECTOR2* pUV1, D3DXVECTOR2* pUV2);
	LPD3DTEXTURE Submit();

	//internal
	void Submit4444();
	void Submit8xxx();
	void DrawChar(WORD dwCode)
	{
		if( m_bSysFont )
		{
			RECT rect = { 0, 0, m_nMaxCharWidth, m_nCharHeight };
			FillRect( m_hMemdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH) );
			//	TextOut( m_hMemdc, 0, 0, (char*)&code, _IsDBCSDoubleByteChar(code) ? 2 : 1 );
			TextOutW(m_hMemdc, 0, 0, (wchar_t*)&dwCode, 1);
		}
		else
		{
			memset( m_pCharBitmap, 0, 2 * m_nCharWidth * m_nCharHeight );
			DrawCharWithBorder( 2, 1, m_pCharBitmap, m_nCharWidth, m_nCharHeight, m_nCharWidth * 2, 0, 0, GetCharData(dwCode), m_nCharWidth, m_nCharHeight );
		}
	}
	DWORD GetPixel(INT nX, INT nY){ return m_bSysFont ? ::GetPixel(m_hMemdc, nX, nY) : m_pCharBitmap[nY * m_nCharWidth + nX]; }
	void* GetCharData(WORD dwCode)			//non-sys
	{
		if( m_bSysFont )
			return 0;

		if( !m_bBig5 )	//gb2312
		{
			DWORD uCharIndex;

			if( g_sFontConfig.bSingleByteCharSet )
			{
				uCharIndex = dwCode;
			}
			else
			{
				BYTE cFirst, cNext;
				cFirst = (BYTE)(dwCode & 0xFF);

				if( cFirst <= 0x80 )
				{
					cNext = cFirst < 0x5f ? (0x20 + cFirst) : (0x21 + cFirst);
					cFirst = 0xa3;
				}
				else
				{
					cNext = (BYTE)((dwCode >> 8) & 0xFF);
				}

				uCharIndex = (cFirst - 0x81) * 190 + (cNext - 0x40) - (cNext >> 7);
			}
			if( uCharIndex < m_dwCharCount && m_pOffsetTablePtr[uCharIndex] )
				return m_pData + m_pOffsetTablePtr[uCharIndex];
			return 0;
		}
		else
		{
			BYTE cFirst, cNext;
			cFirst = (BYTE)(dwCode & 0xFF);

			if( cFirst <= 0x80 )
			{
				cNext = 0x80 + cFirst;
				cFirst = 0x81;
			}
			else
			{
				cNext = (BYTE)((dwCode >> 8) & 0xFF);
			}

			DWORD uCharIndex = (cFirst - 0x81) * 157 + (cNext - 0x40);
			uCharIndex = (cNext > 0x80) ? (uCharIndex - 0x22) : uCharIndex;
			if( uCharIndex < m_dwCharCount && m_pOffsetTablePtr[uCharIndex])
				return m_pData + m_pOffsetTablePtr[uCharIndex];
			return 0;
		}
	}
};

KMedia* CreateFont(LPCSTR pszIdentifier, void* pArg);

BOOL InitFontCreator(void* pArg);

void ShutdownFontCreator();

//inline BOOL _IsDBCSLeadingByte( BYTE code )	{ return _font_config_.single_byte_char_set ? FALSE : (code > 0x80); }
//inline BOOL _IsDBCSDoubleByteChar( byte2 code ) { return _font_config_.single_byte_char_set ? FALSE : (code > 0x80); }
/*
inline BOOL _IsDBCSDoubleByteChar(byte2 code)
{	
const UnicodeRangeProp* pUsrItem = JXTextRender::GetUnicodeRange(code);
return (pUsrItem->nClass != SC_Ascii); 
}
*/
#endif

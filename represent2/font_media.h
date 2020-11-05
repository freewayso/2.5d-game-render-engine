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
	HWND hWnd;
	BOOL bSingleByteCharSet;
};
extern KFontConfig g_sFontConfig;

struct FontInitParam
{
	HWND hWnd;
	BOOL bSingleByteCharSet;
};

struct KFont
{
	virtual BOOL	IsSysFont() = 0;
	virtual BOOL	GetAntialiased() = 0;
	virtual INT		GetBorderWidth() = 0;	//for antialiased font
	virtual INT		GetCharHeight() = 0;
	virtual INT		GetCharWidth(WORD wCode) = 0;
	virtual void*	GetCharData(WORD wCode) = 0;
};

//inline BOOL _IsDBCSLeadingByte( byte code )	{ return _font_config_.single_byte_char_set ? FALSE : (code > 0x80); }
//inline BOOL _IsDBCSDoubleByteChar( byte2 code ) { return _font_config_.single_byte_char_set ? FALSE : (code > 0x80); }
/*
inline BOOL _IsDBCSDoubleByteChar( byte2 code )
{	
const UnicodeRangeProp* pUsrItem = JXTextRender::GetUnicodeRange(code);
return (pUsrItem->nClass != SC_Ascii); 
}*/

//sysfont
class FontMedia : public KMedia, public KFont
{
public:
	//src
	CHAR						m_szSrcTypeFace[32];
	INT							m_nSrcHeight;
	DWORD						m_dwSrcCharset;
	DWORD						m_dwSrcWeight;
	BOOL						m_bSrcItalic;
	BOOL						m_bSrcAntialiased;
	INT							m_nSrcBorderWidth;

	//processed
	HDC							m_hMemdc;
	HBITMAP						m_hMembmp;
	HFONT						m_hFont;
	INT							m_nCharHeight;
	INT							m_nMaxCharWidth;
	INT							m_nHalfCharWidth;

	map<WORD, void*>			mapChar;

	//media
	FontMedia( LPCSTR pszTypeFace, INT nHeight, DWORD dwCharset, DWORD dwWeight, BOOL bItalic, BOOL bAntialiased, INT nBorderWidth )
	{
		strcpy(m_szSrcTypeFace, pszTypeFace);
		m_nSrcHeight = nHeight;
		m_dwSrcCharset = dwCharset;
		m_dwSrcWeight = dwWeight;
		m_bSrcItalic = bItalic;
		m_bSrcAntialiased = bAntialiased;
		m_nSrcBorderWidth = nBorderWidth;

		m_hMemdc = 0;
		m_hMembmp = 0;
		m_hFont = 0;
	}

	void	Load() {}
	void	Process()
	{
		//create font
		LOGFONT sFontInfo;
		memset( &sFontInfo, 0, sizeof(sFontInfo) );
		strcpy( sFontInfo.lfFaceName, m_szSrcTypeFace );
		sFontInfo.lfHeight = m_nSrcHeight;
		sFontInfo.lfCharSet = (BYTE)m_dwSrcCharset;
		sFontInfo.lfWeight = m_dwSrcWeight;
		sFontInfo.lfItalic = m_bSrcItalic;
		sFontInfo.lfQuality = m_bSrcAntialiased ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
		m_hFont = CreateFontIndirect( &sFontInfo );
		if( !m_hFont )
		{
			Clear();
			return;
		}

		//create memdc/membmp
		HDC dc = GetDC( g_sFontConfig.hWnd );
		m_hMemdc = CreateCompatibleDC( dc );

		//get text metrics
		SelectObject( m_hMemdc, m_hFont );
		TEXTMETRIC metric;
		GetTextMetrics( m_hMemdc, &metric );
		m_nCharHeight = metric.tmHeight + 2;						//add 2 for border
		m_nMaxCharWidth = metric.tmMaxCharWidth + 2;				//add 2 for border
		m_nHalfCharWidth = metric.tmMaxCharWidth / 2 + 2;		//add 2 for border

		//create membmp
		m_hMembmp = CreateCompatibleBitmap( m_bSrcAntialiased ? dc : m_hMemdc, m_nMaxCharWidth, m_nCharHeight );
		ReleaseDC( g_sFontConfig.hWnd, dc );

		// set dc properties
		SelectObject( m_hMemdc, m_hMembmp );
		SetBkColor( m_hMemdc, RGB(0,0,0) );
		SetTextColor( m_hMemdc, RGB(255,255,255) );

		if( !m_hMemdc || !m_hMembmp)
		{
			Clear();
			return;
		}
	}

	void	Produce() {}
	void	PostProcess() {}
	void	DiscardVideoObjects() {}
	void	Clear()
	{
		if( m_hFont )
		{
			DeleteObject(m_hFont);
			m_hFont = 0;
		}
		if( m_hMembmp )
		{
			DeleteObject(m_hMembmp); 
			m_hMembmp = 0;
		}
		if( m_hMemdc )
		{
			DeleteDC(m_hMemdc);
			m_hMemdc = 0;
		}

		map<WORD, void*>::iterator p = mapChar.begin();
		while( p != mapChar.end() )
		{
			SAFE_DELETE_ARRAY( p->second );
			p++;
		}
		mapChar.clear();
	}

	void	Destroy() {Clear(); delete this;}
	void*	Product() {return m_hFont ? (KFont*)this : 0;}

	//font
	BOOL	IsSysFont()				{return TRUE;}
	BOOL	GetAntialiased()			{return m_bSrcAntialiased;}
	INT		GetBorderWidth()			{return m_bSrcAntialiased ? m_nSrcBorderWidth : 1;}
	INT		GetCharHeight()			{return m_nCharHeight;}
	//	INT		GetCharWidth(byte2 code)	{return _IsDBCSDoubleByteChar(code) ? max_char_width : half_char_width;}
	INT		GetCharWidth(WORD code)
	{	
		//为优化，对大陆版本做特殊处理，兼容原有模式
		if (JXTextRender::GetLocaleRegion() == emKPRODUCT_REGION_ZH_CN ||
			JXTextRender::GetLocaleRegion() == emKPRODUCT_REGION_ZH_TW)
		{
			const UnicodeRangeProp* pUsrItem = JXTextRender::GetUnicodeRange(code);
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
	void*	GetCharData(WORD code)
	{
		//find in the map first
		map<WORD, void*>::iterator _p = mapChar.find( code );
		if( _p != mapChar.end() )
		{
			return _p->second;
		}

		//clear if necessary
		RECT rc = { 0, 0, m_nMaxCharWidth, m_nCharHeight };
		FillRect( m_hMemdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH) );

		//text out, modify by wdb
		//	if( !TextOut( m_hMemdc, 0, 0, (char*)&code, _IsDBCSDoubleByteChar(code) ? 2 : 1 ) )		
		if (!TextOutW(m_hMemdc, 0, 0, (wchar_t*)&code, 1))
		{
			return 0;
		}

		//get size
		INT w = GetCharWidth(code);
		INT h = GetCharHeight();

		//allocate (1 byte for each pixel)
		BYTE* p = new BYTE[w * h];
		DWORD dwPitch = w;

		//fill background (0)
		for( INT v = 0; v < h; v++ )
		{
			memset( p + v * dwPitch, 0, w );
		}

		//fill border (1)
		if( !m_bSrcAntialiased )
		{
			for( INT v = 0; v < h - 2; v++ )
			{
				for( INT u = 0; u < w - 2; u++ )
				{
					if( GetPixel(m_hMemdc, u, v) )	//background is black
					{
						BYTE* center = p + (v+1)*dwPitch + (u+1);
						*((BYTE*)(center-dwPitch-1))	= 1;
						*((BYTE*)(center-dwPitch))	= 1;
						*((BYTE*)(center-dwPitch+1))	= 1;
						*((BYTE*)(center-1))		= 1;
						*((BYTE*)(center+1))		= 1;
						*((BYTE*)(center+dwPitch-1))	= 1;
						*((BYTE*)(center+dwPitch))	= 1;
						*((BYTE*)(center+dwPitch+1))	= 1;
					}
				}
			}
		}

		//fill text (2)
		for( INT v = 0; v < h - 2; v++ )
		{
			for( INT u = 0; u < w - 2; u++ )
			{
				DWORD pixel = GetPixel(m_hMemdc, u, v);
				if( pixel )
				{
					BYTE* center = p + (v+1)*dwPitch + (u+1);
					*((BYTE*)center) = m_bSrcAntialiased ? (BYTE)(pixel & 0xff) : 2;
				}
			}
		}

		//add to map
		mapChar[code] = p;

		//return 
		return p;
	}
};

//------user_font_media_t 应该是可以取消

//.fnt font
class KUserFontMedia : public KMedia, public KFont
{
public:
	//src
	string		m_strFileName;
	BOOL		m_bBig5;

	//loaded
	LPD3DBUFFER	m_pFiledata;

	//processed
	//	buffer_t	filedata;
	INT			m_nCharWidth;
	INT			m_nCharHeight;
	DWORD		m_dwCharCount;
	DWORD*		m_pOffsetTable;
	BYTE*		m_pData;

	//ctor 
	KUserFontMedia( LPCSTR pszFileName, BOOL bBig5 )
	{
		if( pszFileName )
			m_strFileName = pszFileName;
		else
			m_strFileName = "";

		m_bBig5 = bBig5;

		m_pFiledata = 0;
		m_pOffsetTable = 0;
		m_pData = 0;
	}

	//media
	void Load() {LoadFile(m_strFileName.c_str(), &m_pFiledata);}
	void Process() 
	{
		//pre-check
		if( !m_pFiledata )
		{
			Clear();
			return;
		}

		struct FontHeader
		{
			CHAR	szId[4];		// 标识
			DWORD	dwSize;		// 大小
			DWORD	dwCount;		// 数量
			WORD	wWidth;		// 宽度
			WORD	wHeight;		// 高度
		};

		//check size
		if( m_pFiledata->GetBufferSize() < sizeof( FontHeader ) )
		{
			Clear();
			return;
		}

		//get header
		FontHeader* pHead = (FontHeader*)m_pFiledata->GetBufferPointer();

		//check id
		if( *((INT*)(&pHead->szId)) != 0x465341 )
		{
			Clear();
			return;
		}

		//check size
		if( m_pFiledata->GetBufferSize() != sizeof( FontHeader ) + 4 * pHead->dwCount + pHead->dwSize )
		{
			Clear();
			return;
		}

		//get info
		m_nCharWidth = pHead->wWidth;
		m_nCharHeight = pHead->wHeight;
		m_dwCharCount = pHead->dwCount;
		m_pOffsetTable = (DWORD*)&pHead[1];
		m_pData = (BYTE*)(m_pOffsetTable + m_dwCharCount);
	}
	void Produce() {}
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear() 
	{
		SAFE_RELEASE(m_pFiledata); 
		m_pOffsetTable = 0;
		m_pData = 0;
	}
	void Destroy() {Clear(); delete this;}
	void* Product() {return m_pData ? (KFont*)this : 0;}

	//font_t
	BOOL	IsSysFont()				{return FALSE;}
	BOOL	GetAntialiased()			{return FALSE;}
	INT		GetBorderWidth()			{return 1;}
	INT		GetCharHeight()			{return m_nCharHeight;}
	INT		GetCharWidth(WORD wCode)	{return m_nCharWidth;}
	void*	GetCharData(WORD wCode)
	{
		if( !m_bBig5 )	//gb2312
		{
			DWORD uCharIndex;
			if( g_sFontConfig.bSingleByteCharSet )
			{
				uCharIndex = wCode;
			}
			else
			{
				BYTE cFirst, cNext;
				cFirst = (BYTE)(wCode & 0xFF);

				if( cFirst <= 0x80 )
				{
					cNext = cFirst < 0x5f ? (0x20 + cFirst) : (0x21 + cFirst);
					cFirst = 0xa3;
				}
				else
				{
					cNext = (BYTE)((wCode >> 8) & 0xFF);
				}

				uCharIndex = (cFirst - 0x81) * 190 + (cNext - 0x40) - (cNext >> 7);
			}
			if( uCharIndex < m_dwCharCount && m_pOffsetTable[uCharIndex] )
				return m_pData + m_pOffsetTable[uCharIndex];
			return 0;
		}
		else
		{
			BYTE cFirst, cNext;
			cFirst = (BYTE)(wCode & 0xFF);

			if( cFirst <= 0x80 )
			{
				cNext = 0x80 + cFirst;
				cFirst = 0x81;
			}
			else
			{
				cNext = (BYTE)((wCode >> 8) & 0xFF);
			}

			DWORD uCharIndex = (cFirst - 0x81) * 157 + (cNext - 0x40);
			uCharIndex = (cNext > 0x80) ? (uCharIndex - 0x22) : uCharIndex;
			if( uCharIndex < m_dwCharCount && m_pOffsetTable[uCharIndex])
				return m_pData + m_pOffsetTable[uCharIndex];
			return 0;
		}
	}
};

KMedia* CreateFont( LPCSTR pszIdentifier, void* pArg );
BOOL InitFontCreator(void* pArg);
void ShutdownFontCreator();

#endif
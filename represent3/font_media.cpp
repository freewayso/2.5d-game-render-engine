/* -------------------------------------------------------------------------
//	文件名		：	font_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	font_media_t的定义
//
// -----------------------------------------------------------------------*/
#include "font_media.h"
#include "igpu.h"
#include "gpu_helper.h"

KFontConfig g_sFontConfig;

FontMedia::FontMedia(LPCSTR pszTypeFace, INT nHeight, DWORD dwCharset, DWORD dwWeight, BOOL bItalic, BOOL bAntialiased, INT nBorderWidthForAntialiased )
{
	m_bSysFont = TRUE;

	strcpy(m_szSrcTypeFace, pszTypeFace);
	m_nSrcHeight = nHeight;
	m_dwSrcCharset = dwCharset;
	m_dwSrcWeight = dwWeight;
	m_bSrcItalic = bItalic;
	m_bSrcAntialiased = bAntialiased;
	m_nSrcBorderWidth = nBorderWidthForAntialiased;

	m_hMemdc = 0;
	m_hMembmp = 0;
	m_hFont = 0;
	m_pTexture = 0;
}

FontMedia::FontMedia(LPCSTR pszFileName, BOOL bBig5)
{
	m_bSysFont = FALSE;

	if( pszFileName )
		strcpy( m_szFileName, pszFileName );
	else
		m_szFileName[0] = 0;
	m_bBig5 = bBig5;

	m_pFileData = 0;

	m_pOffsetTablePtr = 0;
	m_pData = 0;

	m_pCharBitmap = 0;
	m_pTexture = 0;
}

void FontMedia::Load()
{
	if( !m_bSysFont )
		LoadFile( m_szFileName, &m_pFileData );
}

void FontMedia::Process()
{
	if( !m_bSysFont )
	{
		//pre-check
		if( !m_pFileData )
		{
			Clear();
			return;
		}

		struct KHeader
		{
			char	szId[4];		// 标识
			DWORD	dwSize;		// 大小
			DWORD	dwCount;		// 数量
			WORD	wWidth;		// 宽度
			WORD	wHeight;		// 高度
		};

		//check size
		if( m_pFileData->GetBufferSize() < sizeof( KHeader ) )
		{
			Clear();
			return;
		}

		//get header
		KHeader* pHead = (KHeader*)m_pFileData->GetBufferPointer();

		//check id
		if( *((INT*)(&pHead->szId)) != 0x465341 )
		{
			Clear();
			return;
		}

		//check size
		if( m_pFileData->GetBufferSize() != sizeof( KHeader ) + 4 * pHead->dwCount + pHead->dwSize )
		{
			Clear();
			return;
		}

		//get info
		m_nCharHeight = pHead->wHeight;
		m_nMaxCharWidth = m_nCharWidth = pHead->wWidth;
		m_dwCharCount = pHead->dwCount;
		m_pOffsetTablePtr = (DWORD*)&pHead[1];
		m_pData = (BYTE*)(m_pOffsetTablePtr + m_dwCharCount);
	}
}

void FontMedia::Produce()
{
	if( m_bSysFont )
	{
		//create font
		LOGFONT sFontInfo;
		memset(&sFontInfo, 0, sizeof(sFontInfo));
		strcpy(sFontInfo.lfFaceName, m_szSrcTypeFace);
		sFontInfo.lfHeight = m_nSrcHeight;
		sFontInfo.lfCharSet = (BYTE)m_dwSrcCharset;
		sFontInfo.lfWeight = m_dwSrcWeight;
		sFontInfo.lfItalic = m_bSrcItalic;
		sFontInfo.lfQuality = m_bSrcAntialiased ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
		m_hFont = CreateFontIndirect(&sFontInfo);
		if(!m_hFont)
		{
			Clear();
			return;
		}

		//create memdc
		HDC hDC = GetDC(Gpu()->GetCreationParameters()->hWnd);
		m_hMemdc = CreateCompatibleDC(hDC);

		//get font metric
		SelectObject(m_hMemdc, m_hFont);
		TEXTMETRIC metric;
		GetTextMetrics(m_hMemdc, &metric);
		m_nCharHeight = metric.tmHeight+2;					//add 2 for border
		m_nMaxCharWidth = metric.tmMaxCharWidth+2;			//add 2 for border
		m_nHalfCharWidth = metric.tmMaxCharWidth/2 + 2;		//add 2 for border

		//create membmp
		m_hMembmp = CreateCompatibleBitmap(m_bSrcAntialiased ? hDC : m_hMemdc, m_nMaxCharWidth, m_nCharHeight);
		ReleaseDC(Gpu()->GetCreationParameters()->hWnd, hDC);

		// set dc properties
		SelectObject(m_hMemdc, m_hMembmp);
		SetBkColor(m_hMemdc, RGB(0,0,0));
		SetTextColor(m_hMemdc, RGB(255,255,255));

		if( !m_hMemdc || !m_hMembmp )
		{
			Clear();
			return;
		}
	}
	else
	{
		m_pCharBitmap = (WORD*) new CHAR[m_nCharWidth * m_nCharHeight * 2];
		if( !m_pCharBitmap )
		{
			Clear();
			return;
		}
	}

	AdjustTextureSize(Gpu(), g_sFontConfig.bEnableConditionalNonPow2, FALSE, 0, &KPOINT(g_sFontConfig.dwFontTextureSize, g_sFontConfig.dwFontTextureSize), &m_cTextureSize);
	if(!Gpu()->CreateTexture(m_cTextureSize.x, m_cTextureSize.y, 1, 0, g_sFontConfig.eFmt, D3DPOOL_MANAGED, &m_pTexture))
	{
		Clear();
		return;
	}

	PlusMediaBytes( (DWORD)(m_cTextureSize.x * m_cTextureSize.y * FormatBytes(g_sFontConfig.eFmt)) );

	m_bHasFree = TRUE;
	m_cNextFree = KPOINT(0, 0);
	m_dwLastInUse = 0;
}

void FontMedia::Clear()
{
	if( m_bSysFont )
	{
		if(m_hFont)
		{
			DeleteObject(m_hFont);
			m_hFont = 0;
		}
		if(m_hMembmp)
		{
			DeleteObject(m_hMembmp); 
			m_hMembmp = 0;
		}
		if(m_hMemdc)
		{
			DeleteDC(m_hMemdc);
			m_hMemdc = 0;
		}
	}
	else
	{
		SAFE_DELETE_ARRAY(m_pCharBitmap);
		SAFE_RELEASE(m_pFileData); 
		m_pOffsetTablePtr = 0;
		m_pData = 0;
	}

	if( m_pTexture) 
	{
		MinusMediaBytes( (DWORD)(m_cTextureSize.x * m_cTextureSize.y * FormatBytes(g_sFontConfig.eFmt)) );
		SAFE_RELEASE(m_pTexture);
	}
	m_vlstMemCharList.Clear();
	m_mapMemCharMap.clear();
	m_vecDirty.clear();
}

BOOL FontMedia::WriteChar(WORD wCode, const KPOINT* size, D3DXVECTOR2* uv1, D3DXVECTOR2* uv2)
{
	MEMCHARMAP::iterator p = m_mapMemCharMap.find(wCode);
	if(p == m_mapMemCharMap.end() && !m_bHasFree && m_dwLastInUse == m_vlstMemCharList.RBegin())
		return FALSE;	//no more cell
	if(p != m_mapMemCharMap.end())	//already in map
	{
		DWORD entry = p->second;
		//update last_in_use
		if(m_dwLastInUse == 0)
			m_dwLastInUse = entry;
		else if(m_dwLastInUse == entry && m_vlstMemCharList.Begin() != entry)
			m_dwLastInUse = m_vlstMemCharList.Pre(m_dwLastInUse);
		//bring to head
		m_vlstMemCharList.Move(entry, m_vlstMemCharList.Begin());
		//return uv
		KMemCharInfo& c = m_vlstMemCharList.Value(entry);
		*uv1 = D3DXVECTOR2(c.cUV.x/(float)m_cTextureSize.x, c.cUV.y/(float)m_cTextureSize.y);
		*uv2 = D3DXVECTOR2((c.cUV.x + c.cSize.x)/(float)m_cTextureSize.x, (c.cUV.y + c.cSize.y)/(float)m_cTextureSize.y);
	}
	else if(m_bHasFree)	//not in map, but has free cell
	{
		//push to list front
		KMemCharInfo new_char;
		new_char.cUV = m_cNextFree;
		new_char.wCode = wCode;
		new_char.cSize = *size;
		m_vlstMemCharList.PushFront(new_char);
		DWORD new_entry = m_vlstMemCharList.Begin();
		//add to map
		m_mapMemCharMap[wCode] = new_entry;
		//update last_in_use
		if(m_dwLastInUse == 0)
			m_dwLastInUse = new_entry;
		//push to dirty vec
		m_vecDirty.push_back(new_entry);
		//update has_free, next_free
		m_cNextFree.x += m_nMaxCharWidth;
		if(m_cNextFree.x + m_nMaxCharWidth > m_cTextureSize.x)
		{
			m_cNextFree.x = 0;
			m_cNextFree.y += m_nCharHeight;
			if(m_cNextFree.y + m_nCharHeight > m_cTextureSize.y)
				m_bHasFree = FALSE;
		}
		//return uv
		*uv1 = D3DXVECTOR2(new_char.cUV.x/(float)m_cTextureSize.x, new_char.cUV.y/(float)m_cTextureSize.y);
		*uv2 = D3DXVECTOR2((new_char.cUV.x + new_char.cSize.x)/(float)m_cTextureSize.x, (new_char.cUV.y + new_char.cSize.y)/(float)m_cTextureSize.y);
	}
	else	//not in map, and have no free cell
	{
		WORD dwOldCode = m_vlstMemCharList.Back().wCode;
		//change code & size
		m_vlstMemCharList.Back().wCode = wCode;
		m_vlstMemCharList.Back().cSize = *size;
		//erase from map
		m_mapMemCharMap.erase(dwOldCode);
		//bring to head
		m_vlstMemCharList.Move(m_vlstMemCharList.RBegin(), m_vlstMemCharList.Begin());
		//add to map
		m_mapMemCharMap[wCode] = m_vlstMemCharList.Begin();
		//add to dirty vec
		m_vecDirty.push_back(m_vlstMemCharList.Begin());
		//update last_in_use
		if(m_dwLastInUse == 0)
			m_dwLastInUse = m_vlstMemCharList.Begin();
		//return uv
		KMemCharInfo& first = m_vlstMemCharList.Front();
		*uv1 = D3DXVECTOR2(first.cUV.x/(float)m_cTextureSize.x, first.cUV.y/(float)m_cTextureSize.y);
		*uv2 = D3DXVECTOR2((first.cUV.x + first.cSize.x)/(float)m_cTextureSize.x, (first.cUV.y + first.cSize.y)/(float)m_cTextureSize.y);
	}
	return TRUE;
}

LPD3DTEXTURE FontMedia::Submit()
{
	//clear history
	m_dwLastInUse = 0;

	if(!m_vecDirty.empty())
	{
		if( g_sFontConfig.eFmt == D3DFMT_A8 )
			Submit8xxx();
		else
			Submit4444();

		m_vecDirty.resize(0);
	}
	return m_pTexture;
}

void FontMedia::Submit4444()
{
	D3DLOCKED_RECT locked_rect;
	m_pTexture->LockRect(0, &locked_rect, 0, D3DLOCK_NO_DIRTY_UPDATE);	//add dirty region manually
	INT pitch = locked_rect.Pitch;
	for(DWORD i = 0; i < m_vecDirty.size(); i++)
	{
		KMemCharInfo& c = m_vlstMemCharList.Value(m_vecDirty[i]);
		DrawChar(c.wCode);

		INT w = c.cSize.x;
		INT h = c.cSize.y;
		RECT rc = {c.cUV.x, c.cUV.y, c.cUV.x+w, c.cUV.y+h};
		BYTE* p = (BYTE*)locked_rect.pBits + rc.top * pitch + rc.left*2;
		if( m_bSysFont )
		{
			//background
			for(INT v = 0; v < h; v++)
			{
				memset(p + v*pitch, m_bSrcAntialiased ? 0 : 0x77, w*2);
			}
			//fill border
			if( !m_bSrcAntialiased )
			{
				for(INT v = 0; v < h-2; v++)
				{
					for(INT u = 0; u < w-2; u++)
					{
						if( GetPixel( u, v ) )
						{
							BYTE* center = p + (v+1)*pitch + (u+1)*2;
							*((WORD*)(center-pitch-2)) = 0x0000;
							*((WORD*)(center-pitch))	= 0x0000;
							*((WORD*)(center-pitch+2)) = 0x0000;
							*((WORD*)(center-2))		= 0x0000;
							*((WORD*)(center+2))		= 0x0000;
							*((WORD*)(center+pitch-2)) = 0x0000;
							*((WORD*)(center+pitch))	= 0x0000;
							*((WORD*)(center+pitch+2)) = 0x0000;
						}
					}
				}
			}
			//fill text
			for(INT v = 0; v < h-2; v++)
			{
				for(INT u = 0; u < w-2; u++)
				{
					DWORD pixel = GetPixel(u,v);
					if( pixel )
					{
						BYTE* center = p + (v+1)*pitch + (u+1)*2;
						*((WORD*)center) = m_bSrcAntialiased ? (WORD)(pixel & 0xffff) : 0xffff;
					}
				}
			}
		}
		else
		{
			for(INT v = 0; v < h; v++)
			{
				for(INT u = 0; u < w; u++)
				{
					BYTE* center = p + v * pitch + u * 2;
					switch( GetPixel( u, v ) )
					{
					case 0:	//background
						*((WORD*)center) = 0x7777;
						break;
					case 1:	//border
						*((WORD*)center) = 0x0000;
						break;
					case 2:	//body
						*((WORD*)center) = 0xffff;
						break;
					}
				}
			}
		}
		m_pTexture->AddDirtyRect(&rc);
	}
	m_pTexture->UnlockRect(0);
}

void FontMedia::Submit8xxx()
{
	D3DLOCKED_RECT sLockedRect;
	m_pTexture->LockRect(0, &sLockedRect, 0, D3DLOCK_NO_DIRTY_UPDATE);
	INT nPitch = sLockedRect.Pitch;
	for(DWORD i = 0; i < m_vecDirty.size(); i++)
	{
		KMemCharInfo& rCharInfo = m_vlstMemCharList.Value(m_vecDirty[i]);
		DrawChar( rCharInfo.wCode );

		INT w = rCharInfo.cSize.x;
		INT h = rCharInfo.cSize.y;
		RECT rect = {rCharInfo.cUV.x, rCharInfo.cUV.y, rCharInfo.cUV.x+w, rCharInfo.cUV.y+h};
		BYTE* p = (BYTE*)sLockedRect.pBits + rect.top * nPitch + rect.left*1;
		if( m_bSysFont )
		{
			//background
			for(INT v = 0; v < h; v++)
			{
				memset(p + v*nPitch, m_bSrcAntialiased ? 0 : 0x7f, w*1);
			}
			//fill border
			if( !m_bSrcAntialiased )
			{
				for(INT v = 0; v < h-2; v++)
				{
					for(INT u = 0; u < w-2; u++)
					{
						if( GetPixel( u, v ) )
						{
							BYTE* center = p + (v+1)*nPitch + (u+1)*1;
							*((BYTE*)(center-nPitch-1))	= 0x00;
							*((BYTE*)(center-nPitch))	= 0x00;
							*((BYTE*)(center-nPitch+1))	= 0x00;
							*((BYTE*)(center-1))		= 0x00;
							*((BYTE*)(center+1))		= 0x00;
							*((BYTE*)(center+nPitch-1))	= 0x00;
							*((BYTE*)(center+nPitch))	= 0x00;
							*((BYTE*)(center+nPitch+1))	= 0x00;
						}
					}
				}
			}
			//fill text
			for(INT v = 0; v < h-2; v++)
			{
				for(INT u = 0; u < w-2; u++)
				{
					DWORD pixel = GetPixel(u,v);
					if( pixel )
					{
						BYTE* center = p + (v+1)*nPitch + (u+1)*1;
						*((BYTE*)center) = m_bSrcAntialiased ? (BYTE)(pixel & 0xff) : 0xff;
					}
				}
			}
		}
		else
		{
			for(INT v = 0; v < h; v++)
			{
				for(INT u = 0; u < w; u++)
				{
					BYTE* center = p + (v)*nPitch + (u)*1;
					switch( GetPixel( u, v ) )
					{
					case 0:	//background
						*center = 0x7f;
						break;
					case 1:	//border
						*center = 0x00;
						break;
					case 2:	//body
						*center = 0xff;
						break;
					}
				}
			}
		}
		m_pTexture->AddDirtyRect(&rect);
	}
	m_pTexture->UnlockRect(0);
}

void DrawCharWithBorder(DWORD nColor, DWORD nBorderColor, void* lpBuffer, INT nCanvasW, INT nCanvasH, INT nPitch, INT nX, INT nY, void* lpFont, INT nCharW, INT nCharH)
{
	if( !lpFont )
		return;

	INT nClipX = 0;
	INT nClipY = 0;
	INT nClipWidth = nCharW;
	INT nClipHeight = nCharH;
	INT nClipRight = 0;

	INT nNextLine = nPitch - nClipWidth * 2;

	__asm
	{
		//---------------------------------------------------------------------------
		// 计算 EDI 指向屏幕起点的偏移量 (以字节计)
		// edi = nPitch * Clipper.y + nX * 2 + lpBuffer
		//---------------------------------------------------------------------------
		mov		eax, nPitch
			mov		ebx, nY
			mul		ebx
			mov     ebx, nX
			add		ebx, ebx
			add     eax, ebx
			mov 	edi, lpBuffer
			add		edi, eax
			//---------------------------------------------------------------------------
			// 初始化 ESI 指向图块数据起点 (跳过 Clipper.top 行图形数据)
			//---------------------------------------------------------------------------
			mov		esi, lpFont
			mov		ecx, nClipY
			or		ecx, ecx
			jz		loc_DrawFont_0011
			xor		eax, eax

loc_DrawFont_0008:

		mov		edx, nCharW

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

		mov		eax, nClipX
			or		eax, eax
			jz		loc_DrawFont_0012
			jmp		loc_DrawFont_exit

loc_DrawFont_0012:

		mov		eax, nClipRight
			or		eax, eax
			jz		loc_DrawFont_0100
			jmp		loc_DrawFont_exit
			//---------------------------------------------------------------------------
			// Clipper.left  == 0
			// Clipper.right == 0
			//---------------------------------------------------------------------------
loc_DrawFont_0100:

		mov		edx, nClipWidth

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
			dec		nClipHeight
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
			dec		nClipHeight
			jg		loc_DrawFont_0100
			jmp		loc_DrawFont_exit

DrawFrontWithBorder_DrawBorder:		//绘制字符的边缘
		mov		eax, nBorderColor
			rep		stosw

			sub		edx, ebx
			jg		loc_DrawFont_0101
			add		edi, nNextLine
			dec		nClipHeight
			jg		loc_DrawFont_0100
			jmp		loc_DrawFont_exit

loc_DrawFont_exit:
	}
}

KMedia* CreateFont(LPCSTR pszIdentifier, void* pArg)
{
	DWORD dwType = *(DWORD*)pArg;
	if( dwType == 0 )	//sysfont
	{
		//identifier format: "charset,typeface,height,weight,italic,antialiased,border_width"
		CHAR szIdentifier[256];
		strcpy( szIdentifier, pszIdentifier );

		LPSTR comma1 = strchr( szIdentifier, ',' );
		LPSTR comma2 = strchr( &comma1[1], ',' );
		if( !comma1 || !comma2 )
			return new FontMedia( "", 0, 0, 0, 0, 0, 0 );
		LPSTR comma3 = (comma2 && comma2[1]) ? strchr(&comma2[1], ',') : 0;
		LPSTR comma4 = (comma3 && comma3[1]) ? strchr(&comma3[1], ',') : 0;
		LPSTR comma5 = (comma4 && comma4[1]) ? strchr(&comma4[1], ',') : 0;
		LPSTR comma6 = (comma5 && comma5[1]) ? strchr(&comma5[1], ',') : 0;
		*comma1 = 0;
		*comma2 = 0;
		if( comma3 ) 
			*comma3 = 0;
		if( comma4 )
			*comma4 = 0;
		if( comma5 )
			*comma5 = 0;
		if( comma6 )
			*comma6 = 0;
		INT nHeight = atoi( &comma2[1] );
		DWORD dwCharset = (DWORD)atoi( szIdentifier );
		DWORD dwWeight = comma3 ? (DWORD)atoi(&comma3[1]) : FW_NORMAL;
		BOOL bItalic = comma4 ? (atoi(&comma4[1])!=0) : FALSE;
		BOOL bAntialiased = comma5 ? (atoi(&comma5[1])!=0) : FALSE;
		INT nBorderWidth = comma6 ? atoi(&comma6[1]) : 0;
		return new FontMedia( &comma1[1], nHeight, dwCharset, dwWeight, bItalic, bAntialiased, nBorderWidth );
	}
	else
	{
		return new FontMedia( pszIdentifier, dwType != 1 );	//1 for gb2312, else for big5
	}
}

BOOL InitFontCreator(void* pArg)
{
	FontInitParam* pParam = (FontInitParam*)pArg;
	g_sFontConfig.bSingleByteCharSet = pParam->bSingleByteCharSet;
	g_sFontConfig.bEnableConditionalNonPow2 = pParam->bEnableConditionalNonPow2;
	g_sFontConfig.dwFontTextureSize = pParam->dwFontTextureSize;

	vector<D3DFORMAT> vecFmt;
	vecFmt.push_back( D3DFMT_A8 );
	vecFmt.push_back( D3DFMT_A4R4G4B4 );
	g_sFontConfig.eFmt = FindFirstAvaiableOffscrTextureFormat( Gpu(), &vecFmt[0], (DWORD)vecFmt.size() );

	return g_sFontConfig.eFmt != D3DFMT_UNKNOWN;
}

void ShutdownFontCreator() 
{

}

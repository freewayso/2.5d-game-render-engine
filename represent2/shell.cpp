/* -------------------------------------------------------------------------
//	文件名		：	_shell_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-27 11:11:11
//	功能描述	：	shell的具体实现，整个渲染部分的外壳，对外提供各种功能接口
//
// -----------------------------------------------------------------------*/
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "../represent_common/vlist.h"
#include "../represent_common/timer.h"
#include "../represent_common/ifile.h"
#include "../represent_common/async_queue.h"
#include "../represent_common/loader.h"
#include "jpglib.h"
#include "blender.h"
#include "clipper.h"
#include "drawer.h"
#include "kernel_text.h"
#include "shell.h"

bool KShell::Create(HWND hWnd, INT nWidth, INT nHeight, bool& bFullScreen, const KRepresentConfig* pRepresentConfig)
{
	if( !bFullScreen )
	{
		RECT cRect = { 0, 0, nWidth, nHeight };

		SetWindowLongW(hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX);
		AdjustWindowRectEx( &cRect, GetWindowLongW(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL, GetWindowLongW(hWnd, GWL_EXSTYLE) );
		SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, cRect.right - cRect.left, cRect.bottom - cRect.top, SWP_NOACTIVATE );

		RECT cNewRect;
		GetClientRect( hWnd, &cNewRect );
		if( nWidth != cNewRect.right - cNewRect.left || nHeight != cNewRect.bottom - cNewRect.top )
		{
			bFullScreen = true;
		}
	}

	m_lpDD = 0;
	m_lpDDSPrimary = 0;
	m_lpDDSCanvas = 0;
	m_lpDDClipper = 0;

	//setup direct draw
	if( !SetupDirectDraw(hWnd, nWidth, nHeight, bFullScreen, &m_lpDD, &m_lpDDSPrimary, &m_lpDDSCanvas, &m_lpDDClipper, &m_cScreenSize.x, &m_cScreenSize.y, &m_wMask16, &m_dwMask32, &m_bFakeFullScreen) )
		return false;

	//save other parameters
	m_cCanvasSize = KPOINT(nWidth, nHeight);
	m_bFullScreen = bFullScreen && !m_bFakeFullScreen;
	m_hWnd = hWnd;
	m_bGpuLost = false;
	m_bRGB565 = ( m_wMask16 != 0x7fff );

	m_nFrameCounter = 0;
	m_nReplayerState = -1;

	//init jpglib.lib
	InitJpglib();

	//load engine.dll
	LoadEngineDll();

	//load config.ini
	LoadGlobalConfig(pRepresentConfig);

	//setup media center
	InitMediaCenter(g_cGlobalConfig.m_dwLoaderThreadCount, true);
	//spr
	KSprInitParam spr_init_param;
	spr_init_param.bRGB565 = m_bRGB565;
	spr_init_param.dwAlphaLevel = g_cGlobalConfig.m_dwAlphaLevel;
	g_dwMediaTypeSpr = InstallMediaType( InitSprCreator, &spr_init_param, CreateSpr, ShutdownSprCreator );
	//jpg
	KJpgInitParam jpg_init_param;
	jpg_init_param.m_bRgb565 = m_bRGB565;
	g_dwMediaTypeJpg = InstallMediaType( InitJpgCreator, &jpg_init_param, CreateJpg, ShutdownJpgCreator );
	//bitmap
	g_dwMediaTypeBitmap = InstallMediaType( InitBitmapCreator, 0, CreateBitmap, ShutdownBitmapCreator );
	//font
	FontInitParam font_init_param;
	font_init_param.hWnd = hWnd;
	font_init_param.bSingleByteCharSet = BOOL_bool(g_cGlobalConfig.m_bSingleByteCharSet);
	g_dwMediaTypeFont = InstallMediaType( InitFontCreator, &font_init_param, CreateFont, ShutdownFontCreator );

	//start timer
	StartTimer();

	//turn on font smoothing
	SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	ZeroMemory( &desc, sizeof( DDSURFACEDESC ) );

	return true;
}

bool KShell::Reset(INT nWidth, INT nHeight, bool bFullScreen, bool bNotAdjustStyle)
{
	KPOINT cOldCanvasSize = m_cCanvasSize;
	bool bOldFullscreen = m_bFullScreen || m_bFakeFullScreen;

	SAFE_RELEASE( m_lpDDClipper );
	SAFE_RELEASE( m_lpDDSCanvas );
	SAFE_RELEASE( m_lpDDSPrimary );
	SAFE_RELEASE( m_lpDD);

	//setup direct draw
	if( !SetupDirectDraw( m_hWnd, nWidth, nHeight, bFullScreen, &m_lpDD, &m_lpDDSPrimary, &m_lpDDSCanvas, &m_lpDDClipper, &m_cScreenSize.x, &m_cScreenSize.y, &m_wMask16, &m_dwMask32, &m_bFakeFullScreen ) )
	{
		SetupDirectDraw( m_hWnd, cOldCanvasSize.x, cOldCanvasSize.y, bOldFullscreen, &m_lpDD, &m_lpDDSPrimary, &m_lpDDSCanvas, &m_lpDDClipper, &m_cScreenSize.x, &m_cScreenSize.y, &m_wMask16, &m_dwMask32, &m_bFakeFullScreen );
		return false;
	}

	//save other parameters
	m_cCanvasSize = KPOINT(nWidth, nHeight);
	m_bFullScreen = bFullScreen && !m_bFakeFullScreen;
	m_bGpuLost = false;

	ZeroMemory( &desc, sizeof( DDSURFACEDESC ) );

	return true;
}

void KShell::Release()
{
	ShutdownMediaCenter();
	ReleaseEngineDll();
	ShutdownJpglib();

	SAFE_RELEASE( m_lpDDClipper );
	SAFE_RELEASE( m_lpDDSCanvas );
	SAFE_RELEASE( m_lpDDSPrimary );
	SAFE_RELEASE( m_lpDD);
}

bool KShell::RepresentBegin(INT bClear, DWORD dwColor)
{
	//update time
	UpdateTime();

	//handle device lost
	if( m_bGpuLost )
	{
		if( m_pJxPlayer && 
			( m_pJxPlayer->GetCurState() == enum_Replay_Playing ||
			m_pJxPlayer->GetCurState() == enum_Replay_PreDraw ||
			m_pJxPlayer->IsReplayPaused() ) )
			m_pJxPlayer->Stop();

		if( m_lpDDSPrimary->Restore() != DD_OK  || m_lpDDSCanvas->Restore() != DD_OK )
		{
			if( !m_bFullScreen )
			{
				DDSURFACEDESC ddsd;
				ZeroMemory( &ddsd, sizeof(ddsd) );
				ddsd.dwSize = sizeof(ddsd);
				ddsd.dwFlags = DDSD_ALL;
				m_lpDD->GetDisplayMode( &ddsd );
				if( ddsd.ddpfPixelFormat.dwRGBBitCount != 15 && ddsd.ddpfPixelFormat.dwRGBBitCount != 16 )
				{
					m_lpDD->SetDisplayMode( ddsd.dwWidth, ddsd.dwHeight, 16 );
				}
			}
			return false;
		}

		m_bGpuLost = false;
	}

	//clear canvas
	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = 0;
	m_lpDDSCanvas->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);

	return true;//Lock( lpDDSCanvas );
}

void KShell::RepresentEnd() 
{
	if( g_cGlobalConfig.m_bShowFps )
	{
		//performance monitor
		static DWORD s_dwLastTime = NowTime();
		static DWORD s_dwFrameElapsed = 0;
		static WCHAR s_wchMsg[256] = L"";

		UpdateTime();
		s_dwFrameElapsed++;
		if( NowTime() - s_dwLastTime > 1000 )
		{
			DWORD dwFps = 1000 * s_dwFrameElapsed / (NowTime() - s_dwLastTime);
			DWORD dwTime = (NowTime() - g_vecMediaCasing[g_vlstMediaPriorityList.Back()].m_dwLastQueryTime) / 1000;
			DWORD dwSpace = g_dwMediaBytes/1024/1024;

			s_dwLastTime = NowTime();
			s_dwFrameElapsed = 0;

			swprintf(s_wchMsg, L"FPS:%d TIME:%d SPACE:%d", dwFps, dwTime, dwSpace);
		}
		OutputText(12, s_wchMsg, -1, 30, 42, 0xff00ff00);
	}

	//Unlock( lpDDSCanvas );
	//present
	if( m_bFullScreen )
	{
		if( DDERR_SURFACELOST == m_lpDDSPrimary->Blt( NULL, m_lpDDSCanvas, NULL, DDBLT_WAIT, NULL ) )
		{
			m_bGpuLost = true;
		}
	}
	else
	{
		RECT cClientRect = { 0, 0, m_cCanvasSize.x, m_cCanvasSize.y };
		ClientToScreen( m_hWnd, (LPPOINT)&cClientRect );
		ClientToScreen( m_hWnd, (LPPOINT)&cClientRect + 1 );

		RECT cScreenRect = {0, 0, m_cScreenSize.x, m_cScreenSize.y};

		RECT cDestRect;
		if( RectIntersect( &cClientRect, &cScreenRect, &cDestRect ) )
		{
			RECT cSrcRect = {	cDestRect.left - cClientRect.left, 
				cDestRect.top - cClientRect.top,
				cDestRect.right - cClientRect.left,
				cDestRect.bottom - cClientRect.top	};

			if( DDERR_SURFACELOST == m_lpDDSPrimary->Blt( &cDestRect, m_lpDDSCanvas, &cSrcRect, DDBLT_WAIT, NULL ) )
			{
				m_bGpuLost = true;
			}
		}
	}

	//garbage collection
	CollectGarbage( g_cGlobalConfig.m_dwMediaTimeLimit*1000, g_cGlobalConfig.m_dwMediaSpaceLimit*1024*1024 );
	KMedia::ResetTaskCount();
}

bool KShell::CreateAFont(LPCSTR pszFontFile, DWORD CharaSet, INT nId)
{
	if(pszFontFile[0] == '#')
	{
		INT nSharedId = atoi(&pszFontFile[1]);
		//add nId
		if(nSharedId != nId)
		{
			map<INT, DWORD>::iterator p = m_mapFont.find(nSharedId);
			if( p == m_mapFont.end() ) 
				return false;
			m_mapFont[nId] = p->second;
			return true;
		}
	}

	if(pszFontFile[0] == '@')
	{
		char* pchNowCheck = (char*)&pszFontFile[1];
		while( true )
		{
			char szFace[256];
			INT i = 0;
			while(pchNowCheck[i] != '@' && pchNowCheck[i] != ',' && pchNowCheck[i] != ' ' && pchNowCheck[i] != 0)
			{
				szFace[i] = pchNowCheck[i];
				i++;
			}
			szFace[i] = 0;

			if( CheckFont( szFace, CharaSet, m_hWnd ) )
				break;

			char* a = strchr(pchNowCheck, '@');
			if( !a )
			{
				pchNowCheck = 0;
				break;
			}

			pchNowCheck = &a[1];
		}

		if( pchNowCheck == 0 )
			pchNowCheck = (char*)&pszFontFile[1];

		char c[256];
		INT i = 0;
		while(pchNowCheck[i] != '@' && pchNowCheck[i] != 0)
		{
			c[i] = pchNowCheck[i];
			i++;
		}
		c[i] = 0;

		char _identifier[256];
		sprintf(_identifier, "%d,", CharaSet);
		strcat(_identifier, c);
		DWORD fonttype = 0;	//sysfont
		m_mapFont[nId] = FindOrCreateMedia(g_dwMediaTypeFont, _identifier, &fonttype);
		return true;
	}

	DWORD dwFontType = (CharaSet == 3) ? 2 : 1;
	m_mapFont[nId] = FindOrCreateMedia(g_dwMediaTypeFont, pszFontFile, &dwFontType);
	return true;
}

//modify by wdb, 改造支持unicode显示
void KShell::OutputText(INT nFontId, const WCHAR* psText, INT nCount, INT nX, INT nY, DWORD Color, INT nLineWidth, INT nZ, DWORD BorderColor, const RECT* pClipRect)
{
	if(!psText || !psText[0]) 
		return;

	map<INT, DWORD>::iterator p = m_mapFont.find(nFontId);
	if( p == m_mapFont.end() )
		return;

	if(NULL != m_pJxPlayer && 
		(m_pJxPlayer->GetCurState() == enum_Replay_Recing || 
		m_pJxPlayer->GetCurState() == -1))
	{
		std::string strText;
		INT nLen = JXTextRender::ConvertEncodeBuffUnicodeToCurLang(psText, nCount, strText);
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_OutPutText, 
			"npcxyolzb",
			nFontId, strText.c_str(), 
			nLen, nX, nY, 
			Color, nLineWidth, 
			nZ, BorderColor);
	}

	KTextCmd cmd;
	cmd.dwFontHandle = p->second;
	cmd.dwOptions = 0;

	//modify by wdb
	//	cmd.text = (byte*)psText;
	cmd.pwchText = psText;
	cmd.dwBytes = (DWORD)((nCount >= 0) ? nCount : UINT_MAX);
	cmd.nLineWidthLimit = (nLineWidth > 0) ? nLineWidth : INT_MAX;
	cmd.dwStartLine = 0;
	cmd.dwEndLine = UINT_MAX;
	cmd.nLineInterSpace = nFontId;
	cmd.nCharInterSpace = nFontId;
	cmd.dwColor = Color;	//alpha channel is ignored
	cmd.dwBorderColor = BorderColor;
	cmd.fInlinePicScaling = 1;
	cmd.nFontId = nFontId;

	if(nZ == -32767)	// TEXT_IN_SINGLE_PLANE_COORD
	{
		cmd.cPos = KPOINT(nX, nY);
	}
	else
	{
		cmd.cPos = KPOINT(nX, nY);
		SpaceToScreen(cmd.cPos.x, cmd.cPos.y, nZ);
	}

	if( pClipRect != NULL )
	{
		RECT cCanvasRect = {0, 0, m_cCanvasSize.x, m_cCanvasSize.y};
		if( !RectIntersect(&cCanvasRect, pClipRect, &g_sScissorRect) )
		{
			return;
		}

		g_bScissorTestEnable = true;
	}

	WriteText( m_bRGB565, &cmd, m_lpDDSCanvas );

	if( pClipRect != NULL )
	{
		g_bScissorTestEnable = false;
	}
}

//改造成支持unicode显示,要求外面传入的为unicode字符
INT KShell::OutputRichText(INT nFontId, void* pParam, const WCHAR* psText, INT nCount, INT nLineWidth, const RECT* pClipRect)
{
	if(!pParam || !psText || !psText[0]) 
		return 0;

	map<INT, DWORD>::iterator p = m_mapFont.find(nFontId);	// <-- use priority list instead?
	if( p == m_mapFont.end() )
		return 0;

	if(NULL != m_pJxPlayer && 
		(m_pJxPlayer->GetCurState() == enum_Replay_Recing || 
		m_pJxPlayer->GetCurState() == -1))
	{
		std::string strText;
		INT nLen = JXTextRender::ConvertEncodeBuffUnicodeToCurLang(psText, nCount, strText);
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_OutPutRichText, 
			"npscl", nFontId, pParam, 
			strText.c_str(), nLen, nLineWidth);
	}

	struct KOutputTextParam
	{
		INT				nX;
		INT				nY;
		INT				nZ;
		short 			nSkipLine;
		short 			nNumLine;
		DWORD			Color;
		DWORD			BorderColor;
		WORD			nVertAlign;
		WORD			nHoriAlign;
		INT				nHoriLen;
		INT				bPicPackInSingleLine;
		INT				nPicStretchPercent;
		INT				nRowSpacing;
	};
	KOutputTextParam* pOutputParam = (KOutputTextParam*)pParam;

	KTextCmd cmd;
	cmd.dwFontHandle = p->second;
	cmd.dwOptions = emTEXT_CMD_COLOR_TAG | emTEXT_CMD_INLINE_PICTURE | emTEXT_CMD_UNDERLINE_TAG;

	//modify by wdb, 改成unicode显示，要求外面传入的是unicode字符
	//	cmd.text = (byte*)psText;
	cmd.pwchText = psText;
	cmd.dwBytes = (DWORD)( (nCount > 0) ? nCount : UINT_MAX );	
	cmd.nLineWidthLimit = (nLineWidth > 0) ? nLineWidth : INT_MAX;
	cmd.dwStartLine = pOutputParam->nSkipLine;
	cmd.dwEndLine = pOutputParam->nSkipLine + pOutputParam->nNumLine;
	cmd.nLineInterSpace = nFontId + pOutputParam->nRowSpacing;
	cmd.nCharInterSpace = nFontId;
	cmd.dwColor = pOutputParam->Color;
	cmd.dwBorderColor = pOutputParam->BorderColor;
	cmd.fInlinePicScaling = pOutputParam->nPicStretchPercent/100.f;
	cmd.nFontId = nFontId;

	if( pOutputParam->nZ == -32767 )	// TEXT_IN_SINGLE_PLANE_COORD
	{
		cmd.cPos = KPOINT(pOutputParam->nX, pOutputParam->nY);
	}
	else
	{
		cmd.cPos = KPOINT(pOutputParam->nX, pOutputParam->nY);
		SpaceToScreen(cmd.cPos.x, cmd.cPos.y, pOutputParam->nZ);
	}	

	if( pClipRect != NULL )
	{
		RECT canvas_rect = {0, 0, m_cCanvasSize.x, m_cCanvasSize.y};
		if( !RectIntersect(&canvas_rect, pClipRect, &g_sScissorRect) )
		{
			return 0;
		}

		g_bScissorTestEnable = true;
	}

	WriteText( m_bRGB565, &cmd, m_lpDDSCanvas );

	if( pClipRect != NULL )
	{
		g_bScissorTestEnable = false;
	}

	return 0;
}

//image info

bool KShell::GetImageParam(LPCSTR pszImage, void* pImageData, DWORD nType, bool bNoCache)
{
	if( bNoCache )
	{
		//only support .spr now
		if( nType != emPIC_TYPE_SPR )
			return false;

		LPD3DBUFFER pFiledata = 0;
		if( !LoadFile(pszImage, &pFiledata) || !pFiledata )
			return false;

		struct KHead
		{
			BYTE	byComment[4];	
			WORD	wWidth;		
			WORD	wHeight;		
			WORD	wCenterX;	
			WORD	wCenterY;	
			WORD	wFrameCount;
			WORD	wPalEntryCount;
			WORD	wDirectionCount;
			WORD	wInterval;
			WORD	wReserved[6];
		};

		//check size for head
		if( pFiledata->GetBufferSize() < sizeof(KHead) )
		{
			SAFE_RELEASE( pFiledata );
			return false;
		}

		//load head info
		KHead* pHead = (KHead*)pFiledata->GetBufferPointer();

	struct KInfoRet
	{
		short shFrameCount;
		short shInterval;
		short shWidth;
		short shHeight;
		short shCenterX;
		short shCenterY;
		short shDirectionCount;
		BYTE  byBlendStyle;
	};
		KInfoRet* pSprInfo = (KInfoRet*)pImageData;
		if( pSprInfo )
		{
			pSprInfo->shFrameCount		= (short)pHead->wFrameCount;
			pSprInfo->shInterval			= (short)pHead->wInterval;
			pSprInfo->shWidth				= (short)pHead->wWidth;
			pSprInfo->shHeight			= (short)pHead->wHeight;
			pSprInfo->shCenterX			= (short)pHead->wCenterX;
			pSprInfo->shCenterY			= (short)pHead->wCenterY;
			pSprInfo->shDirectionCount	= (short)pHead->wDirectionCount;
			pSprInfo->byBlendStyle		= (BYTE)(pHead->wReserved[1] & 0xff);
		}

		SAFE_RELEASE(pFiledata);
		return true;
	}

	struct KInfoRet
	{
		short shFrameCount;
		short shInterval;
		short shWidth;
		short shHeight;
		short shCenterX;
		short shCenterY;
		short shDirectionCount;
		BYTE  byBlendStyle;
	};
	switch( nType )
	{
	case emPIC_TYPE_SPR:
		{
			DWORD handle = FindOrCreateMedia( g_dwMediaTypeSpr, pszImage, 0 );

			KSprite* spr = 0;
			if( !QueryProduct(handle, true, (void**)&spr) || !spr )
				return false;

			KInfoRet* pSprInfo		= (KInfoRet*)pImageData;
			pSprInfo->shFrameCount		= (short)spr->GetFrameCount();
			pSprInfo->shInterval			= (short)spr->GetInterval();
			pSprInfo->shWidth				= (short)spr->GetSize()->x;
			pSprInfo->shHeight			= (short)spr->GetSize()->y;
			pSprInfo->shCenterX			= (short)spr->GetCenter()->x;
			pSprInfo->shCenterY			= (short)spr->GetCenter()->y;
			pSprInfo->shDirectionCount	= (short)spr->GetDirectionCount();
			pSprInfo->byBlendStyle		= (BYTE)spr->GetBlendStyle();

			return true;
		}
		break;
	case emPIC_TYPE_JPG_OR_LOCKABLE:
	case emPIC_TYPE_RT:
		{
			DWORD dwHandle;
			if( !(dwHandle = FindMedia( g_dwMediaTypeBitmap, pszImage )) )
			{
				dwHandle = FindOrCreateMedia( g_dwMediaTypeJpg, pszImage, 0 );
			}

			if( !dwHandle )
				return false;

			IKBitmap_t* pBmp = 0;
			if( !QueryProduct(dwHandle, true, (void**)&pBmp) || !pBmp )
				return false;

			KInfoRet* SprInfo		= (KInfoRet*)pImageData;
			SprInfo->shFrameCount		= 1;
			SprInfo->shInterval			= 0;
			SprInfo->shWidth				= (short)pBmp->GetSize()->x;
			SprInfo->shHeight			= (short)pBmp->GetSize()->y;
			SprInfo->shCenterX			= 0;
			SprInfo->shCenterY			= 0;
			SprInfo->shDirectionCount	= 1;
			SprInfo->byBlendStyle		= 0;

			return true;
		}
		break;
	}

	return false;
}

bool KShell::GetImageFrameParam(LPCSTR pszImage, INT nFrame, KPOINT* pOffset, KPOINT* pSize, DWORD nType, bool bNoCache) 
{
	if( bNoCache )
	{
		//only support .spr now
		if( nType != emPIC_TYPE_SPR )
			return false;

		if( nFrame < 0 )
			return false;

		LPD3DBUFFER	pFiledata = 0;
		bool		bPacked;

		IFile* pFile = g_pfnOpenFile(pszImage, false, false);
		if( !pFile )
			return false;
		if( pFile->IsPackedByFragment() )
		{
			bPacked = true;
			LoadFileFragment(pFile, nFrame+2, &pFiledata);
		}
		else
		{
			bPacked = false;
			LoadFile(pFile, &pFiledata);
		}
		SAFE_RELEASE(pFile);

		if( !pFiledata )
			return false;

		struct KFrameInfo
		{
			WORD	wWidth;
			WORD	wHeight;
			WORD	wOffsetX;
			WORD	wOffsetY;
		};

		KFrameInfo* pFrameInfo = 0;
		if( bPacked )
		{
			if( pFiledata->GetBufferSize() < sizeof(KFrameInfo) )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}
			pFrameInfo = (KFrameInfo*)pFiledata->GetBufferPointer();
		}
		else
		{
			struct KHead
			{
				BYTE	bComment[4];	
				WORD	wWidth;		
				WORD	wHeight;		
				WORD	wCenterX;	
				WORD	wCenterY;	
				WORD	wFrameCount;
				WORD	wPalEntryCount;
				WORD	wDirectionCount;
				WORD	wInterval;
				WORD	wReserved[6];
			};

			struct KOffsetTable
			{
				DWORD	dwOffset;
				DWORD	dwSize;
			};

			if( pFiledata->GetBufferSize() < sizeof(KHead) )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}
			KHead* pHead = (KHead*)pFiledata->GetBufferPointer();

			if( nFrame >= pHead->wFrameCount )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}

			if( pFiledata->GetBufferSize() < sizeof(KHead) + 3*256 + sizeof(KOffsetTable)*pHead->wFrameCount )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}
			KOffsetTable* offtbl = (KOffsetTable*)(((BYTE*)pFiledata->GetBufferPointer()) + sizeof(KHead) + 3*256);

			if( offtbl[nFrame].dwSize < sizeof(KFrameInfo) )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}

			if( pFiledata->GetBufferSize() < sizeof(KHead) + 3*256 + sizeof(KOffsetTable)*pHead->wFrameCount + offtbl[nFrame].dwOffset + offtbl[nFrame].dwSize )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}

			pFrameInfo = (KFrameInfo*)(((BYTE*)pFiledata->GetBufferPointer()) + sizeof(KHead) + 3*256 + sizeof(KOffsetTable)*pHead->wFrameCount + offtbl[nFrame].dwOffset);
		}

		if( pOffset )
			*pOffset = KPOINT(pFrameInfo->wOffsetX, pFrameInfo->wOffsetY);
		if( pSize )
			*pSize = KPOINT(pFrameInfo->wWidth, pFrameInfo->wHeight);

		SAFE_RELEASE(pFiledata);
		return true;
	}

	switch( nType )
	{
	case emPIC_TYPE_SPR:
		{
			DWORD dwHandle = FindOrCreateMedia( g_dwMediaTypeSpr, pszImage, 0 );

			KSprite* spr = 0;
			if( !QueryProduct(dwHandle, true, (void**)&spr) || !spr )
				return false;

			KSpriteFrame* frame = 0;
			if( !spr->QueryFrame(nFrame, true, (void**)&frame) || !frame )
				return false;

			*pOffset = KPOINT(frame->GetFrameOffset()->x, frame->GetFrameOffset()->y);
			*pSize = KPOINT(frame->GetFrameSize()->x, frame->GetFrameSize()->y);

			return true;
		}
		break;
	case emPIC_TYPE_JPG_OR_LOCKABLE:
	case emPIC_TYPE_RT:
		{
			DWORD dwHandle;
			if( !(dwHandle = FindMedia( g_dwMediaTypeBitmap, pszImage )) )
			{
				dwHandle = FindOrCreateMedia( g_dwMediaTypeJpg, pszImage, 0 );
			}

			if( !dwHandle )
				return false;

			IKBitmap_t* bmp = 0;
			if( !QueryProduct(dwHandle, true, (void**)&bmp) || !bmp )
				return false;

			*pOffset = KPOINT(0, 0);
			*pSize = KPOINT(bmp->GetSize()->x, bmp->GetSize()->y);

			return true;
		}
		break;
	}
	return false;
}

INT KShell::GetImagePixelAlpha(LPCSTR pszImage, INT nFrame, INT nX, INT nY, INT nType)
{
	switch( nType )
	{
	case emPIC_TYPE_SPR:
		{
			DWORD dwHandle = FindOrCreateMedia( g_dwMediaTypeSpr, pszImage, 0 );
			if( !dwHandle )
				return 0;

			KSprite* pSpr = 0;
			if( !QueryProduct( dwHandle, true, (void**)&pSpr ) || !pSpr )
				return 0;

			KSpriteFrame* pFrame = 0;
			if( !pSpr->QueryFrame( nFrame, true, (void**)&pFrame ) || !pFrame )
				return 0;

			nX -= pFrame->GetFrameOffset()->x;
			nY -= pFrame->GetFrameOffset()->y;

			if( nX < 0 || nX >= pFrame->GetFrameSize()->x || nY < 0 || nY >= pFrame->GetFrameSize()->y )
				return 0;

			INT	nNumPixels = pFrame->GetFrameSize()->x;
			void* pSprite =  pFrame->GetFrameData();
			nY++;

			INT  nRet = 0;
			_asm
			{
				//使SDI指向sprite中的图形数据位置
				mov		esi, pSprite
dec_line:
				dec		nY				//减掉一行
					jz		last_line

					mov		edx, nNumPixels
skip_line:
				movzx	eax, BYTE ptr[esi]
				inc		esi
					movzx	ebx, BYTE ptr[esi]
				inc		esi
					or		ebx, ebx
					jz		skip_line_continue
					add		esi, eax
skip_line_continue:
				sub		edx, eax
					jg		skip_line
					jmp		dec_line

last_line:
				mov		edx, nX
last_line_alpha_block:
				movzx	eax, BYTE ptr[esi]
				inc		esi
					movzx	ebx, BYTE ptr[esi]
				inc		esi
					or		ebx, ebx
					jz		last_line_continue
					add		esi, eax
last_line_continue:
				sub		edx, eax
					jg		last_line_alpha_block

					mov		nRet, ebx
			}

			return nRet;
		}
		break;
	case emPIC_TYPE_JPG_OR_LOCKABLE:
		{
			DWORD wHandle = FindOrCreateMedia( g_dwMediaTypeJpg, pszImage, 0 );
			if( !wHandle )
				return 0;

			IKBitmap_t* pBmp = 0;
			if( !QueryProduct(wHandle, true, (void**)&pBmp) || !pBmp )
				return 0;

			if( nX < 0 || nX >= pBmp->GetSize()->x || nY < 0 || nY >= pBmp->GetSize()->y )
				return 0;

			return 255;
		}
		break;
	}

	return 0;
}

//dynamic bitmap (当创建的类型是SPR时，将不会被回收)
DWORD KShell::CreateImage(LPCSTR pszName, INT nWidth, INT nHeight, DWORD nType)
{
	if (m_pJxPlayer != NULL)
	{
		m_pJxPlayer->RECjxr(m_nFrameCounter, enum_RepreFun_CreateImage, 
			"pnwh", pszName, nType, nWidth, nHeight);
	}

	KCreateBitmapParam param;
	param.m_dwFormat = nType;
	param.m_pSize = KPOINT(nWidth, nHeight);
	switch( nType )
	{
		case emPIC_TYPE_SPR:
			return FindOrCreateMedia( g_dwMediaTypeBitmap, pszName, &param, 1);
		default:
			return FindOrCreateMedia( g_dwMediaTypeBitmap, pszName, &param );
	}
}

void KShell::ClearImageData(LPCSTR pszImage, DWORD uImage, DWORD nImagePosition)
{
	// 	if(NULL != m_pJxPlayer && 
	// 		(m_pJxPlayer->GetCurState() == enum_Replay_Recing || 
	// 		 m_pJxPlayer->GetCurState() == enum_Replay_PauseRec || 
	// 		 m_pJxPlayer->GetCurState() == -1))
	if(NULL != m_pJxPlayer)
	{
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_ClearImageData, 
			"pun", 
			pszImage, 
			uImage, 
			nImagePosition);
	}

	DWORD dwHandle = 0;
	if( CheckHandle( uImage ) == g_dwMediaTypeBitmap )
		dwHandle = uImage;
	else if( CheckHandle( nImagePosition ) == g_dwMediaTypeBitmap )
		dwHandle = nImagePosition;
	else dwHandle = FindMedia( g_dwMediaTypeBitmap, pszImage );

	if( !dwHandle )
		return;

	IKBitmap_t* pBmp = 0;
	if( !QueryProduct( dwHandle, true, (void**)&pBmp ) || !pBmp )
		return;

	switch( pBmp->GetFormat() )
	{
	case emPIC_TYPE_JPG_OR_LOCKABLE:
		memset( pBmp->GetData(), 0, pBmp->GetSize()->x * pBmp->GetSize()->y * 2 );
		break;
	case emPIC_TYPE_RT:
		{
			WORD* p = (WORD*)pBmp->GetData();
			INT n = pBmp->GetSize()->x * pBmp->GetSize()->y;
			for( INT  i = 0; i < n; i++ )
			{
				p[i] = 0x8000;
			}
		}
		break;
	}
}

void* KShell::GetBitmapDataBuffer(LPCSTR pszImage, void* pInfo)
{
	DWORD dwHandle = FindMedia( g_dwMediaTypeBitmap, pszImage );
	if( !dwHandle )
		return 0;

	IKBitmap_t* pBmp = 0;
	if( !QueryProduct(dwHandle, true, (void**)&pBmp) || !pBmp )
		return 0;

	struct KBitmapDataBuffInfo
	{
		INT		nWidth;
		INT		nHeight;
		INT		nPitch;	
		DWORD	eFormat;
		void*	pData;
	}; 

	if( pInfo )
	{
		KBitmapDataBuffInfo* out = (KBitmapDataBuffInfo*)pInfo;
		out->nWidth = pBmp->GetSize()->x;
		out->nHeight = pBmp->GetSize()->y;
		out->nPitch = pBmp->GetSize()->x * 2;
		out->pData = pBmp->GetData();
		out->eFormat = m_bRGB565 ? 2 : 1;
	}

	return pBmp->GetData();
}

void KShell::ReleaseBitmapDataBuffer(LPCSTR pszImage, void* pBuffer) {}

void KShell::DrawPrimitives(INT nPrimitiveCount, void* pPrimitives, DWORD uGenre, INT bSinglePlaneCoord, KPRIMITIVE_INDEX_LIST** pStandBy) 
{
	if(NULL != m_pJxPlayer && 
		(m_pJxPlayer->GetCurState() == enum_Replay_Recing ||
		m_pJxPlayer->GetCurState() == -1))
	{
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_DrawPrimitives, 
			"npub", 
			nPrimitiveCount, 
			pPrimitives, 
			uGenre, 
			bSinglePlaneCoord);
	}

	struct KPointCmd
	{
		D3DXVECTOR3	sPos;
		DWORD	wColor;
	};

	struct KLineCmd
	{
		D3DXVECTOR3	sPos1;
		D3DXVECTOR3	sPos2;
		DWORD	dwColor1;
		DWORD	dwColor2;
	};

	struct KRectFrameCmd
	{
		D3DXVECTOR3	sPos1;
		D3DXVECTOR3	sPos2;
		DWORD	dwColor1;
		DWORD	dwColor2;
	};

	struct KRectCmd
	{
		D3DXVECTOR3	sPos1;
		D3DXVECTOR3	sPos2;
		DWORD	wColor;
	};

	switch( uGenre )
	{
	case emPRIMITIVE_TYPE_POINT:
		{
			KPointCmd* pCmds = (KPointCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				INT x = (INT)pCmds[i].sPos.x;
				INT y = (INT)pCmds[i].sPos.y;
				if( !bSinglePlaneCoord )
				{
					SpaceToScreen(x, y, (INT)pCmds[i].sPos.z);
				}

				DDSURFACEDESC desc;
				desc.dwSize = sizeof(desc);
				if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
				{
					DrawPixel(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, x, y, Color32To16(pCmds[i].wColor, m_bRGB565));
					m_lpDDSCanvas->Unlock( NULL );
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_LINE:
		{
			KLineCmd* cmds = (KLineCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				INT	nX1 = (INT)cmds[i].sPos1.x;
				INT nY1 = (INT)cmds[i].sPos1.y;

				INT nX2 = (INT)cmds[i].sPos2.x;
				INT nY2 = (INT)cmds[i].sPos2.y;

				if( !bSinglePlaneCoord )
				{
					SpaceToScreen(nX1, nY1, (INT)cmds[i].sPos1.z);
					SpaceToScreen(nX2, nY2, (INT)cmds[i].sPos2.z);
				}

				if( (cmds[i].dwColor1 >> 24) >= 248 )
				{
					DDSURFACEDESC desc;
					desc.dwSize = sizeof(desc);
					if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
					{
						DrawLine(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX1, nY1, nX2, nY2, Color32To16(cmds[i].dwColor1, m_bRGB565));
						m_lpDDSCanvas->Unlock( NULL );
					}
				}
				else if( (cmds[i].dwColor1 >> 24) >= 8 )
				{
					DDSURFACEDESC desc;
					desc.dwSize = sizeof(desc);
					if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
					{
						DrawLineAlpha(m_dwMask32, desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX1, nY1, nX2, nY2, Color32To16(cmds[i].dwColor1, m_bRGB565), 31 - (cmds[i].dwColor1 >> 24) / 8);
						m_lpDDSCanvas->Unlock( NULL );
					}
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_RECT_FRAME:
		{
			KRectFrameCmd* pCmds = (KRectFrameCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				INT	nX1 = (INT)pCmds[i].sPos1.x;
				INT nY1 = (INT)pCmds[i].sPos1.y;

				INT nX2 = (INT)pCmds[i].sPos2.x;
				INT nY2 = (INT)pCmds[i].sPos2.y;

				if( !bSinglePlaneCoord )
				{
					SpaceToScreen(nX1, nY1, (INT)pCmds[i].sPos1.z);
					SpaceToScreen(nX2, nY2, (INT)pCmds[i].sPos2.z);
				}

				DDSURFACEDESC desc;
				desc.dwSize = sizeof(desc);
				if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
				{
					WORD color = Color32To16(pCmds[i].dwColor1, m_bRGB565);
					DrawLine(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX1, nY1, nX2, nY1, color);
					DrawLine(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX1, nY2, nX2, nY2, color);
					DrawLine(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX1, nY1, nX1, nY2, color);
					DrawLine(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX2, nY1, nX2, nY2, color);
					m_lpDDSCanvas->Unlock( NULL );
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_RECT:
		{
			KRectCmd* pCmds = (KRectCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				INT	nX1 = (INT)pCmds[i].sPos1.x;
				INT nY1 = (INT)pCmds[i].sPos1.y;

				INT nX2 = (INT)pCmds[i].sPos2.x;
				INT nY2 = (INT)pCmds[i].sPos2.y;

				if( !bSinglePlaneCoord )
				{
					SpaceToScreen(nX1, nY1, (INT)pCmds[i].sPos1.z);
					SpaceToScreen(nX2, nY2, (INT)pCmds[i].sPos2.z);
				}

				DDSURFACEDESC desc;
				desc.dwSize = sizeof(desc);
				if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
				{
					DrawRectAlpha(m_dwMask32, desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, nX1, nY1, nX2 - nX1, nY2 - nY1, Color32To16(pCmds[i].wColor, m_bRGB565), pCmds[i].wColor >> 24);
					m_lpDDSCanvas->Unlock( NULL );
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_PIC:
	case emPRIMITIVE_TYPE_PIC_PART:
	case emPRIMITIVE_TYPE_PIC_PART_FOUR:
		{
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				KPicCmd* pCmd;
				switch( uGenre )
				{
				case emPRIMITIVE_TYPE_PIC:
					pCmd = &((KPicCmd*)pPrimitives)[i];
					break;
				case emPRIMITIVE_TYPE_PIC_PART:
					pCmd = &((KPicCmdPart*)pPrimitives)[i];
					break;
				case emPRIMITIVE_TYPE_PIC_PART_FOUR:
					pCmd = &((KPicCmdPartFour*)pPrimitives)[i];
					break;
				}

				if( !pCmd->szFileName || !pCmd->szFileName[0] )
					continue;

				switch( pCmd->dwType )
				{
				case emPIC_TYPE_SPR:	
					{
						//correct handle
						if( CheckHandle(pCmd->dwHandle1) != g_dwMediaTypeSpr )
						{
							pCmd->dwHandle1 = pCmd->dwHandle2 = FindOrCreateMedia( g_dwMediaTypeSpr, pCmd->szFileName, 0 );
						}

						KSprite* pSpr = 0;
						KSpriteFrame* pFrame = 0;

						//query spr
						if( QueryProduct(pCmd->dwHandle1, g_bForceDrawPic, (void**)&pSpr) )
						{
							if( !pSpr )
								break;

							if( pCmd->wFrameIndex < 0 || pCmd->wFrameIndex >= (short)pSpr->GetFrameCount() )
								break;

							if( pSpr->QueryFrame(pCmd->wFrameIndex, g_bForceDrawPic, (void**)&pFrame) )
							{
								if( !pFrame )
									break;
							}
						}

						INT nRealRenderStyle = pCmd->byRenderStyle;
						if( !pFrame )
						{
							//use stand by list
							KPRIMITIVE_INDEX_LIST* pNode = pStandBy ? pStandBy[i] : 0;
							while( pNode )
							{
								if( (CheckHandle(pNode->sIndex.uImage) == g_dwMediaTypeSpr) && 
									QueryProduct(pNode->sIndex.uImage, g_bForceDrawPic, (void**)&pSpr) && pSpr &&
									(pNode->sIndex.nFrame >= 0) && (pNode->sIndex.nFrame < (INT)pSpr->GetFrameCount()) &&
									pSpr->QueryFrame(pNode->sIndex.nFrame, g_bForceDrawPic, (void**)&pFrame) && pFrame )
								{
									if( pNode->sIndex.nRenderStyle != 0 )
										nRealRenderStyle = pNode->sIndex.nRenderStyle;
									break;
								}
								pNode = pNode->pNext;
							}
						}

						// 渲染备用资源
						if( !pFrame )
						{	
							DWORD dwHandle;
							if(!(dwHandle = FindMedia(g_dwMediaTypeSpr, pCmd->szSubstrituteFileName)))
							{
								// 这里应该一早就被创建好，如果找不到。则是创建有问题
								return;
							}

							if(QueryProduct(dwHandle, 1, (void**)&pSpr))
							{
								if( !pSpr )
									break;

								if( pSpr->QueryFrame((WORD)pSpr->GetFrameCount(), g_bForceDrawPic, (void**)&pFrame) )
								{
									if( !pFrame )
										break;
								}
							}
						}


						if( !pFrame )
							break;

						//eval pos
						D3DXVECTOR3 sPosSaved;
						if( !bSinglePlaneCoord )
						{
							sPosSaved = pCmd->sPos1;
							KPOINT out;
							out.x = (INT)pCmd->sPos1.x;
							out.y = (INT)pCmd->sPos1.y;
							SpaceToScreen(out.x, out.y, (INT)pCmd->sPos1.z);
							pCmd->sPos1.x = (float)out.x;
							pCmd->sPos1.y = (float)out.y;
						}

						KPOINT pos;
						switch( pCmd->byRefMethod )
						{
						case emREF_METHOD_TOP_LEFT:
							pos.x = (INT)pCmd->sPos1.x + pFrame->GetFrameOffset()->x;
							pos.y = (INT)pCmd->sPos1.y + pFrame->GetFrameOffset()->y;
							break;
						case emREF_METHOD_CENTER:
							{
								KPOINT center = *pSpr->GetCenter();
								if( center == KPOINT(0, 0) && pSpr->GetSize()->x > 160 )
								{
									center = KPOINT(160, 208);
								}
								pos.x = (INT)pCmd->sPos1.x - center.x + pFrame->GetFrameOffset()->x;
								pos.y = (INT)pCmd->sPos1.y - center.y + pFrame->GetFrameOffset()->y;
							}
							break;
						case emREF_METHOD_FRAME_TOP_LEFT:
							pos.x = (INT)pCmd->sPos1.x;
							pos.y = (INT)pCmd->sPos1.y;
							break;
						default:
							pos.x = (INT)pCmd->sPos1.x;
							pos.y = (INT)pCmd->sPos1.y;
							break;
						}

						if( !bSinglePlaneCoord )
						{
							pCmd->sPos1 = sPosSaved;
						}

						//src_rect
						RECT cSrcRect;
						if( uGenre == emPRIMITIVE_TYPE_PIC )
						{
							SetRect( &cSrcRect, 0, 0, pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y );
						}
						else
						{
							KPicCmdPart* pCmdEx = (KPicCmdPart*)pCmd;
							SetRect( &cSrcRect, pCmdEx->cTopLeft.x, pCmdEx->cTopLeft.y, pCmdEx->cBottomRight.x, pCmdEx->cBottomRight.y );
							cSrcRect.left -= pFrame->GetFrameOffset()->x;
							cSrcRect.right -= pFrame->GetFrameOffset()->x;
							cSrcRect.top -= pFrame->GetFrameOffset()->y;
							cSrcRect.bottom -= pFrame->GetFrameOffset()->y;
							RECT pic_rc = { 0, 0, pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y };
							if( !RectIntersect(&cSrcRect, &pic_rc, &cSrcRect) )
								break;

							//adjust pos
							pos.x += cSrcRect.left;
							pos.y += cSrcRect.top;
						}

						if( g_bClipOverwrite )
						{
							if( !RectIntersect(&cSrcRect, &g_sClipOverwriteRect, &cSrcRect) )
								break;
							pos.x += cSrcRect.left;
							pos.y += cSrcRect.top;
						}

						//handle exchange color
						if( pSpr->GetExchangeColorCount() > 0 )
						{
							GenerateExchangeColorPalette((WORD*)pSpr->GetPalette16bit() + pSpr->GetColorCount() - pSpr->GetExchangeColorCount(), (COLORCOUNT*)pSpr->GetExchangePalette24bit(), pSpr->GetExchangeColorCount(), pCmd->dwExchangeColor, m_bRGB565);
						}

						//dispatch
						switch( nRealRenderStyle )
						{
						case emRENDER_STYLE_ALPHA_MODULATE_ALPHA_BLENDING:
							{
								DDSURFACEDESC desc;
								desc.dwSize = sizeof(desc);
								if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
								{
									DrawSpriteAlpha((pCmd->dwColor>>24)/8, m_dwMask32, desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, pSpr->GetPalette16bit(), pFrame->GetFrameData(), pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y, &cSrcRect);
									m_lpDDSCanvas->Unlock( NULL );
								}
							}
							break;
						case emRENDER_STYLE_ALPHA_BLENDING :
							{
								DDSURFACEDESC desc;
								desc.dwSize = sizeof(desc);
								if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
								{
									DrawSprite3LevelAlpha(m_dwMask32, desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, pSpr->GetPalette16bit(), pFrame->GetFrameData(), pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y, &cSrcRect);
									m_lpDDSCanvas->Unlock( NULL );
								}
							}
							break;
						case emRENDER_STYLE_COPY:
							{
								DDSURFACEDESC desc;
								desc.dwSize = sizeof(desc);
								if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
								{
									DrawSprite(desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, pSpr->GetPalette16bit(), pFrame->GetFrameData(), pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y, &cSrcRect);
									m_lpDDSCanvas->Unlock( NULL );
								}
							}
							break;
						case emRENDER_STYLE_ALPHA_COLOR_MODULATE_ALPHA_BLENDING:
							{
								//handle modulate color  
								WORD wModulatedPal[256];
								ModulatePalette(wModulatedPal, (WORD*)pSpr->GetPalette16bit(), pSpr->GetColorCount(), pCmd->dwColor, m_bRGB565);

								DDSURFACEDESC desc;
								desc.dwSize = sizeof(desc);
								if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
								{
									DrawSpriteAlpha((pCmd->dwColor>>24)/8, m_dwMask32, desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, wModulatedPal, pFrame->GetFrameData(), pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y, &cSrcRect);
									m_lpDDSCanvas->Unlock( NULL );
								}
							}
							break;
						case emRENDER_STYLE_SCREEN:
							{
								DDSURFACEDESC desc;
								desc.dwSize = sizeof(desc);
								if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
								{
									DrawSpriteScreenBlendMMX((BYTE)(pCmd->dwColor>>24), m_dwMask32, desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, pSpr->GetPalette16bit(), pFrame->GetFrameData(), pFrame->GetFrameSize()->x, pFrame->GetFrameSize()->y, &cSrcRect);
									m_lpDDSCanvas->Unlock( NULL );
								}
							}
							break;
						}
					}
					break;
				case emPIC_TYPE_JPG_OR_LOCKABLE:
				case emPIC_TYPE_RT:
					{
						//correct handle
						DWORD dwType = CheckHandle(pCmd->dwHandle1);
						if( dwType != g_dwMediaTypeJpg && dwType != g_dwMediaTypeBitmap )
						{
							DWORD dwHandle;
							if( !(dwHandle = FindMedia( g_dwMediaTypeBitmap, pCmd->szFileName )) )
							{
								dwHandle = FindOrCreateMedia( g_dwMediaTypeJpg, pCmd->szFileName, 0 );
							}
							pCmd->dwHandle1 = pCmd->dwHandle2 = dwHandle;
						}

						//query bitmap
						IKBitmap_t* bmp = 0;
						if( !QueryProduct(pCmd->dwHandle1, (CheckHandle(pCmd->dwHandle1) == g_dwMediaTypeBitmap) ? true : false, (void**)&bmp) || !bmp )
							break;

						//pos
						D3DXVECTOR3 sPosSaved;
						if( !bSinglePlaneCoord )
						{
							sPosSaved = pCmd->sPos1;
							KPOINT out;
							out.x = (INT)pCmd->sPos1.x;
							out.y = (INT)pCmd->sPos1.y;
							SpaceToScreen(out.x, out.y, (INT)pCmd->sPos1.z);
							pCmd->sPos1.x = (float)out.x;
							pCmd->sPos1.y = (float)out.y;
						}

						KPOINT pos;
						pos.x = (INT)pCmd->sPos1.x;
						pos.y = (INT)pCmd->sPos1.y;

						if( !bSinglePlaneCoord )
						{
							pCmd->sPos1 = sPosSaved;
						}

						//src_rect
						RECT sSrcRect;
						if( uGenre == emPRIMITIVE_TYPE_PIC )
						{
							SetRect( &sSrcRect, 0, 0, bmp->GetSize()->x, bmp->GetSize()->y );
						}
						else
						{
							KPicCmdPart* cmdex = (KPicCmdPart*)pCmd;
							SetRect( &sSrcRect, cmdex->cTopLeft.x, cmdex->cTopLeft.y, cmdex->cBottomRight.x, cmdex->cBottomRight.y );
							RECT pic_rc = { 0, 0, bmp->GetSize()->x, bmp->GetSize()->y };
							if( !RectIntersect(&sSrcRect, &pic_rc, &sSrcRect) )
								break;

							//adjust pos
							pos.x += sSrcRect.left;
							pos.y += sSrcRect.top;
						}

						DDSURFACEDESC desc;
						desc.dwSize = sizeof(desc);
						if ( m_lpDDSCanvas->Lock( NULL, &desc, DDLOCK_WAIT, NULL ) == DD_OK )
						{
							switch( pCmd->dwType )
							{
							case emPIC_TYPE_JPG_OR_LOCKABLE :
								DrawBitmap16( desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, bmp->GetData(), bmp->GetSize()->x, bmp->GetSize()->y, &sSrcRect );
								break;
							case emPIC_TYPE_RT :
								DrawBitmap16AlphaMMX( desc.lpSurface, m_cCanvasSize.x, m_cCanvasSize.y, desc.lPitch, pos.x, pos.y, bmp->GetData(), bmp->GetSize()->x, bmp->GetSize()->y, &sSrcRect );
								break;
							}
							m_lpDDSCanvas->Unlock( NULL );
						}
					}
					break;
				}
			}
		}
		break;
	}
}

void KShell::DrawPrimitivesOnImage(INT nPrimitiveCount, void* pPrimitives, DWORD uGenre, LPCSTR pszImage, DWORD uImage, DWORD& nImagePosition, INT bForceDrawFlag)
{
	if(NULL != m_pJxPlayer)
	{
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_DrawPrimitivesOnImg, 
			"npusim", 
			nPrimitiveCount, 
			pPrimitives, 
			uGenre, 
			pszImage, 
			uImage, 
			nImagePosition);
	}

	//find render target
	DWORD dwHandle;
	if( CheckHandle( uImage ) == g_dwMediaTypeBitmap )
		dwHandle = uImage;
	else if( CheckHandle( nImagePosition ) == g_dwMediaTypeBitmap )
		dwHandle = nImagePosition;
	else
		dwHandle = FindMedia( g_dwMediaTypeBitmap, pszImage );

	if( !dwHandle )
		return;

	//save handle
	nImagePosition = dwHandle;

	//query rt
	IKBitmap_t* pRT = 0;
	if( !QueryProduct(dwHandle, true, (void**)&pRT) || !pRT )
		return;

	switch( uGenre )
	{
	case emPRIMITIVE_TYPE_PIC:	
		{
			KPicCmd* pCmds = (KPicCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				switch( pCmds[i].dwType )
				{
				case emPIC_TYPE_SPR:	
					{
						//correct handle
						if( CheckHandle(pCmds[i].dwHandle1) != g_dwMediaTypeSpr )
						{
							pCmds[i].dwHandle1 = pCmds[i].dwHandle2 = FindOrCreateMedia( g_dwMediaTypeSpr, pCmds[i].szFileName, 0 );
						}

						//query spr
						KSprite* spr = 0;
						if( !QueryProduct(pCmds[i].dwHandle1, true, (void**)&spr) || !spr )
							break;

						//query frame
						KSpriteFrame* frame = 0;
						if( !spr->QueryFrame(pCmds[i].wFrameIndex, true, (void**)&frame) || !frame )
							break;

						//eval pos
						KPOINT pos;
						switch( pCmds[i].byRefMethod )
						{
						case emREF_METHOD_TOP_LEFT:
							pos.x = (INT)pCmds[i].sPos1.x + frame->GetFrameOffset()->x;
							pos.y = (INT)pCmds[i].sPos1.y + frame->GetFrameOffset()->y;
							break;
						case emREF_METHOD_CENTER:
							{
								KPOINT center = *spr->GetCenter();
								if( center == KPOINT(0, 0) && spr->GetSize()->x > 160 )
								{
									center = KPOINT(160, 208);
								}
								pos.x = (INT)pCmds[i].sPos1.x - center.x + frame->GetFrameOffset()->x;
								pos.y = (INT)pCmds[i].sPos1.y - center.y + frame->GetFrameOffset()->y;
							}
							break;
						case emREF_METHOD_FRAME_TOP_LEFT:
							pos.x = (INT)pCmds[i].sPos1.x;
							pos.y = (INT)pCmds[i].sPos1.y;
							break;
						default:
							pos.x = (INT)pCmds[i].sPos1.x;
							pos.y = (INT)pCmds[i].sPos1.y;
							break;
						}

						//dispatch
						switch( pCmds[i].byRenderStyle )
						{
						case emRENDER_STYLE_ALPHA_MODULATE_ALPHA_BLENDING:
							{
								switch( pRT->GetFormat() )
								{
								case emPIC_TYPE_JPG_OR_LOCKABLE:
									RIO_CopySprToBufferAlpha(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, spr->GetPalette16bit(), pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								case emPIC_TYPE_RT:
									RIO_CopySprToAlphaBufferAlpha(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, spr->GetPalette16bit(), pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								}
							}
							break;
						case emRENDER_STYLE_ALPHA_BLENDING:
							{
								switch( pRT->GetFormat() )
								{
								case emPIC_TYPE_JPG_OR_LOCKABLE:
									RIO_CopySprToBuffer3LevelAlpha(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, spr->GetPalette16bit(), pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								case emPIC_TYPE_RT:
									RIO_CopySprToAlphaBuffer3LevelAlpha(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, spr->GetPalette16bit(), pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								}
							}
							break;
						case emRENDER_STYLE_COPY:
							{
								switch( pRT->GetFormat() )
								{
								case emPIC_TYPE_JPG_OR_LOCKABLE:
									RIO_CopySprToBuffer(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, spr->GetPalette16bit(), pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								case emPIC_TYPE_RT:
									RIO_CopySprToAlphaBuffer(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, spr->GetPalette16bit(), pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y);
									break;
								}
							}
							break;
						case emRENDER_STYLE_ALPHA_COLOR_MODULATE_ALPHA_BLENDING:
							{
								//handle modulate color  
								WORD modulated_pal[256];
								ModulatePalette(modulated_pal, (WORD*)spr->GetPalette16bit(), spr->GetColorCount(), pCmds[i].dwColor, m_bRGB565);

								switch( pRT->GetFormat() )
								{
								case emPIC_TYPE_JPG_OR_LOCKABLE:
									RIO_CopySprToBufferAlpha(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, modulated_pal, pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								case emPIC_TYPE_RT:
									RIO_CopySprToAlphaBufferAlpha(frame->GetFrameData(), frame->GetFrameSize()->x, frame->GetFrameSize()->y, modulated_pal, pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, pos.x, pos.y, m_dwMask32);
									break;
								}
							}
							break;
						}
					}
					break;
				case emPIC_TYPE_JPG_OR_LOCKABLE:
				case emPIC_TYPE_RT:
					{
						//correct handle
						DWORD dwType = CheckHandle(pCmds[i].dwHandle1);
						if( dwType != g_dwMediaTypeJpg && dwType != g_dwMediaTypeBitmap )
						{
							DWORD dwHandle;
							if( !(dwHandle = FindMedia( g_dwMediaTypeBitmap, pCmds[i].szFileName )) )
							{
								dwHandle = FindOrCreateMedia( g_dwMediaTypeJpg, pCmds[i].szFileName, 0 );
							}
							pCmds[i].dwHandle1 = pCmds[i].dwHandle2 = dwHandle;
						}

						//query bitmap
						IKBitmap_t* bmp = 0;
						if( !QueryProduct(pCmds[i].dwHandle1, true, (void**)&bmp) || !bmp )
							break;

						//pos
						KPOINT cPos;
						cPos.x = (INT)pCmds[i].sPos1.x;
						cPos.y = (INT)pCmds[i].sPos1.y;

						RIO_CopyBitmap16ToBuffer(bmp->GetData(), bmp->GetSize()->x, bmp->GetSize()->y, pRT->GetData(), pRT->GetSize()->x, pRT->GetSize()->y, cPos.x, cPos.y);
					}
					break;
				}
			}
		}
		break;
	}
}

//jpg loader
void* KShell::GetJpgImage(const char cszName[], unsigned int uRGBMask16)
{
	LPD3DBUFFER pFiledata = 0;
	if( !LoadFile(cszName, &pFiledata) || !pFiledata )
		return NULL;

	EnterJpglib();

	if( !jpeg_decode_init( uRGBMask16 == 0x7fff, true) )
	{
		LeaveJpglib();
		SAFE_RELEASE(pFiledata);
		return NULL;
	}

	JPEG_INFO info;
	if( !jpeg_decode_info( (PBYTE)pFiledata->GetBufferPointer(), &info ) )
	{
		LeaveJpglib();
		SAFE_RELEASE(pFiledata);
		return NULL;
	}

	LPVOID pRetBuf = new CHAR[sizeof(KPOINT) + info.nWidth * info.nHeight * 2]; 
	if( !pRetBuf )
	{
		LeaveJpglib();
		SAFE_RELEASE(pFiledata);
		return NULL;
	}

	((KPOINT*)pRetBuf)->x = info.nWidth;
	((KPOINT*)pRetBuf)->y = info.nHeight;

	if( !jpeg_decode_data( (PWORD)&((KPOINT*)pRetBuf)[1], &info ) )
	{
		LeaveJpglib();
		SAFE_DELETE_ARRAY(pRetBuf);
		SAFE_RELEASE(pFiledata);
		return NULL;
	}

	LeaveJpglib();

	SAFE_RELEASE(pFiledata);
	return pRetBuf;
}

void KShell::ReleaseImage(void* pImage)
{
	SAFE_DELETE_ARRAY(pImage);
}

//screen capture
bool SaveToBmpFileInMemory_8888(LPD3DBUFFER* pOut, PVOID pBitmap, INT nPitch, INT nWidth, INT nHeight)
{
	LPD3DBUFFER pBuffer = 0;

	// byte per line % 4 must = 0
	INT nBytesPerLine = nWidth * 3;
	if ((nBytesPerLine % 4) != 0)
		nBytesPerLine = nBytesPerLine + 4 - (nBytesPerLine % 4);

	// alloc buffer
	if( FAILED(D3DXCreateBuffer(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + nHeight * nBytesPerLine, &pBuffer)) )
		return false;

	// file header
	BITMAPFILEHEADER* FileHeader = (BITMAPFILEHEADER*)pBuffer->GetBufferPointer();

	FileHeader->bfType          = 0x4d42; // "BM"
	FileHeader->bfSize          = nBytesPerLine * nHeight + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	FileHeader->bfReserved1     = 0;
	FileHeader->bfReserved2     = 0;
	FileHeader->bfOffBits       = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// info header
	BITMAPINFOHEADER* InfoHeader = (BITMAPINFOHEADER*)&FileHeader[1];

	InfoHeader->biSize          = sizeof(BITMAPINFOHEADER);
	InfoHeader->biWidth         = nWidth;
	InfoHeader->biHeight        = nHeight;
	InfoHeader->biPlanes        = 1;
	InfoHeader->biBitCount      = 24;
	InfoHeader->biCompression   = 0;
	InfoHeader->biSizeImage     = 0;
	InfoHeader->biXPelsPerMeter = 0xb40;
	InfoHeader->biYPelsPerMeter = 0xb40;
	InfoHeader->biClrUsed       = 0;
	InfoHeader->biClrImportant  = 0;

	// encode bitmap
	LPBYTE lpDes = (LPBYTE)&InfoHeader[1];
	LPBYTE lpSrc = (LPBYTE)pBitmap;

	INT nLineAdd = nPitch - nWidth * 4;
	lpDes += (nHeight - 1) * nBytesPerLine;
	{
		for (INT i = 0; i < nHeight; i++)
		{
			for (INT j = 0; j < nWidth; j++)
			{
				lpDes[0] = lpSrc[0];
				lpDes[1] = lpSrc[1];
				lpDes[2] = lpSrc[2];
				lpDes += 3;
				lpSrc += 4;
			}
			lpDes -= 3 * nWidth;
			lpDes -= nBytesPerLine;
			lpSrc += nLineAdd;
		}
	}

	*pOut = pBuffer;
	return true;
}

bool SaveToBmpFile_8888(LPCSTR lpFileName, PVOID pBitmap, INT nPitch, INT nWidth, INT nHeight)
{
	LPD3DBUFFER pFiledata = 0;
	if( !SaveToBmpFileInMemory_8888(&pFiledata, pBitmap, nPitch, nWidth, nHeight) )
		return false;

	// crete the file
	IFile* pFile = g_pfnCreateFile(lpFileName);
	if( !pFile )
	{
		SAFE_RELEASE(pFiledata);
		return false;
	}

	// write
	pFile->Write(pFiledata->GetBufferPointer(), pFiledata->GetBufferSize());

	// release
	SAFE_RELEASE(pFile);
	SAFE_RELEASE(pFiledata);
	return true;
}

INT GetEncoderClsid(CONST WCHAR* pwchFormat, CLSID* pClsid)
{
	UINT  uNum = 0;          // number of image encoders
	UINT  uSize = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&uNum, &uSize);
	if(uSize == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)new CHAR[uSize];
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(uNum, uSize, pImageCodecInfo);

	for(UINT j = 0; j < uNum; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, pwchFormat) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			delete[] (CHAR*)pImageCodecInfo;
			return j;  // Success
		}    
	}

	delete[] (CHAR*)pImageCodecInfo;
	return -1;  // Failure
}

bool SaveToJpgFile_8888(LPCSTR lpFileName, PVOID pBitmap, INT nPitch, INT nWidth, INT nHeight, UINT nQuality)
{
	LPD3DBUFFER pFiledata = 0;
	if( !SaveToBmpFileInMemory_8888(&pFiledata, pBitmap, nPitch, nWidth, nHeight) )
		return false;

	LPSTREAM pStream = 0;
	if( FAILED(CreateStreamOnHGlobal(NULL, true, &pStream)) )
	{
		SAFE_RELEASE(pFiledata);
		return false;
	}

	ULARGE_INTEGER unStreamSize;
	unStreamSize.HighPart = 0;
	unStreamSize.LowPart = pFiledata->GetBufferSize();
	pStream->SetSize(unStreamSize);
	pStream->Write(pFiledata->GetBufferPointer(), pFiledata->GetBufferSize(), 0);
	SAFE_RELEASE(pFiledata);

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Get the CLSID of the JPEG encoder.
	CLSID encoderClsid;
	if( GetEncoderClsid(L"image/jpeg", &encoderClsid) == -1 )
	{
		GdiplusShutdown(gdiplusToken);
		SAFE_RELEASE(pStream);
		return false;
	}

	EncoderParameters cEncoderParameters;
	cEncoderParameters.Count = 1;
	cEncoderParameters.Parameter[0].Guid = EncoderQuality;
	cEncoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	cEncoderParameters.Parameter[0].NumberOfValues = 1;
	cEncoderParameters.Parameter[0].Value = &nQuality;

	WCHAR szWFileName[512];
	MultiByteToWideChar(CP_ACP, 0, lpFileName, (INT)strlen(lpFileName)+1, szWFileName, 510);

	Image* pImage = ::new Image(pStream);
	bool ok = (pImage->Save(szWFileName, &encoderClsid, &cEncoderParameters) == Ok);

	::delete pImage;
	SAFE_RELEASE(pStream);
	GdiplusShutdown(gdiplusToken);

	return ok;
}

bool KShell::SaveScreenToFile(LPCSTR pszName, DWORD eType, DWORD nQuality)
{
	RECT cSrcRect;

	if( !m_bFullScreen )
	{
		RECT client_rc;
		GetClientRect(m_hWnd, &client_rc);
		ClientToScreen(m_hWnd, (LPPOINT)&client_rc);
		ClientToScreen(m_hWnd, (LPPOINT)&client_rc + 1);

		RECT screen_rc;
		SetRect(&screen_rc, 0, 0, m_cScreenSize.x, m_cScreenSize.y);

		if( !RectIntersect( &client_rc, &screen_rc, &cSrcRect ) )
			return false;
	}
	else
	{
		SetRect(&cSrcRect, 0, 0, m_cScreenSize.x, m_cScreenSize.y);
	}

	INT w = cSrcRect.right - cSrcRect.left;
	INT h = cSrcRect.bottom - cSrcRect.top;

	LPVOID buf = new CHAR[w * h * 4];
	if( !buf )
		return false;

	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if( m_lpDDSPrimary->Lock( &cSrcRect, &ddsd, DDLOCK_WAIT, NULL ) != DD_OK )
	{
		delete[] (CHAR*)buf;
		return false;
	}

	WORD* src = (WORD*)ddsd.lpSurface;
	BYTE* dst = (BYTE*)buf;
	for( INT v = 0; v < h; v++ )
	{
		for( INT u = 0; u < w; u++ )
		{
			if( m_bRGB565 )
			{
				dst[4*u+3] = 0;
				dst[4*u+2] = (src[u] & 0xf800) >> 8;
				dst[4*u+1] = (src[u] & 0x07e0) >> 3;
				dst[4*u] = (src[u] & 0x001f) << 3;
			}
			else
			{
				dst[4*u+3] = 0;
				dst[4*u+2] = (src[u] & 0x7c00) >> 7;
				dst[4*u+1] = (src[u] & 0x03e0) >> 2;
				dst[4*u] = (src[u] & 0x001f) << 3;
			}
		}
		src = (WORD*)((BYTE*)src + ddsd.lPitch);
		dst += 4*w;
	}
	m_lpDDSPrimary->Unlock( NULL );

	bool ok;
	if( eType == 0 )
		ok = SaveToBmpFile_8888(pszName, buf, w*4, w, h);
	else 
		ok = SaveToJpgFile_8888(pszName, buf, w*4, w, h, nQuality);

	delete[] (CHAR*)buf;

	return ok;
}

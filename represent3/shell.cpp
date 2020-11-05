/* -------------------------------------------------------------------------
//	文件名		：	_shell_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	shell的具体实现，整个渲染部分的外壳，对外提供各种功能接口
//
// -----------------------------------------------------------------------*/
#include "shell.h"

KShell g_cShell;
DWORD g_dwCallTimes1 = 0; // tmp: spr绘制次数
CONST DWORD VIDEO_MEM_THRESHOLD = 32; // 剩余显存容量低-下限
CONST DWORD VIDEO_MEM_DETECT_INTEVAL = 1000; // 剩余显存侦测间隔
BOOL g_bLowVidMem = FALSE;

bool CreateMultiquadIndexBuffer(DWORD dwQuadCount, LPD3DIB* pOut)
{
	D3DPOOL pool = (Gpu()->GetCreationParameters()->eVertexProcessingMethod == emGPU_VERTEX_PROCESSING_METHOD_SOFEWARE) ? D3DPOOL_SYSTEMMEM : D3DPOOL_MANAGED;
	DWORD dwIbSize = dwQuadCount * 6 * 2;
	LPD3DIB pIb = 0;
	if( !Gpu()->CreateIndexBuffer(dwIbSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, pool, &pIb) )
		return false;

	WORD* k;
	pIb->Lock(0, 0, (void**)&k, 0);
	for(WORD i = 0; i < dwQuadCount; i++)
	{
		k[6*i+0] = 4*i+0;
		k[6*i+1] = 4*i+1;
		k[6*i+2] = 4*i+2;
		k[6*i+3] = 4*i+2;
		k[6*i+4] = 4*i+1;
		k[6*i+5] = 4*i+3;
	}
	pIb->Unlock();

	*pOut = pIb;
	return true;
}

INT	KShell::GetRepresentParam(DWORD lCommand, INT& lParam, DWORD& uParam) 
{
	enum
	{
		emCMD_TEXTURE_FORMAT = 101,
	};

	switch( lCommand )
	{
	case emCMD_TEXTURE_FORMAT:
		{
			lParam = (INT)g_cGlobalConfig.m_dwTextureFormat;
		}
		break;
	}
	return 0;
}
INT	KShell::SetRepresentParam(DWORD lCommand, INT lParam, DWORD uParam)
{
	enum
	{
		emCMD_TEXTURE_FORMAT = 101,
	};

	switch( lCommand )
	{
	case emCMD_TEXTURE_FORMAT:
		if( g_cGlobalConfig.m_dwTextureFormat != lParam )
		{
			PurgeProcessingQueues();
			CollectGarbage(0,0);
			KSprInitParam param;
			param.bEnableConditionalNonPow2 = g_sSprConfig.bEnableConditionalNonPow2;
			param.dwMipmap = g_sSprConfig.dwMipmap;
			switch( lParam )
			{
			case 0:
				param.bUseCompressedTextureFormat = false;
				param.bUse16bitColor = false;
				break;
			case 1:
				param.bUseCompressedTextureFormat = false;
				param.bUse16bitColor = true;
				break;
			default:
				param.bUseCompressedTextureFormat = true;
				param.bUse16bitColor = true;
				break;
			}
			InitSprCreator( &param );
			g_cGlobalConfig.m_dwTextureFormat = lParam;
		}
		break;
	}
	return 0;
}

bool KShell::Create(HWND hWnd, INT nWidth, INT nHeight, bool& bFullScreen, const KRepresentConfig* pRepresentConfig)
{
	//load engine.dll
	if( !LoadEngineDll() )
		return false;

	//load config.ini
	LoadGlobalConfig(pRepresentConfig);

	//init gpu
	KE_GPU_ERROR error;
	if( !InitGpu(&error) )
	{
		ReleaseEngineDll();
		return false;
	}

	//set window size & style (force to fullscreen mode if the window size is not avaiable in windowed mode)
	if( !bFullScreen )
	{
		RECT rc = { 0, 0, nWidth, nHeight };
		SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX );
		AdjustWindowRectEx( &rc, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE) );
		SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE );

		RECT new_rc;
		GetClientRect( hWnd, &new_rc );
		if( nWidth != new_rc.right - new_rc.left || nHeight != new_rc.bottom - new_rc.top )
		{
			bFullScreen = true;
		}
	}

	Gpu()->GetCurrentDisplayMode(&m_sDesktopMode);
	bool bFakeFullscreen = false;
	if( bFullScreen )
	{
		if( m_sDesktopMode.Width == nWidth && m_sDesktopMode.Height == nHeight )
			bFakeFullscreen = true;

		if( bFakeFullscreen )
		{
			SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP );
			SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, nWidth, nHeight, SWP_NOACTIVATE );
		}
		else
			SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_POPUP );
	}

	//choose cp & pp
	KGpuCreationParameters cp;
	cp.hWnd = hWnd;
	cp.bMultithread = (g_cGlobalConfig.m_dwLoaderThreadCount > 0) ? BOOL_bool(g_cGlobalConfig.m_bMultithreadedD3d) : false;
	cp.eVertexProcessingMethod = Gpu()->GetHardwareCaps()->eMaxVertexProcessingLevel;

	KGpuPresentationParameters pp;
	if( !ChooseBestGpuPP( Gpu(), !(bFullScreen && !bFakeFullscreen), nWidth, nHeight, !BOOL_bool(g_cGlobalConfig.m_bUse16bitColor), BOOL_bool(g_cGlobalConfig.m_bDisableFullscreenIme), &pp) )
	{
		ShutdownGpu();
		ReleaseEngineDll();
		return false;
	}
	if( pp.bWindowed )
	{
		pp.dwBackbufferWidth = nWidth;
		pp.dwBackbufferHeight = nHeight;
	}
	pp.dwBackbufferCount = 0;
	pp.bVerticalSync = BOOL_bool(g_cGlobalConfig.m_bVerticalSync);

	//check pp
	if( pp.bWindowed != !(bFullScreen && !bFakeFullscreen) || pp.dwBackbufferWidth != nWidth || pp.dwBackbufferHeight != nHeight )
	{
		ShutdownGpu();
		ReleaseEngineDll();
		return false;
	}

	//turn on gpu
	if( !Gpu()->TurnOn(&cp, &pp, &error) )
	{
		ShutdownGpu();
		ReleaseEngineDll();
		return false;
	}

	//set dynamic vertex buffer size
	Gpu()->SetDynamicVertexBufferSize(g_cGlobalConfig.m_dwMaxBatchedQuadCount * 4 * 32);

	//init media center
	InitMediaCenter(g_cGlobalConfig.m_dwLoaderThreadCount, BOOL_bool(g_cGlobalConfig.m_bMultithreadedD3d));

	//install media types
	//spr
	KSprInitParam spr_init_param;
	spr_init_param.bEnableConditionalNonPow2 = BOOL_bool(g_cGlobalConfig.m_bEnableConditionalNonPow2);
	spr_init_param.dwMipmap = g_cGlobalConfig.m_dwMipmap;
	spr_init_param.bUseCompressedTextureFormat = BOOL_bool(g_cGlobalConfig.m_bUseCompressedTextureFormat);
	spr_init_param.bUse16bitColor = BOOL_bool(g_cGlobalConfig.m_bUse16bitColor);
	g_dwMediaTypeSpr = InstallMediaType(InitSprCreator, &spr_init_param, CreateSpr, ShutdownSprCreator);
	//jpg
	KJpgInitParam jpg_init_param;
	jpg_init_param.m_bEnableConditionalNonPow2 = BOOL_bool(g_cGlobalConfig.m_bEnableConditionalNonPow2);
	jpg_init_param.m_dwMipmap = g_cGlobalConfig.m_dwMipmap;
	jpg_init_param.m_bUse16bitColor = BOOL_bool(g_cGlobalConfig.m_bUse16bitColor);
	jpg_init_param.m_bUseCompressedTextureFormat = BOOL_bool(g_cGlobalConfig.m_bUseCompressedTextureFormat);
	g_dwMediaTypeJpg = InstallMediaType(InitJpgCreator, &jpg_init_param, CreateJpg, ShutdownJpgCreator);
	//rt
	KRTInitParam rt_init_param;
	rt_init_param.bEnableConditionalNonPow2 = BOOL_bool(g_cGlobalConfig.m_bEnableConditionalNonPow2);
	rt_init_param.dwMipmap = g_cGlobalConfig.m_dwMipmap;
	rt_init_param.bUse16bitColor = BOOL_bool(g_cGlobalConfig.m_bUse16bitColor);
	g_dwMediaTypeRt = InstallMediaType(InitRTCreator, &rt_init_param, CreateRT, ShutdownRTCreator);
	//lockable
	KLockableInitParam lockable_init_param;
	lockable_init_param.bEnableConditionalNonPow2 = BOOL_bool(g_cGlobalConfig.m_bEnableConditionalNonPow2);
	g_dwMediaTypeLockable = InstallMediaType(InitLockableCreator, &lockable_init_param, CreateLockable, ShutdownLockableCreator);
	//font
	FontInitParam font_init_param;
	font_init_param.bEnableConditionalNonPow2 = BOOL_bool(g_cGlobalConfig.m_bEnableConditionalNonPow2);
	font_init_param.bSingleByteCharSet = BOOL_bool(g_cGlobalConfig.m_bSingleByteCharSet);
	font_init_param.dwFontTextureSize = g_cGlobalConfig.m_dwFontTextureSize;
	g_dwMediaTypeFont = InstallMediaType(InitFontCreator, &font_init_param, CreateFont, ShutdownFontCreator);

	//init kernel
	InitKernel();

	//install sub kernels
	LPD3DIB	multiquad_ib = 0;
	CreateMultiquadIndexBuffer( g_cGlobalConfig.m_dwMaxBatchedQuadCount * 2, &multiquad_ib );
	//pic
	KPicKernelInitParam pic_kernel_init_param;
	pic_kernel_init_param.pMultiquadIb = multiquad_ib;
	pic_kernel_init_param.bLinearFilter = BOOL_bool(g_cGlobalConfig.m_bLinearFilter);
	g_dwKernelTypePic = InstallSubKernel(InitPicKernel, &pic_kernel_init_param, BatchPicCmd, FlushPicBuf, ShutdownPicKernel );
	//text
	KTextKernelInitParam text_kernel_init_param;
	text_kernel_init_param.pMultiquadIb = multiquad_ib;
	text_kernel_init_param.bWordWrap = BOOL_bool(g_cGlobalConfig.m_bWordWrap);
	text_kernel_init_param.bTextJustification = BOOL_bool(g_cGlobalConfig.m_bTextJustification);
	g_dwKernelTypeText = InstallSubKernel(InitTextKernel, &text_kernel_init_param, BatchTextCmd, FlushTextBuf, ShutdownTextKernel );
	//rect
	KRectKernelInitParam rect_kernel_init_param;
	rect_kernel_init_param.pMultiquadIb = multiquad_ib;
	g_dwKernelTypeRect = InstallSubKernel(InitRectKernel, &rect_kernel_init_param, BatchRectCmd, FlushRectBuf, ShutdownRectKernel );
	//line
	g_dwKernelTypeLine = InstallSubKernel(InitLineKernel, 0, BatchLineCmd, FlushLineBuf, ShutdownLineKernel );

	SAFE_RELEASE( multiquad_ib );

	//start timer
	StartTimer();

	//internal states
	m_bRedrawGround = false;
	m_bGpuLost = false;

	//turn on font smoothing
	SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	return true;
}

void KShell::Release()
{
	m_mapFont.clear();
	ShutdownKernel();
	ShutdownMediaCenter();
	Gpu()->TurnOff();
	ShutdownGpu();
	ReleaseEngineDll();
}

bool KShell::Reset(INT nWidth, INT nHeight, bool bFullScreen, bool bNotAdjustStyle)
{
	//save desktop mode
	if( Gpu()->GetPresentationParameters()->bWindowed )
		Gpu()->GetCurrentDisplayMode(&m_sDesktopMode);

	//save old pp
	KPOINT sOldCanvasSize = KPOINT(Gpu()->GetPresentationParameters()->dwBackbufferWidth, Gpu()->GetPresentationParameters()->dwBackbufferHeight);
	bool bOldFullScreen = !Gpu()->GetPresentationParameters()->bWindowed;

	bool bFakeFullScreen = false;
	if( bFullScreen && m_sDesktopMode.Width == nWidth && m_sDesktopMode.Height == nHeight )
		bFakeFullScreen = true;

	//skip trival case
	if( sOldCanvasSize == KPOINT(nWidth, nHeight) && bOldFullScreen == (bFullScreen && !bFakeFullScreen) )
		return true;

	//choose best pp
	KGpuPresentationParameters pp;
	if( !ChooseBestGpuPP( Gpu(), !(bFullScreen && !bFakeFullScreen), nWidth, nHeight, !BOOL_bool(g_cGlobalConfig.m_bUse16bitColor), BOOL_bool(g_cGlobalConfig.m_bDisableFullscreenIme), &pp) )
		return false;
	if( pp.bWindowed )
	{
		pp.dwBackbufferWidth = nWidth;
		pp.dwBackbufferHeight = nHeight;
	}
	pp.dwBackbufferCount = 1;
	pp.bVerticalSync = BOOL_bool(g_cGlobalConfig.m_bVerticalSync);

	//check pp
	if( pp.bWindowed != !(bFullScreen && !bFakeFullScreen) || pp.dwBackbufferWidth != nWidth || pp.dwBackbufferHeight != nHeight )
		return false;

	//reset window style before resetting gpu
	HWND hWnd = Gpu()->GetCreationParameters()->hWnd;
	if( pp.bWindowed )
	{
		if( bFakeFullScreen )
			SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP );
		else
			SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX );
	}
	else
		SetWindowLong( hWnd, GWL_STYLE, WS_VISIBLE | WS_SYSMENU | WS_POPUP );

	//discard any video objects before resetting gpu
	PurgeProcessingQueues();
	DiscardVideoObjects();

	//reset gpu
	KE_GPU_ERROR error;
	if( !Gpu()->Reset(&pp, &error) )
	{
		if( error == emGPU_ERROR_DEVICE_LOST )
		{
			m_bGpuLost = true;
		}
		return false;
	}

	//mark the ground as lost
	m_bRedrawGround = true;

	//set window size for windowed mode
	if( pp.bWindowed )
	{
		if( bFakeFullScreen )
		{
			SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, nWidth, nHeight, SWP_NOACTIVATE );
		}
		else
		{
			RECT rc = { 0, 0, nWidth, nHeight };
			AdjustWindowRectEx( &rc, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE) );
			SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE );

			//the client size should be the same as the backbuffer size
			RECT sNewRect;
			GetClientRect( hWnd, &sNewRect );
			if( sNewRect.right - sNewRect.left != nWidth || sNewRect.bottom - sNewRect.top != nHeight )
			{
				//restore
				Reset( sOldCanvasSize.x, sOldCanvasSize.y, bOldFullScreen, 0 );
				return false;
			}
		}
	}

	return true;
}

bool KShell::RepresentBegin(INT bClear, DWORD Color)
{
	//timer
	UpdateTime();
	g_dwCallTimes1 = 0;

	if(m_bGpuLost)
	{
		KE_GPU_ERROR error;
		if( !Gpu()->TestCooperativeLevel(&error) )
		{
			if( error == emGPU_ERROR_DEVICE_LOST )
				return false;

			if( m_pJxPlayer && 
				( m_pJxPlayer->GetCurState() == enum_Replay_Playing ||
				m_pJxPlayer->GetCurState() == enum_Replay_PreDraw ||
				m_pJxPlayer->IsReplayPaused() ) )
				m_pJxPlayer->Stop();

			PurgeProcessingQueues();
			DiscardVideoObjects();

			//save desktop mode
			if( Gpu()->GetPresentationParameters()->bWindowed )
				Gpu()->GetCurrentDisplayMode(&m_sDesktopMode);

			if( !Gpu()->Reset(Gpu()->GetPresentationParameters(), &error) )
				return false;

			m_bRedrawGround = true;
		}

		m_bGpuLost = false;
	}

	Gpu()->ClearTarget(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);

	return Gpu()->BeginScene() == TRUE;
}

void KShell::RepresentEnd()
{
	//if (_gpu_.device && timeGetTime() - now_time() > VIDEO_MEM_DETECT_INTEVAL)
	//{
	//	if (_gpu_.device->GetAvailableTextureMem() < VIDEO_MEM_THRESHOLD * 1024 *1024)
	//	{
	//	KGLogPrintf(KGLOG_WARNING, "可用显存过低!");
	//	g_bLowVidMem = TRUE;
	//	}
	//	else
	//	{
	//		g_bLowVidMem = FALSE;
	//	}
	//}

	if( g_cGlobalConfig.m_bShowFps )
	{
		//performance monitor
		static DWORD last_time = NowTime();
		static DWORD frame_elapsed = 0;
		static WCHAR msg[256] = L"";

		UpdateTime();
		frame_elapsed++;
		if( NowTime() - last_time > 1000 )
		{
			DWORD fps = 1000 * frame_elapsed / (NowTime() - last_time);
			DWORD time = (NowTime() - g_vecMediaCasing[g_vlstMediaPriorityList.Back()].m_dwLastQueryTime) / 1000;
			DWORD space = g_dwMediaBytes/1024/1024;

			last_time = NowTime();
			frame_elapsed = 0;

			swprintf(msg, L"FPS:%d TIME:%d SPACE:%d, %d", fps, time, space, g_dwCallTimes1);
		}
		OutputText(12, msg, -1, 30, 42, 0xff00ff00);
	}

	m_bRedrawGround = false;

	//flush
	ProcessCmd(0, 0, 0);

	//unbind
	Gpu()->SetStreamSource(0, 0);
	Gpu()->SetIndices(0);
	Gpu()->SetTexture(0);

	//frame end
	Gpu()->EndScene();

	//present
	KE_GPU_ERROR error;
	if( !Gpu()->Present(&error) )
	{
		m_bGpuLost = true;
	}

	//garbage collection
	CollectGarbage( g_cGlobalConfig.m_dwMediaTimeLimit * 1000, g_cGlobalConfig.m_dwMediaSpaceLimit*1024*1024 );

	KMedia::ResetTaskCount();
}

bool CheckFont(LPCSTR typeface, DWORD char_set, HWND hwnd)
{
	//create font
	LOGFONT font_info;
	memset(&font_info, 0, sizeof(font_info));
	strcpy(font_info.lfFaceName, typeface);
	font_info.lfCharSet = (BYTE)char_set;
	HFONT hfont = CreateFontIndirect(&font_info);
	if(!hfont)
		return false;

	//create memdc
	HDC dc = GetDC(hwnd);
	HFONT hfont_old = (HFONT)SelectObject(dc, hfont);
	char real_typeface[256];
	GetTextFace(dc, 255, real_typeface);
	SelectObject(dc, hfont_old);
	ReleaseDC(hwnd, dc);
	DeleteObject(hfont);
	return (strcmp( typeface, real_typeface ) == 0);
}

bool KShell::CreateAFont(LPCSTR pszFontFile, DWORD CharaSet, INT nId)
{
	if(pszFontFile[0] == '#')
	{
		INT shared_id = atoi(&pszFontFile[1]);
		//add nId
		if(shared_id != nId)
		{
			map<INT, DWORD>::iterator p = m_mapFont.find(shared_id);
			if( p == m_mapFont.end() ) 
				return false;
			m_mapFont[nId] = p->second;
			return true;
		}
	}

	if(pszFontFile[0] == '@')
	{
		char* now_check = (char*)&pszFontFile[1];
		while( true )
		{
			char face[256];
			INT i = 0;
			while(now_check[i] != '@' && now_check[i] != ',' && now_check[i] != ' ' && now_check[i] != 0)
			{
				face[i] = now_check[i];
				i++;
			}
			face[i] = 0;

			if( CheckFont( face, CharaSet, Gpu()->GetCreationParameters()->hWnd ) )
				break;

			char* a = strchr(now_check, '@');
			if( !a )
			{
				now_check = 0;
				break;
			}

			now_check = &a[1];
		}

		if( now_check == 0 )
			now_check = (char*)&pszFontFile[1];

		char c[256];
		INT i = 0;
		while(now_check[i] != '@' && now_check[i] != 0)
		{
			c[i] = now_check[i];
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

	DWORD fonttype = (CharaSet == 3) ? 2 : 1;
	m_mapFont[nId] = FindOrCreateMedia(g_dwMediaTypeFont, pszFontFile, &fonttype);
	return true;
}

bool RectIntersect(const RECT* rc1, const RECT* rc2, RECT* out)
{
	out->left = max( rc1->left, rc2->left );
	out->right = min( rc1->right, rc2->right );
	out->top = max( rc1->top, rc2->top );
	out->bottom = min( rc1->bottom, rc2->bottom );
	return (out->right > out->left) && (out->bottom > out->top);
}

void KShell::OutputText(INT nFontId, const WCHAR* psText, INT nCount, INT nX, INT nY, DWORD Color, INT nLineWidth, INT nZ, DWORD BorderColor, const RECT* pClipRect)
{
	if(!psText || !psText[0]) 
		return;

	if(NULL != m_pJxPlayer && 
		(m_pJxPlayer->GetCurState() == enum_Replay_Recing || 
		m_pJxPlayer->GetCurState() == -1))
	{
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_OutPutText, 
			"npcxyolzb",
			nFontId, psText, 
			nCount, nX, nY, 
			Color, nLineWidth, 
			nZ, BorderColor);
	}

	map<INT, DWORD>::iterator p = m_mapFont.find(nFontId);
	if( p == m_mapFont.end() )
		return;

	KTextCmd cmd;
	cmd.dwFontHandle = p->second;
	cmd.dwOptions = 0;
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

	if( pClipRect )
	{
		RECT canvas_rect = {0, 0, Gpu()->GetPresentationParameters()->dwBackbufferWidth, Gpu()->GetPresentationParameters()->dwBackbufferHeight};
		if( !RectIntersect( &canvas_rect, pClipRect, &g_sScissorRect ) )
		{
			return;
		}
	}

	if( pClipRect )
	{
		ProcessCmd(0, 0, 0);

		g_bScissorTestEnable = true;
		if( Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
		{
			Gpu()->SetScissorRect(&g_sScissorRect);
		}
	}

	ProcessCmd(g_dwKernelTypeText, &cmd, 0);

	//flush for space chars
	if( nZ != -32767 )
	{
		ProcessCmd( 0, 0, 0 );
	}

	if( pClipRect )
	{
		ProcessCmd(0, 0, 0);
		g_bScissorTestEnable = false;
		if( Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
		{
			Gpu()->SetScissorRect(0);
		}
	}
}

INT KShell::OutputRichText(INT nFontId, void* pParam, const WCHAR* psText, INT nCount, INT nLineWidth, const RECT* pClipRect)
{
	if(!pParam || !psText || !psText[0]) 
		return 0;

	if(NULL != m_pJxPlayer && 
		(m_pJxPlayer->GetCurState() == enum_Replay_Recing || 
		m_pJxPlayer->GetCurState() == -1))
	{
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_OutPutRichText, 
			"npscl", nFontId, pParam, 
			psText, nCount, nLineWidth);
	}

	map<INT, DWORD>::iterator p = m_mapFont.find(nFontId);	// <-- use priority list instead?
	if( p == m_mapFont.end() )
		return 0;

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
	KOutputTextParam* param = (KOutputTextParam*)pParam;

	KTextCmd cmd;
	cmd.dwFontHandle = p->second;
	cmd.dwOptions = emTEXT_CMD_COLOR_TAG | emTEXT_CMD_INLINE_PICTURE | emTEXT_CMD_UNDERLINE_TAG;
	//	cmd.text = (byte*)psText;
	cmd.pwchText = psText;
	cmd.dwBytes = (DWORD)( (nCount > 0) ? nCount : UINT_MAX );
	cmd.nLineWidthLimit = (nLineWidth > 0) ? nLineWidth : INT_MAX;
	cmd.dwStartLine = param->nSkipLine;
	cmd.dwEndLine = param->nSkipLine + param->nNumLine;
	cmd.nLineInterSpace = nFontId + param->nRowSpacing;
	cmd.nCharInterSpace = nFontId;
	cmd.dwColor = param->Color;
	cmd.dwBorderColor = param->BorderColor;
	cmd.fInlinePicScaling = param->nPicStretchPercent/100.f;
	cmd.nFontId = nFontId;

	if( param->nZ == -32767 )	// TEXT_IN_SINGLE_PLANE_COORD
	{
		cmd.cPos = KPOINT(param->nX, param->nY);
	}
	else
	{
		cmd.cPos = KPOINT(param->nX, param->nY);
		SpaceToScreen(cmd.cPos.x, cmd.cPos.y, param->nZ);
	}

	if( pClipRect )
	{
		RECT canvas_rect = {0, 0, Gpu()->GetPresentationParameters()->dwBackbufferWidth, Gpu()->GetPresentationParameters()->dwBackbufferHeight};
		if( !RectIntersect( &canvas_rect, pClipRect, &g_sScissorRect ) )
		{
			return 0;
		}
	}

	if( pClipRect )
	{
		ProcessCmd(0, 0, 0);

		g_bScissorTestEnable = true;
		if( Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
		{
			Gpu()->SetScissorRect(&g_sScissorRect);
		}
	}

	ProcessCmd(g_dwKernelTypeText, &cmd, 0);

	//flush for space chars
	if( param->nZ != -32767 )
	{
		ProcessCmd( 0, 0, 0 );
	}

	//process underline
	if( !g_vecUnderLineCmd.empty() )
	{
		for( DWORD i = 0; i < g_vecUnderLineCmd.size(); i++ )
		{
			if( pClipRect && !Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
			{
				if( g_vecUnderLineCmd[i].sPos1.x < g_sScissorRect.left || g_vecUnderLineCmd[i].sPos2.x > g_sScissorRect.right || 
					g_vecUnderLineCmd[i].sPos1.y < g_sScissorRect.top || g_vecUnderLineCmd[i].sPos1.y > g_sScissorRect.bottom )
				{
					continue;
				}
			}

			ProcessCmd(g_dwKernelTypeLine, &g_vecUnderLineCmd[i], 0 );
		}
		g_vecUnderLineCmd.clear();
	}

	if( pClipRect )
	{
		ProcessCmd(0, 0, 0);
		g_bScissorTestEnable = false;
		if( Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
		{
			Gpu()->SetScissorRect(0);
		}
	}

	return 0;
}

//image lib
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

		//check size for head
		if( pFiledata->GetBufferSize() < sizeof(KHead) )
		{
			SAFE_RELEASE( pFiledata );
			return false;
		}

		//load head info
		KHead* pHead = (KHead*)pFiledata->GetBufferPointer();

		struct KInfoRect
		{
			short	shFrameCount;
			short	shInterval;
			short	shWidth;
			short	shHeight;
			short	shCenter_x;
			short	shCenterY;
			short	shDirectionCount;
			BYTE	byBlendStyle;
		};

		KInfoRect* pSprInfo = (KInfoRect*)pImageData;
		if( pSprInfo )
		{
			pSprInfo->shFrameCount		= (short)pHead->wFrameCount;
			pSprInfo->shInterval			= (short)pHead->wInterval;
			pSprInfo->shWidth				= (short)pHead->wWidth;
			pSprInfo->shHeight			= (short)pHead->wHeight;
			pSprInfo->shCenter_x			= (short)pHead->wCenterX;
			pSprInfo->shCenterY			= (short)pHead->wCenterY;
			pSprInfo->shDirectionCount	= (short)pHead->wDirectionCount;
			pSprInfo->byBlendStyle		= (BYTE)(pHead->wReserved[1] & 0xff);
		}

		SAFE_RELEASE(pFiledata);
		return true;
	}

	DWORD dwHandle = FindOrCreateMedia( ((nType == emPIC_TYPE_SPR) ?  g_dwMediaTypeSpr : g_dwMediaTypeJpg), pszImage, 0);

	KSprite* pSpr = 0;
	if( !QueryProduct(dwHandle, true, (void**)&pSpr) || !pSpr )
		return false;

	struct KInfoRet
	{
		short	shFrameCount;
		short	shInterval;
		short	shWidth;
		short	shHeight;
		short	shCenterX;
		short	shCenterY;
		short	shDirectionCount;
		BYTE	byBlendStyle;
	};

	KInfoRet* pSprInfo		= (KInfoRet*)pImageData;
	if( pSprInfo )
	{
		pSprInfo->shFrameCount		= (short)pSpr->GetFrameCount();
		pSprInfo->shInterval			= (short)pSpr->GetInterval();
		pSprInfo->shWidth				= (short)pSpr->GetSize()->x;
		pSprInfo->shHeight			= (short)pSpr->GetSize()->y;
		pSprInfo->shCenterX			= (short)pSpr->GetCenter()->x;
		pSprInfo->shCenterY			= (short)pSpr->GetCenter()->y;
		pSprInfo->shDirectionCount	= (short)pSpr->GetDirectionCount();
		pSprInfo->byBlendStyle		= (BYTE)pSpr->GetBlendStyle();
	}

	return true;
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
			KHead* head = (KHead*)pFiledata->GetBufferPointer();

			if( nFrame >= head->wFrameCount )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}

			if( pFiledata->GetBufferSize() < sizeof(KHead) + 3*256 + sizeof(KOffsetTable)*head->wFrameCount )
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

			if( pFiledata->GetBufferSize() < sizeof(KHead) + 3*256 + sizeof(KOffsetTable)*head->wFrameCount + offtbl[nFrame].dwOffset + offtbl[nFrame].dwSize )
			{
				SAFE_RELEASE(pFiledata);
				return false;
			}

			pFrameInfo = (KFrameInfo*)(((BYTE*)pFiledata->GetBufferPointer()) + sizeof(KHead) + 3*256 + sizeof(KOffsetTable)*head->wFrameCount + offtbl[nFrame].dwOffset);
		}

		if( pOffset )
			*pOffset = KPOINT(pFrameInfo->wOffsetX, pFrameInfo->wOffsetY);
		if( pSize )
			*pSize = KPOINT(pFrameInfo->wWidth, pFrameInfo->wHeight);

		SAFE_RELEASE(pFiledata);
		return true;
	}

	DWORD handle = FindOrCreateMedia( ((nType == emPIC_TYPE_SPR) ?  g_dwMediaTypeSpr : g_dwMediaTypeJpg), pszImage, 0);

	KSprite* spr = 0;
	if( !QueryProduct(handle, true, (void**)&spr) || !spr )
		return false;

	if( nFrame < 0 || nFrame >= (INT)spr->GetFrameCount() )
		return false;

	KSpriteFrame* frame = 0;
	if( !spr->QueryFrame(handle, nFrame, true, (void**)&frame) || !frame )
		return false;

	if( pOffset )
		*pOffset = KPOINT(frame->GetFrameOffset()->x, frame->GetFrameOffset()->y);
	if( pSize )
		*pSize = KPOINT(frame->GetFrameSize()->x, frame->GetFrameSize()->y);

	return true;
}

INT	KShell::GetImagePixelAlpha(LPCSTR pszImage, INT nFrame, INT nX, INT nY, INT nType)
{
	DWORD dwHandle = FindOrCreateMedia( ((nType == emPIC_TYPE_SPR) ?  g_dwMediaTypeSpr : g_dwMediaTypeJpg), pszImage, 0);

	KSprite* spr = 0;
	if( !QueryProduct(dwHandle, true, (void**)&spr) || !spr )
		return 0;

	if(nFrame < 0 || nFrame >= (INT)spr->GetFrameCount())
		return 0;

	KSpriteFrame* pFrame = 0;
	if( !spr->QueryFrame(dwHandle, nFrame, true, (void**)&pFrame) || !pFrame )
		return 0;

	nX -= pFrame->GetFrameOffset()->x;
	nY -= pFrame->GetFrameOffset()->y;
	if( nX < 0 || nX >= pFrame->GetTextureSize()->x || nY < 0 || nY >= pFrame->GetTextureSize()->y )
		return 0;
	else
		return 255;
}

//dynamic bitmaps
DWORD KShell::CreateImage(LPCSTR pszName, INT nWidth, INT nHeight, DWORD nType)
{
	if (m_pJxPlayer != NULL)
	{
		m_pJxPlayer->RECjxr(m_nFrameCounter, enum_RepreFun_CreateImage, 
			"pnwh", pszName, nType, nWidth, nHeight);
	}

	switch( nType )
	{
	case emPIC_TYPE_RT:
		return FindOrCreateMedia(g_dwMediaTypeRt, pszName, &KPOINT(nWidth, nHeight));
	case emPIC_TYPE_JPG_OR_LOCKABLE:
		return FindOrCreateMedia(g_dwMediaTypeLockable, pszName, &KPOINT(nWidth, nHeight));
	case emPIC_TYPE_SPR:
	{
		DWORD dwHandle = FindOrCreateMedia(g_dwMediaTypeSpr, pszName, 0, 1);
		QueryProductWithoutReturn(dwHandle);
		return dwHandle;
	}
		
	default:
		return 0;
	}
}

void KShell::ClearImageData(LPCSTR pszImage, DWORD uImage, DWORD nImagePosition)
{
	if(NULL != m_pJxPlayer && 
		(m_pJxPlayer->GetCurState() == enum_Replay_Recing || 
		m_pJxPlayer->GetCurState() == enum_Replay_PauseRec || 
		m_pJxPlayer->GetCurState() == -1))
	{
		m_pJxPlayer->RECjxr(
			m_nFrameCounter, 
			enum_RepreFun_ClearImageData, 
			"pun", 
			pszImage, 
			uImage, 
			nImagePosition);
	}

	//find correct handle
	DWORD dwHandle = 0;
	if( CheckHandle(uImage) == g_dwMediaTypeRt )
		dwHandle = uImage;
	else if( CheckHandle(nImagePosition) == g_dwMediaTypeRt )
		dwHandle = nImagePosition;
	else
		dwHandle = FindMedia(g_dwMediaTypeRt, pszImage);

	if( !dwHandle )
		return;

	//query product
	KSprite* pSpr = 0;
	if( !QueryProduct(dwHandle, true, (void**)&pSpr) || !pSpr )
		return;

	KSpriteFrame* pFrame = 0;
	if( !pSpr->QueryFrame(dwHandle, 0, true, (void**)&pFrame) || !pFrame )
		return;

	//check texture
	if( !pFrame->GetTexture() )
		return;

	//clear
	LPD3DSURFACE pSurf = 0;
	pFrame->GetTexture()->GetSurfaceLevel(0, &pSurf);
	Gpu()->ColorFill(pSurf, 0, 0);
	SAFE_RELEASE(pSurf);
}

void* KShell::GetBitmapDataBuffer(LPCSTR pszImage, void* pInfo)
{
	DWORD dwHandle = FindMedia( g_dwMediaTypeLockable, pszImage );
	if( !dwHandle )
		return 0;

	KSprite* pSpr = 0;
	if( !QueryProduct(dwHandle, true, (void**)&pSpr) || !pSpr )
		return 0;

	KSpriteFrame* pBaseFrame = 0;
	if( !pSpr->QueryFrame(dwHandle, 0, true, (void**)&pBaseFrame) || !pBaseFrame )
		return 0;

	KLockableFrame* pFrame = dynamic_cast<KLockableFrame*>(pBaseFrame);

	struct KBitmapDataBuffInfo
	{
		INT		nWidth;
		INT		nHeight;
		INT		nPitch;	
		DWORD	eFormat;
		void*	pData;
	}; 

	DWORD wPitch;
	void* bits = pFrame->Lock( &wPitch );
	if( bits && pInfo )
	{
		KBitmapDataBuffInfo* pOut = (KBitmapDataBuffInfo*)pInfo;

		pOut->nWidth = pFrame->GetFrameSize()->x;
		pOut->nHeight = pFrame->GetFrameSize()->y;
		pOut->nPitch = wPitch;
		pOut->eFormat = (g_sLockableConfig.eFmt == D3DFMT_R5G6B5) ? 2 : 1;
		pOut->pData = bits;
	}

	return bits;
}

void KShell::ReleaseBitmapDataBuffer(LPCSTR pszImage, void* pBuffer)
{
	DWORD dwHandle = FindMedia( g_dwMediaTypeLockable, pszImage );
	if( !dwHandle )
		return;

	KSprite* pSpr = 0;
	if( !QueryProduct(dwHandle, true, (void**)&pSpr) || !pSpr )
		return;

	KSpriteFrame* pBaseFrame = 0;
	if( !pSpr->QueryFrame(dwHandle, 0, true, (void**)&pBaseFrame) || !pBaseFrame )
		return;

	KLockableFrame* frame = dynamic_cast<KLockableFrame*>(pBaseFrame);

	frame->UnLock();
}


void KShell::DrawPrimitives(INT nPrimitiveCount, void* pPrimitives, DWORD uGenre, INT bSinglePlaneCoord, KPRIMITIVE_INDEX_LIST** pStandby)
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

	if( !pPrimitives || !nPrimitiveCount )
		return;

	switch( uGenre )
	{
	case emPRIMITIVE_TYPE_PIC:
	case emPRIMITIVE_TYPE_PIC_PART:
	case emPRIMITIVE_TYPE_PIC_PART_FOUR:
		{
			g_dwCallTimes1++;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				KPicCmd* cmd;
				switch( uGenre )
				{
				case emPRIMITIVE_TYPE_PIC:
					cmd = &((KPicCmd*)pPrimitives)[i];
					break;
				case emPRIMITIVE_TYPE_PIC_PART:
					cmd = &((KPicCmdPart*)pPrimitives)[i];
					break;
				case emPRIMITIVE_TYPE_PIC_PART_FOUR:
					cmd = &((KPicCmdPartFour*)pPrimitives)[i];
					break;
				}

				//KGLogPrintf(KGLOG_INFO,"[pic] %s, %x,%d,%d,%d,%d.", cmd->filename, cmd->exchange_color, cmd->frame_index,
				//	cmd->pic_type, cmd->ref_method, cmd->render_style);

				if( !cmd->szFileName || !cmd->szFileName[0] )
					continue;

				D3DXVECTOR3 sPosSaved;
				if( !bSinglePlaneCoord )
				{
					sPosSaved = cmd->sPos1;
					KPOINT out;
					out.x = (INT)cmd->sPos1.x;
					out.y = (INT)cmd->sPos1.y;
					SpaceToScreen(out.x, out.y, (INT)cmd->sPos1.z);
					cmd->sPos1.x = (float)out.x;
					cmd->sPos1.y = (float)out.y;
				}

				KPicCmdArg arg;
				arg.bPart = (uGenre == emPRIMITIVE_TYPE_PIC_PART || uGenre == emPRIMITIVE_TYPE_PIC_PART_FOUR);
				arg.pStandByList = pStandby ? pStandby[i] : 0;
				ProcessCmd(g_dwKernelTypePic, cmd, &arg);

				if( !bSinglePlaneCoord )
				{
					cmd->sPos1 = sPosSaved;
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_RECT:
		{
			KRectCmd* cmds = (KRectCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				D3DXVECTOR3 sPosSaved1;
				D3DXVECTOR3 sPosSaved2;
				if( !bSinglePlaneCoord )
				{
					sPosSaved1 = cmds[i].sPos1;
					KPOINT out1;
					out1.x = (INT)cmds[i].sPos1.x;
					out1.y = (INT)cmds[i].sPos1.y;
					SpaceToScreen(out1.x, out1.y, (INT)cmds[i].sPos1.z);
					cmds[i].sPos1.x = (float)out1.x;
					cmds[i].sPos1.y = (float)out1.y;

					sPosSaved2 = cmds[i].sPos2;
					KPOINT out2;
					out2.x = (INT)cmds[i].sPos2.x;
					out2.y = (INT)cmds[i].sPos2.y;
					SpaceToScreen(out2.x, out2.y, (INT)cmds[i].sPos2.z);
					cmds[i].sPos2.x = (float)out2.x;
					cmds[i].sPos2.y = (float)out2.y;
				}

				ProcessCmd( g_dwKernelTypeRect, &cmds[i], 0 );

				if( !bSinglePlaneCoord )
				{
					cmds[i].sPos1 = sPosSaved1;
					cmds[i].sPos2 = sPosSaved2;
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_LINE:
		{
			KLineCmd* cmds = (KLineCmd*)pPrimitives;
			for( INT i = 0; i < nPrimitiveCount; i++ )
			{
				D3DXVECTOR3 cPosSaved1;
				D3DXVECTOR3 cPosSaved2;
				if( !bSinglePlaneCoord )
				{
					cPosSaved1 = cmds[i].sPos1;
					KPOINT out1;
					out1.x = (INT)cmds[i].sPos1.x;
					out1.y = (INT)cmds[i].sPos1.y;
					SpaceToScreen(out1.x, out1.y, (INT)cmds[i].sPos1.z);
					cmds[i].sPos1.x = (float)out1.x;
					cmds[i].sPos1.y = (float)out1.y;

					cPosSaved2 = cmds[i].sPos2;
					KPOINT out2;
					out2.x = (INT)cmds[i].sPos2.x;
					out2.y = (INT)cmds[i].sPos2.y;
					SpaceToScreen(out2.x, out2.y, (INT)cmds[i].sPos2.z);
					cmds[i].sPos2.x = (float)out2.x;
					cmds[i].sPos2.y = (float)out2.y;
				}

				ProcessCmd( g_dwKernelTypeLine, &cmds[i], 0 );

				if( !bSinglePlaneCoord )
				{
					cmds[i].sPos1 = cPosSaved1;
					cmds[i].sPos2 = cPosSaved2;
				}
			}
		}
		break;
	case emPRIMITIVE_TYPE_RECT_FRAME:
		{
			KRectFrameCmd* cmds = (KRectFrameCmd*)pPrimitives;
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

				KLineCmd cLineCmd[4];
				cLineCmd[0].sPos1 = D3DXVECTOR3((float)nX1, (float)nY1, 0);
				cLineCmd[0].sPos2 = D3DXVECTOR3((float)nX2, (float)nY1, 0);
				cLineCmd[0].dwColor1 = cLineCmd[0].dwColor2 = cmds[i].dwColor;

				cLineCmd[1].sPos1 = cLineCmd[0].sPos2;
				cLineCmd[1].sPos2 = D3DXVECTOR3((float)nX2, (float)nY2, 0);
				cLineCmd[1].dwColor1 = cLineCmd[1].dwColor2 = cmds[i].dwColor;

				cLineCmd[2].sPos1 = cLineCmd[1].sPos2;
				cLineCmd[2].sPos2 = D3DXVECTOR3((float)nX1, (float)nY2, 0);
				cLineCmd[2].dwColor1 = cLineCmd[2].dwColor2 = cmds[i].dwColor;

				cLineCmd[3].sPos1 = cLineCmd[2].sPos2;
				cLineCmd[3].sPos2 = cLineCmd[0].sPos1;
				cLineCmd[3].dwColor1 = cLineCmd[3].dwColor2 = cmds[i].dwColor;

				//redirect
				for( INT i = 0; i < 4; i++ )
				{
					ProcessCmd( g_dwKernelTypeLine, &cLineCmd[i], 0);
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

	if( !nPrimitiveCount || !pPrimitives )
		return;

	switch( uGenre )
	{
	case emPRIMITIVE_TYPE_PIC:		//screen picture
		{
			//find correct handle
			DWORD wHandle;
			if( CheckHandle(uImage) == g_dwMediaTypeRt )
				wHandle = uImage;
			else if( CheckHandle(nImagePosition) == g_dwMediaTypeRt )
				wHandle = nImagePosition;
			else
				wHandle = FindMedia(g_dwMediaTypeRt, pszImage);

			if( !wHandle )
				break;

			//save handle
			nImagePosition = wHandle;

			//query product
			KSprite* spr = 0;
			if( !QueryProduct(wHandle, true, (void**)&spr) || !spr )
				break;

			KSpriteFrame* pFrame = 0;
			if( !spr->QueryFrame(wHandle, 0, true, (void**)&pFrame) || !pFrame )
				break;

			//check texture
			if( !pFrame->GetTexture() )
				break;

			//save old render target
			LPD3DSURFACE pRTSaved = 0;
			Gpu()->GetRenderTarget(&pRTSaved);

			//flush before changing global states
			ProcessCmd(0, 0, 0);

			//set new render target
			LPD3DSURFACE pNewRT = 0;
			pFrame->GetTexture()->GetSurfaceLevel(0, &pNewRT);
			Gpu()->SetRenderTarget(pNewRT);
			SAFE_RELEASE(pNewRT);

			//apply ground LOD
			g_sPicPostScaling.x = pFrame->GetTextureSize()->x * pFrame->GetUVScale()->x / pFrame->GetFrameSize()->x;
			g_sPicPostScaling.y = pFrame->GetTextureSize()->y * pFrame->GetUVScale()->y / pFrame->GetFrameSize()->y;

			g_bForceDrawPic = TRUE;

			//draw
			DrawPrimitives(nPrimitiveCount, pPrimitives, uGenre, true);

			g_bForceDrawPic = FALSE;

			//restore LOD
			g_sPicPostScaling = D3DXVECTOR2(1, 1);

			//flush before changing global states
			ProcessCmd(0, 0, 0);

			//restore old render target
			Gpu()->SetRenderTarget(pRTSaved);
			SAFE_RELEASE(pRTSaved);
		}
		break;
	}
}

//jpg loader
void* KShell::GetJpgImage(const char cszName[], unsigned uRGBMask16)
{
	LPD3DBUFFER pFileData = 0;
	if( !LoadFile(cszName, &pFileData) || !pFileData )
		return 0;

	D3DXIMAGE_INFO sJpgInfo;
	if( FAILED(D3DXGetImageInfoFromFileInMemory(pFileData->GetBufferPointer(), pFileData->GetBufferSize(), &sJpgInfo)) )
	{
		pFileData->Release();
		return 0;
	}

	LPD3DSURFACE pSurf = 0;
	if( !Gpu()->CreateOffscrSurface(sJpgInfo.Width, sJpgInfo.Height, g_sLockableConfig.eFmt, D3DPOOL_SYSTEMMEM, &pSurf) )
	{
		pFileData->Release();
		return 0;
	}

	RECT rc = {0, 0, sJpgInfo.Width, sJpgInfo.Height};
	if( FAILED(D3DXLoadSurfaceFromFileInMemory(pSurf, 0, &rc, pFileData->GetBufferPointer(), pFileData->GetBufferSize(), &rc, D3DX_FILTER_NONE, 0, 0)) )
	{
		pSurf->Release();
		pFileData->Release();
		return 0;
	}
	SAFE_RELEASE(pFileData);

	struct KSGImageContent
	{
		INT		nWidth;
		INT		nHeight;
		WORD	wData[1];
	};

	KSGImageContent *pImageResult = (KSGImageContent*)new CHAR[sizeof(INT)*2 + sJpgInfo.Width * sJpgInfo.Height * 2]; 
	pImageResult->nWidth = sJpgInfo.Width;
	pImageResult->nHeight = sJpgInfo.Height;

	D3DLOCKED_RECT sLockedRect;
	pSurf->LockRect(&sLockedRect, 0, D3DLOCK_READONLY);
	for( DWORD row = 0; row < sJpgInfo.Height; row++ )
	{
		memcpy(pImageResult->wData + row * sJpgInfo.Width, (BYTE*)sLockedRect.pBits + sLockedRect.Pitch * row, sJpgInfo.Width * 2);
	}
	pSurf->UnlockRect();
	pSurf->Release();

	return pImageResult;
}

void KShell::ReleaseImage(void* pImage)
{
	SAFE_DELETE_ARRAY(pImage);
}

//screen capture
bool SaveToBmpFileInMemory_8888(LPD3DBUFFER* out, void* pBitmap, INT nPitch, INT nWidth, INT nHeight)
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
	BITMAPINFOHEADER* pInfoHeader = (BITMAPINFOHEADER*)&FileHeader[1];

	pInfoHeader->biSize          = sizeof(BITMAPINFOHEADER);
	pInfoHeader->biWidth         = nWidth;
	pInfoHeader->biHeight        = nHeight;
	pInfoHeader->biPlanes        = 1;
	pInfoHeader->biBitCount      = 24;
	pInfoHeader->biCompression   = 0;
	pInfoHeader->biSizeImage     = 0;
	pInfoHeader->biXPelsPerMeter = 0xb40;
	pInfoHeader->biYPelsPerMeter = 0xb40;
	pInfoHeader->biClrUsed       = 0;
	pInfoHeader->biClrImportant  = 0;

	// encode bitmap
	BYTE* lpDes = (BYTE*)&pInfoHeader[1];
	BYTE* lpSrc = (BYTE*)pBitmap;

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

	*out = pBuffer;
	return true;
}

bool SaveToBmpFile_8888(LPCSTR lpFileName, void* pBitmap, INT nPitch, INT nWidth, INT nHeight)
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

//jepg encoder
INT GetEncoderClsid(const wchar_t* psFormat, CLSID* pClsid)
{
	unsigned int  nNum = 0;          // number of image encoders
	unsigned int  nSize = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&nNum, &nSize);
	if(nSize == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*) new CHAR[nSize]; 
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(nNum, nSize, pImageCodecInfo);

	for(DWORD j = 0; j < nNum; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, psFormat) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			delete[] (CHAR*)pImageCodecInfo;
			return j;  // Success
		}    
	}

	delete[] (CHAR*)pImageCodecInfo;
	return -1;  // Failure
}

bool SaveToJpgFile_8888(LPCSTR lpFileName, void* pBitmap, INT nPitch, INT nWidth, INT nHeight, DWORD nQuality)
{
	LPD3DBUFFER pFileData = 0;
	if( !SaveToBmpFileInMemory_8888(&pFileData, pBitmap, nPitch, nWidth, nHeight) )
		return false;

	LPSTREAM pStream = 0;
	if( FAILED(CreateStreamOnHGlobal(NULL, true, &pStream)) )
	{
		SAFE_RELEASE(pFileData);
		return false;
	}

	ULARGE_INTEGER unStreamSize;
	unStreamSize.HighPart = 0;
	unStreamSize.LowPart = pFileData->GetBufferSize();
	pStream->SetSize(unStreamSize);
	pStream->Write(pFileData->GetBufferPointer(), pFileData->GetBufferSize(), 0);
	SAFE_RELEASE(pFileData);

	// Initialize GDI+.
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Get the CLSID of the JPEG encoder.
	CLSID sEncoderClsid;
	if( GetEncoderClsid(L"image/jpeg", &sEncoderClsid) == -1 )
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

	wchar_t szWFileName[512];
	MultiByteToWideChar(CP_ACP, 0, lpFileName, (INT)strlen(lpFileName)+1, szWFileName, 510);

	Image* pImage = ::new Image(pStream);
	bool bResult = (pImage->Save(szWFileName, &sEncoderClsid, &cEncoderParameters) == Ok);

	::delete pImage;
	SAFE_RELEASE(pStream);
	GdiplusShutdown(gdiplusToken);

	return bResult;
}

bool KShell::SaveScreenToFile(LPCSTR pszName, DWORD eType, DWORD nQuality)
{
	RECT sSrcRect;
	D3DDISPLAYMODE sDispMode;
	Gpu()->GetCurrentDisplayMode(&sDispMode);

	if( Gpu()->GetPresentationParameters()->bWindowed )
	{
		RECT sClientRect;
		GetClientRect(Gpu()->GetCreationParameters()->hWnd, &sClientRect);
		POINT cPoint[2];
		cPoint[0].x = sClientRect.left;
		cPoint[0].y = sClientRect.top;
		cPoint[1].x = sClientRect.right;
		cPoint[1].y = sClientRect.bottom;
		ClientToScreen(Gpu()->GetCreationParameters()->hWnd, &cPoint[0]);
		ClientToScreen(Gpu()->GetCreationParameters()->hWnd, &cPoint[1]);

		if( cPoint[0].x < 0 )
			cPoint[0].x = 0;
		if( cPoint[0].y < 0 )
			cPoint[0].y = 0;
		if( cPoint[1].x > (INT)sDispMode.Width )
			cPoint[1].x = (INT)sDispMode.Width - 1;
		if( cPoint[1].y > (INT)sDispMode.Height )
			cPoint[1].y = (INT)sDispMode.Height - 1;

		SetRect(&sSrcRect, cPoint[0].x, cPoint[0].y, cPoint[1].x, cPoint[1].y);
	}
	else
	{
		SetRect(&sSrcRect, 0, 0, sDispMode.Width, sDispMode.Height);
	}

	LPD3DSURFACE pSurf = 0;
	if( !Gpu()->CreateOffscrSurface(sDispMode.Width, sDispMode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSurf) )
		return false;

	if( !Gpu()->GetFrontBufferData(pSurf) )
	{
		SAFE_RELEASE(pSurf);
		return false;
	}

	D3DLOCKED_RECT sLockedRect;
	if( FAILED(pSurf->LockRect(&sLockedRect, NULL, D3DLOCK_READONLY)) )
	{
		SAFE_RELEASE(pSurf);
		return false;
	}

	BYTE* pData = ((LPBYTE)sLockedRect.pBits) + sSrcRect.top * sLockedRect.Pitch + sSrcRect.left * 4;

	bool bRessult;
	if( eType == 0 )
		bRessult = SaveToBmpFile_8888(pszName, pData, sLockedRect.Pitch, sSrcRect.right - sSrcRect.left, sSrcRect.bottom - sSrcRect.top);
	else 
		bRessult = SaveToJpgFile_8888(pszName, pData, sLockedRect.Pitch, sSrcRect.right - sSrcRect.left, sSrcRect.bottom - sSrcRect.top, nQuality);

	pSurf->UnlockRect();
	SAFE_RELEASE(pSurf);

	return bRessult;
}

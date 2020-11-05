/* -------------------------------------------------------------------------
//	文件名		：	global_config_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	整个渲染部分的配置
//
// -----------------------------------------------------------------------*/
#include "global_config.h"
#include "../represent_common/async_queue.h"

KGlobalConfig g_cGlobalConfig;

void LoadGlobalConfig( const KRepresentConfig* pRepresentConfig)
{
	g_cGlobalConfig.m_dwLoaderThreadCount = pRepresentConfig->dwLoaderThreadCount;
	g_cGlobalConfig.m_dwMediaTimeLimit = pRepresentConfig->dwMediaTimeLimit;
	g_cGlobalConfig.m_dwMediaSpaceLimit = pRepresentConfig->dwMediaSpaceLimit;
	g_cGlobalConfig.m_dwLanguage = pRepresentConfig->dwLanguage;
	g_cGlobalConfig.m_bShowFps = pRepresentConfig->bShowFps;
	g_cGlobalConfig.m_bUseStackForLoader = pRepresentConfig->bUseStackForLoader;
	g_cGlobalConfig.m_dwPerFrameTaskCountLimit = pRepresentConfig->dwPerFrameTaskCountLimit;

	g_cGlobalConfig.m_dwTextureFormat = pRepresentConfig->dwTextureFormat;
	g_cGlobalConfig.m_dwMipmap = pRepresentConfig->dwMipmap;
	g_cGlobalConfig.m_bVerticalSync = pRepresentConfig->bVerticalSync;
	g_cGlobalConfig.m_bMultithreadedD3d = pRepresentConfig->bMultithreadedD3d;
	g_cGlobalConfig.m_bEnableConditionalNonPow2 = pRepresentConfig->bEnableConditionalNonPow2;
	g_cGlobalConfig.m_dwMaxBatchedQuadCount = pRepresentConfig->dwMaxBatchedQuadCount;

	switch( g_cGlobalConfig.m_dwTextureFormat )
	{
	case 0:
		g_cGlobalConfig.m_bUseCompressedTextureFormat = FALSE;
		g_cGlobalConfig.m_bUse16bitColor = FALSE;
		break;
	case 1:
		g_cGlobalConfig.m_bUseCompressedTextureFormat = FALSE;
		g_cGlobalConfig.m_bUse16bitColor = TRUE;
		break;
	default:
		g_cGlobalConfig.m_bUseCompressedTextureFormat = TRUE;
		g_cGlobalConfig.m_bUse16bitColor = TRUE;
		break;
	}

	g_cGlobalConfig.m_bLinearFilter = g_cGlobalConfig.m_dwMipmap > 0;

	switch( g_cGlobalConfig.m_dwLanguage )
	{
	case 0:
		g_cGlobalConfig.m_bDisableFullscreenIme = FALSE;
		g_cGlobalConfig.m_bSingleByteCharSet = FALSE;
		g_cGlobalConfig.m_dwFontTextureSize = 512;
		g_cGlobalConfig.m_bWordWrap = FALSE;
		g_cGlobalConfig.m_bTextJustification = FALSE;
		break;
	default:
		g_cGlobalConfig.m_bDisableFullscreenIme = TRUE;
		g_cGlobalConfig.m_bSingleByteCharSet = TRUE;
		g_cGlobalConfig.m_dwFontTextureSize = 256;
		g_cGlobalConfig.m_bWordWrap = TRUE;
		g_cGlobalConfig.m_bTextJustification = FALSE;
		break;
	}

	s_bUseStackForLoader = BOOL_bool(g_cGlobalConfig.m_bUseStackForLoader);
}
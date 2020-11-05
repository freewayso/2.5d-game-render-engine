/* -------------------------------------------------------------------------
//	文件名		：	global_config_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	整个渲染部分的配置
//
// -----------------------------------------------------------------------*/
#ifndef ___GLOBAL_CONFUG_T_H__
#define ___GLOBAL_CONFUG_T_H__
#include "../represent_common/common.h"

class KGlobalConfig
{
public:
	KGlobalConfig()
	{
		m_bUseStackForLoader = TRUE;
		m_bVerticalSync = FALSE;
		m_bDisableFullscreenIme = FALSE;
		m_dwLoaderThreadCount = 1;
		m_bMultithreadedD3d = TRUE;
		m_bUseCompressedTextureFormat = FALSE;
		m_bUse16bitColor = FALSE;
		m_bEnableConditionalNonPow2 = TRUE;
		m_dwMipmap = 0;
		m_bLinearFilter = FALSE;
		m_dwMaxBatchedQuadCount = 2000;
		m_dwMediaTimeLimit = 60;
		m_dwMediaSpaceLimit = 150;
		m_bSingleByteCharSet = FALSE;
		m_dwFontTextureSize = 512;
		m_bWordWrap = FALSE;
		m_bTextJustification = FALSE;
		m_bShowFps = FALSE;
		m_dwTextureFormat = 1;
		m_dwLanguage = 0;
		m_dwPerFrameTaskCountLimit = 10;
	}

	BOOL	m_bUseStackForLoader;
	BOOL	m_bVerticalSync;
	BOOL	m_bDisableFullscreenIme;
	DWORD	m_dwLoaderThreadCount;
	BOOL	m_bMultithreadedD3d;
	BOOL	m_bUseCompressedTextureFormat;
	BOOL	m_bUse16bitColor;
	BOOL	m_bEnableConditionalNonPow2;
	DWORD	m_dwMipmap;
	BOOL	m_bLinearFilter;
	DWORD	m_dwMaxBatchedQuadCount;
	DWORD	m_dwMediaTimeLimit;		//in seconds
	DWORD	m_dwMediaSpaceLimit;	//in megabytes
	BOOL	m_bSingleByteCharSet;
	DWORD	m_dwFontTextureSize;
	BOOL	m_bWordWrap;
	BOOL	m_bTextJustification;
	BOOL	m_bShowFps;
	DWORD	m_dwPerFrameTaskCountLimit;

	DWORD	m_dwTextureFormat;	//0 - 32bit, 1 - 16bit, else - compressed
	DWORD	m_dwLanguage;		//0 - chinese, else - others

};

extern KGlobalConfig g_cGlobalConfig;

void LoadGlobalConfig(const KRepresentConfig* pRepresentConfig);
#endif
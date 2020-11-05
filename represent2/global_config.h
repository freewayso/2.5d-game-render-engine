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
		m_dwAlphaLevel = 256;
		m_dwLoaderThreadCount = 1;
		m_dwMediaTimeLimit = 60;
		m_dwMediaSpaceLimit = 150;
		m_bSingleByteCharSet = FALSE;
		m_bWordWrap = FALSE;
		m_bTextJustification = FALSE;
		m_bShowFps = FALSE;
		m_dwLanguage = 0;
		m_dwPerFrameTaskCountLimit = 10;
	}

	BOOL	m_bUseStackForLoader;
	DWORD	m_dwAlphaLevel;
	DWORD	m_dwLoaderThreadCount;
	DWORD	m_dwMediaTimeLimit;		//in seconds
	DWORD	m_dwMediaSpaceLimit;	//in megabytes
	BOOL	m_bSingleByteCharSet;
	BOOL	m_bWordWrap;
	BOOL	m_bTextJustification;
	BOOL	m_bShowFps;
	DWORD	m_dwLanguage;
	DWORD	m_dwPerFrameTaskCountLimit;
};

extern KGlobalConfig g_cGlobalConfig;

void LoadGlobalConfig(const KRepresentConfig* pRepresentConfig);
#endif
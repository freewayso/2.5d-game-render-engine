/* -------------------------------------------------------------------------
//	文件名		：	ifile.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	IFile和IIniFile的定义 
//					其实就是对engined.dll中file和inifile的封装，对外提供的接口
// -----------------------------------------------------------------------*/
//implementation
#include "ifile.h"

HMODULE			g_hEngineDll = 0;
KSetRootPath	g_pfnSetRootPath = 0;
KCreateFile	g_pfnCreateFile = 0;
KOpenFile		g_pfnOpenFile = 0;
KOpenIniFile	g_pfnOpenIniFile = 0;

BOOL LoadEngineDll()
{
#ifdef _DEBUG
	g_hEngineDll = LoadLibrary("engined.dll");
#else
	g_hEngineDll = LoadLibrary("engine.dll");
#endif

	if( !g_hEngineDll )
		return FALSE;

	g_pfnSetRootPath	= (KSetRootPath)	GetProcAddress(g_hEngineDll, "g_SetRootPath");
	g_pfnCreateFile	= (KCreateFile)	GetProcAddress(g_hEngineDll, "g_CreateFile");
	g_pfnOpenFile		= (KOpenFile)		GetProcAddress(g_hEngineDll, "g_OpenFile");
	g_pfnOpenIniFile	= (KOpenIniFile)	GetProcAddress(g_hEngineDll, "g_OpenIniFile");
	//	get_ctrl_handle	= (GetCtrlHandle_t)	GetProcAddress(engine_dll, "GetCtrlHandle");//-->engined.dll改成使用text.dll,modify by wdb

	if ( !g_pfnSetRootPath || !g_pfnCreateFile || !g_pfnOpenFile || !g_pfnOpenIniFile)
	{
		ReleaseEngineDll();
		return FALSE;
	}

	g_pfnSetRootPath(0);
	return TRUE;
}

VOID ReleaseEngineDll()
{
	if( g_hEngineDll )
	{
		FreeLibrary( g_hEngineDll );
		g_hEngineDll = 0;
	}
	g_pfnSetRootPath = 0;
	g_pfnCreateFile = 0;
	g_pfnOpenFile = 0;
	g_pfnOpenIniFile = 0;
	//	get_ctrl_handle = 0;
}

BOOL LoadFile( IFile* pFile, LPD3DBUFFER* ppData )
{
	DWORD data_size = pFile->Size();
	if( !data_size )
		return FALSE;
	LPD3DBUFFER buf = 0;
	if( FAILED(D3DXCreateBuffer(data_size, &buf)) || !buf )
		return FALSE;
	VOID* data_ptr = buf->GetBufferPointer();
	if( data_size != pFile->Read(data_ptr, data_size) )
	{
		buf->Release();
		return FALSE;
	}
	*ppData = buf;
	return TRUE;
}

BOOL LoadFile(LPCSTR pszFileName, LPD3DBUFFER* ppData)
{
	IFile* pFile = g_pfnOpenFile(pszFileName, FALSE, FALSE);
	if( !pFile )
		return FALSE;
	BOOL ret = LoadFile(pFile, ppData);
	pFile->Release();
	return ret;
}

BOOL LoadFileFragment(IFile* pFile, INT nFragmentIndex, LPD3DBUFFER* ppData)
{
	DWORD data_size = pFile->GetFragmentSize(nFragmentIndex);
	if( !data_size )
		return FALSE;
	LPD3DBUFFER buf = 0;
	if( FAILED(D3DXCreateBuffer(data_size, &buf)) || !buf )
		return FALSE;
	VOID* data_ptr = buf->GetBufferPointer();
	if( data_size != pFile->ReadFragment(nFragmentIndex, data_ptr) )
	{
		buf->Release();
		return FALSE;
	}
	*ppData = buf;
	return TRUE;
}

BOOL LoadFileFragment(LPCSTR szFilename, INT nFragmentIndex, LPD3DBUFFER* ppData)
{
	IFile* pFile = g_pfnOpenFile(szFilename, FALSE, FALSE);
	if( !pFile )
		return FALSE;
	BOOL bRet = LoadFileFragment(pFile, nFragmentIndex, ppData);
	pFile->Release();
	return bRet;
}

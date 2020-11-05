/* -------------------------------------------------------------------------
//	文件名		：	ifile.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	IFile和IIniFile的定义 
//					其实就是对engined.dll中file和inifile的封装，对外提供的接口
// -----------------------------------------------------------------------*/
#ifndef __IFILE_H__
#define __IFILE_H__
#include "common.h"

class IFile
{
public:
	virtual DWORD	Read(VOID* Buffer, DWORD dwReadBytes) = 0;
	virtual DWORD	Write(const VOID* Buffer, DWORD dwWriteBytes) = 0;
	virtual VOID*	GetBuffer() = 0;
	virtual INT		Seek(INT nOffset, INT nOrigin) = 0;
	virtual INT		Tell() = 0;
	virtual DWORD	Size() = 0;
	virtual INT		IsFileInPak() = 0;
	virtual INT		IsPackedByFragment() = 0;
	virtual INT		GetFragmentCount() = 0;
	virtual DWORD	GetFragmentSize(INT nFragmentIndex) = 0;
	virtual DWORD	ReadFragment(INT nFragmentIndex, VOID*& pBuffer) = 0;
	virtual VOID	Close() = 0;
	virtual VOID	Release() = 0;
	virtual ~IFile() {};
};

class IIniFile
{
public:
	virtual INT		Save(LPCSTR pszFileName) = 0;
	virtual VOID	Clear() = 0;
	virtual VOID	Release() = 0;

	virtual INT		GetNextSection(LPCSTR pszSection, LPSTR pNextSection) = 0;
	virtual INT		GetNextKey(LPCSTR pszSection, LPCSTR pszKey, LPSTR pNextKey) = 0;	
	virtual INT		GetSectionCount() = 0;
	virtual INT		IsSectionExist(LPCSTR pszSection) = 0;
	virtual VOID	EraseSection(LPCSTR pszSection) = 0;
	virtual VOID	EraseKey(LPCSTR pszSection, LPCSTR pszKey) = 0;

	virtual INT		GetString(LPCSTR pszSection, LPCSTR pszKeyName, LPCSTR lpDefault, LPSTR lpRString, DWORD dwSize) = 0;
	virtual INT		GetInteger(LPCSTR pszSection, LPCSTR pszKeyName, INT nDefault, INT* pnValue) = 0;
	virtual INT		GetInteger2(LPCSTR	pszSection, LPCSTR pszKeyName, INT* pnValue1, INT* pnValue2) = 0;
	virtual INT		GetMultiInteger(LPCSTR	pszSection, LPCSTR pszKeyName, INT* pValues, INT nCount) = 0;
	virtual INT		GetMultiLong(LPCSTR pszSection, LPCSTR pszKeyName, INT* pValues, INT nCount) = 0;
	virtual INT		GetFloat(LPCSTR pszSection, LPCSTR pszKeyName, FLOAT fDefault, FLOAT* pfValue) = 0;
	virtual INT		GetFloat2(LPCSTR pszSection, LPCSTR pszKeyName, FLOAT* pfValue1, FLOAT* pfValue2) = 0;
	virtual INT		GetMultiFloat(LPCSTR pszSection, LPCSTR pszKeyName, FLOAT* pValues, INT nCount) = 0;
	virtual INT		GetStruct(LPCSTR pszSection, LPCSTR pszKeyName, VOID* lpStruct, DWORD dwSize) = 0;
	virtual INT		GetBool(LPCSTR	pszSection, LPCSTR pszKeyName, INT* pBool) = 0;

	virtual INT		WriteString(LPCSTR	pszSection, LPCSTR pszKeyName, LPCSTR pszString) = 0;
	virtual INT		WriteInteger(LPCSTR pszSection, LPCSTR pszKeyName, INT nValue) = 0;
	virtual INT		WriteInteger2(LPCSTR pszSection, LPCSTR pszKeyName, INT nValue1, INT nValue2) = 0;
	virtual INT		WriteMultiInteger(LPCSTR pszSection, LPCSTR pszKeyName, INT* pValues, INT nCount) = 0;
	virtual INT		WriteMultiLong(LPCSTR pszSection, LPCSTR pszKeyName, INT* pValues, INT nCount) = 0;
	virtual INT		WriteFloat(LPCSTR pszSection, LPCSTR pszKeyName, FLOAT fValue) = 0;
	virtual INT		WriteFloat2(LPCSTR	pszSection, LPCSTR pszKeyName, FLOAT fValue1, FLOAT fValue2) = 0;
	virtual INT		WriteMultiFloat(LPCSTR	pszSection, LPCSTR pszKeyName, FLOAT* pValues, INT nCount) = 0;
	virtual INT		WriteStruct(LPCSTR	pszSection, LPCSTR pszKeyName, VOID* lpStruct, DWORD dwSize) = 0;

	virtual ~IIniFile() {};
};

typedef	VOID			(*KSetRootPath)(LPCSTR pszPathName);
typedef	IFile*			(*KCreateFile)(LPCSTR pszFileName);
typedef	IFile*			(*KOpenFile)(LPCSTR pszFileName, INT nForceUnpakFile, INT nForWrite);
typedef	IIniFile*		(*KOpenIniFile)(LPCSTR pszFileName, INT nForceUnpakFile, INT nForWrite);
typedef IInlineControl* (*KGetCtrlHandle_t)(INT nCtrl);

extern HMODULE			g_hEngineDll;
extern KSetRootPath		g_pfnSetRootPath;
extern KCreateFile		g_pfnCreateFile;
extern KOpenFile		g_pfnOpenFile;
extern KOpenIniFile		g_pfnOpenIniFile;

BOOL			LoadEngineDll();
VOID			ReleaseEngineDll();
BOOL			LoadFile(IFile* pFile, LPD3DBUFFER* ppData);
BOOL			LoadFile(LPCSTR pszFileName, LPD3DBUFFER* ppData);
BOOL			LoadFileFragment(IFile* pFile, INT nFragmentIndex, LPD3DBUFFER* ppData);
BOOL			LoadFileFragment(LPCSTR pszFileName, INT nFragmentIndex, LPD3DBUFFER* ppData);

//GetCtrlHandle_t	get_ctrl_handle = 0;		-->engined.dll改成使用text.dll,modify by wdb
#endif
/* -------------------------------------------------------------------------
//	文件名		：	jpglib.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-24 11:11:11
//	功能描述	：	jpg库接口封装
//
// -----------------------------------------------------------------------*/
#include "jpglib.h"
CRITICAL_SECTION g_sJpglib_cs;

void InitJpglib() 
{ 
	InitializeCriticalSection(&g_sJpglib_cs); 
}

void ShutdownJpglib() 
{ 
	DeleteCriticalSection(&g_sJpglib_cs); 
}

void EnterJpglib() 
{ 
	EnterCriticalSection( &g_sJpglib_cs ); 
}

void LeaveJpglib() 
{ 
	LeaveCriticalSection( &g_sJpglib_cs ); 
}
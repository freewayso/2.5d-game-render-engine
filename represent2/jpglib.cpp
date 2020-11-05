/* -------------------------------------------------------------------------
//	�ļ���		��	jpglib.cpp
//	������		��	fenghewen
//	����ʱ��	��	2009-11-24 11:11:11
//	��������	��	jpg��ӿڷ�װ
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
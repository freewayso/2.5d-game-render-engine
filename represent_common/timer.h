/* -------------------------------------------------------------------------
//	�ļ���		��	timer.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-20 11:11:11
//	��������	��	��ʱ��
//
// -----------------------------------------------------------------------*/
//timer 
#ifndef __TIMER_H__
#define __TIMER_H__
#include "common.h"
#include <time.h>
extern DWORD	g_dwStartTime;
extern DWORD	g_dwNowTime;
inline VOID StartTimer() {g_dwStartTime = timeGetTime(); g_dwNowTime = 0;}
inline VOID UpdateTime() { g_dwNowTime = timeGetTime() - g_dwStartTime; }
inline INT NowTime() {return g_dwNowTime;}

#endif
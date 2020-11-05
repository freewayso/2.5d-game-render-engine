/* -------------------------------------------------------------------------
//	文件名		：	timer.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	计时器
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
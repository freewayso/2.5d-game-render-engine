/* -------------------------------------------------------------------------
//	文件名		：	kernel_center.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_center的定义
//
// -----------------------------------------------------------------------*/
#include "kernel_center.h"

vector<KernelSub>	g_vecSub;
DWORD 				g_dwCurSub;

//kernel types
DWORD g_dwKernelTypePic;
DWORD g_dwKernelTypeText;
DWORD g_dwKernelTypeRect;
DWORD g_dwKernelTypeLine;

DWORD InstallSubKernel(KernelInit pfnInit, void* pInitArg, KernelBatch pfnBatch, KernelFlush pfnFlush, KernelShutdown pfnShutdown)
{
	if( !pfnInit(pInitArg) )
		return 0;
	DWORD old_size = (DWORD)g_vecSub.size();
	g_vecSub.resize(old_size + 1);
	g_vecSub[old_size].pfnBatch = pfnBatch;
	g_vecSub[old_size].pfnFlush = pfnFlush;
	g_vecSub[old_size].pfnShutdown = pfnShutdown;
	return old_size;
}

void ProcessCmd(DWORD dwType, void* pCmd, void* pArg)
{
	if( dwType != g_dwCurSub )
	{
		g_vecSub[g_dwCurSub].pfnFlush();
		g_dwCurSub = dwType;
	}

	g_vecSub[dwType].pfnBatch(pCmd, pArg);
}

void ShutdownKernel()
{
	for(DWORD i = 0; i < g_vecSub.size(); i++)
	{
		g_vecSub[i].pfnShutdown();
	}

	g_vecSub.clear();
	g_dwCurSub = 0;
}

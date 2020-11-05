/* -------------------------------------------------------------------------
//	文件名		：	kernel_center.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_center的定义
//
// -----------------------------------------------------------------------*/
#ifndef __KERNEL_CENTER_H__
#define __KERNEL_CENTER_H__
#include "../represent_common/common.h"

typedef BOOL (*KernelInit)(void* arg);
typedef void (*KernelBatch)(void* cmd, void* arg);
typedef void (*KernelFlush)();
typedef void (*KernelShutdown)();

struct KernelSub
{
	KernelBatch		pfnBatch;
	KernelFlush		pfnFlush;	
	KernelShutdown	pfnShutdown;
};

extern vector<KernelSub>		g_vecSub;
extern DWORD 					g_dwCurSub;

//kernel types
extern DWORD g_dwKernelTypePic;
extern DWORD g_dwKernelTypeText;
extern DWORD g_dwKernelTypeRect;
extern DWORD g_dwKernelTypeLine;

inline BOOL DummyKernelInit(void* pArg) {return TRUE;}
inline void DummyKernelBatch(void* pCmd, void* pArg) {}
inline void DummyKernelFlush() {}
inline void DummyKernelShutdown() {}

DWORD	InstallSubKernel(KernelInit pfnIinit, void* pInitArg, KernelBatch pfnBatch, KernelFlush pfnFlush, KernelShutdown pfnShutdown);

inline void InitKernel()
{
	g_dwCurSub = InstallSubKernel(DummyKernelInit, 0, DummyKernelBatch, DummyKernelFlush, DummyKernelShutdown);
}

void	ProcessCmd(DWORD type, void* cmd, void* arg);

void	ShutdownKernel();

#endif
/* -------------------------------------------------------------------------
//	文件名		：	kernel_line.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_line的定义,文本行的定义
//
// -----------------------------------------------------------------------*/
#ifndef __KERNEL_LINE_H__
#define __KERNEL_LINE_H__
#include "../represent_common/common.h"

struct KLineCmd
{
	D3DXVECTOR3	sPos1;
	D3DXVECTOR3	sPos2;
	DWORD		dwColor1;
	DWORD		dwColor2;
};

struct KLineDrawable
{
	D3DXVECTOR2	sPos[2];
	DWORD	dwColor[2];
};

//buf
extern vector<KLineDrawable>		g_vecLineDrawable;

//gpu stuff
extern DWORD						g_dwLineSrs;

BOOL InitLineKernel(void* pArg);

void ShutdownLineKernel();

struct KLineCmd;
struct KLineDrawable;
void SnapshotLine(KLineCmd* pIn, KLineDrawable* pOut);

void BatchLineCmd(void* pCmd, void* pArg);

//flush
void FlushLineBuf();

#endif
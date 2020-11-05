/* -------------------------------------------------------------------------
//	�ļ���		��	kernel_line.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-23 11:11:11
//	��������	��	kernel_line�Ķ���,�ı��еĶ���
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
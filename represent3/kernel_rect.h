/* -------------------------------------------------------------------------
//	�ļ���		��	kernel_rect.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-23 11:11:11
//	��������	��	kernel_rect�Ķ���,���εĶ���
//
// -----------------------------------------------------------------------*/
#ifndef __KERNEL_RECT_H__
#define __KERNEL_RECT_H__
#include "../represent_common/common.h"
//kernel - rect
struct KRectCmd
{
	D3DXVECTOR3	sPos1;
	D3DXVECTOR3	sPos2;
	DWORD		dwColor;
};

struct KRectFrameCmd
{
	D3DXVECTOR3	sPos1;
	D3DXVECTOR3	sPos2;
	DWORD		dwColor;
	DWORD		dwUnUsed;
};

struct KRectDrawable
{
	D3DXVECTOR2	aPos[2];
	DWORD		dwColor;
};

//buf
extern vector<KRectDrawable>			g_vecRectDrawable;

//gpu stuff
extern LPD3DIB							g_pRectMultiquadIndexBuffer;
extern DWORD							g_dwRectSrs;

struct KRectKernelInitParam
{
	LPD3DIB	pMultiquadIb;
};

BOOL InitRectKernel(void* pArg);

void ShutdownRectKernel();

void SnapshotRect(KRectCmd* pIn, KRectDrawable* pOut);

//batch
void BatchRectCmd(void* pCmd, void* pArg);

//flush
void FlushRectBuf();
#endif
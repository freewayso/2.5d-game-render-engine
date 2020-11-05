/* -------------------------------------------------------------------------
//	文件名		：	gpu_helper.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-19 11:11:11
//	功能描述	：	渲染器工具函数 
//
// -----------------------------------------------------------------------*/
#ifndef __GPU_HELPER_H__
#define __GPU_HELPER_H__
#include "../represent_common/common.h"
#include "igpu.h"

//gpu helper functions
float FormatBytes( D3DFORMAT eFmt );

D3DFORMAT FindFirstAvaiableOffscrTextureFormat( IKGpu* pGpu, const D3DFORMAT* pFmts, DWORD dwCount );

D3DFORMAT FindFirstAvaiableRenderTargetTextureFormat( IKGpu* pGpu, const D3DFORMAT* pFmts, DWORD dwCount );

void AdjustTextureSize(IKGpu* pGpu, BOOL bEnableConditionalNonPow2, BOOL dxtn, DWORD mipmap_level, const KPOINT* pIn, KPOINT* pOut);

//choose best gpu pp
extern BOOL					g_bDisableFullscreenIme;
extern BOOL					g_bInputWindowed;
extern DWORD				g_dwInputWidth;
extern DWORD				g_dwInputHeight;
extern vector<D3DFORMAT>	g_vecDisplayFmtPriority; 
extern vector<D3DFORMAT>	g_vecBkbufFmtPriority;

INT GpuPPSortCmpFunc( const void* pArg1, const void* pArg2 );

BOOL ChooseBestGpuPP(IKGpu* pGpu, 
						BOOL bWindowed, 
						DWORD dwWidth, 
						DWORD dwHeight, 
						BOOL bPrefer32BitsColor, 
						BOOL bDisableFullscreenIme, 
						KGpuPresentationParameters* pGPP);

#endif
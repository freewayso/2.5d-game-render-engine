/* -------------------------------------------------------------------------
//	文件名		：	_gpu_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	D3D渲染器类的定义及其相关的全局函数
//
// -----------------------------------------------------------------------*/
#ifndef ___GPU_T_H__
#define ___GPU_T_H__
#include "../represent_common/common.h"
#include "igpu.h"

struct KTransitionInfo
{
	vector< pair<DWORD, DWORD> >	vecrRenderState;
	vector< pair<DWORD, DWORD> >	vecTextureStageState;
	vector< pair<DWORD, DWORD> >	vecSamplerState;
};

struct KDynamicRenderStates
{
	LPD3DVB					vbVertexBuffer;
	DWORD					dwStride;
	LPD3DIB					ibIndexBuffer;
	DWORD					dwfvf;
	LPD3DTEXTURE			pTexture;
};

class KGpu : public IKGpu
{
public:
	//d3d
	LPD3D									m_pD3d;

	//hardware caps
	KGpuCaps								m_sCaps;

	//d3d device
	LPD3DDEVICE								m_pDevice;

	D3DPRESENT_PARAMETERS					m_d3dPresentParameters;				//zjq add 09.3.9	把参数保存下来，便于给IGW使用
	//pipeline initialization parameters
	KGpuCreationParameters					m_sCurCP;
	KGpuPresentationParameters				m_sCurPP;

	//dynamic vertex buffer
	LPD3DVB									m_pvbDynVb;
	DWORD									m_dwDynVbSize;
	DWORD									m_dwDynVbUsedBytes;

	//static render states
	vector< KStaticRenderStates >			m_vecSrs;
	vector< vector<KTransitionInfo> >		m_vecTransitionTable;
	DWORD									m_dwCurSrs;						//0 for the default one

	//dynamic render states
	KDynamicRenderStates					m_sCurDrs;

	//current display mode
	VOID	GetCurrentDisplayMode(D3DDISPLAYMODE* pMode) {m_pD3d->GetAdapterDisplayMode(0, pMode);}

	//static caps
	const KGpuCaps*	GetHardwareCaps() {return &m_sCaps;}
	DWORD BuildValidPresentationParameterCombos(LPD3DBUFFER* ppOut);

	//pipeline control
	BOOL	TurnOn(const KGpuCreationParameters* pCP, const KGpuPresentationParameters* pPP, KE_GPU_ERROR* pError);
	VOID	TurnOff();
	BOOL	Reset(const KGpuPresentationParameters* pPP, KE_GPU_ERROR* pError);
	BOOL	BeginScene() {return SUCCEEDED(m_pDevice->BeginScene());}
	BOOL	EndScene() {return SUCCEEDED(m_pDevice->EndScene());}
	BOOL	Present(KE_GPU_ERROR* pError);
	BOOL	TestCooperativeLevel(KE_GPU_ERROR* pError);
	BOOL	ValidateDevice(DWORD* pPasses) {return SUCCEEDED(m_pDevice->ValidateDevice(pPasses));}

	//creation parameters
	const KGpuCreationParameters*		GetCreationParameters() {return &m_sCurCP;}
	const KGpuPresentationParameters*	GetPresentationParameters() {return &m_sCurPP;}

	//dynamic caps
	DWORD	BuildValidRenderTargetTextureFormats(LPD3DBUFFER* ppOut);
	DWORD	BuildValidOffscrTextureFormats(LPD3DBUFFER* ppOut);

	//static objects
	BOOL	CreateVertexBuffer(DWORD dwLength, DWORD dwUsage, DWORD dwFvf, D3DPOOL ePool, LPD3DVB* ppvbOut) {return SUCCEEDED(m_pDevice->CreateVertexBuffer(dwLength, dwUsage, dwFvf, ePool, ppvbOut, 0));}
	BOOL	CreateIndexBuffer(DWORD dwLength, DWORD dwUsage, D3DFORMAT eFormat, D3DPOOL ePool, LPD3DIB* ppibOut) {return SUCCEEDED(m_pDevice->CreateIndexBuffer(dwLength, dwUsage, eFormat, ePool, ppibOut, 0));}
	BOOL	CreateOffscrSurface(DWORD dwWidth, DWORD dwHeight, D3DFORMAT eFormat, D3DPOOL ePool, LPD3DSURFACE* ppOut) {return SUCCEEDED(m_pDevice->CreateOffscreenPlainSurface(dwWidth, dwHeight, eFormat, ePool, ppOut, 0));}
	BOOL	CreateTexture(DWORD dwWidth, DWORD dwHeight, DWORD dwLevels, DWORD dwUsage, D3DFORMAT eFormat, D3DPOOL ePool, LPD3DTEXTURE* ppOutTexture) {return SUCCEEDED(m_pDevice->CreateTexture(dwWidth, dwHeight, dwLevels, dwUsage, eFormat, ePool, ppOutTexture, 0));}

	//dynamic vertex buffer
	VOID	SetDynamicVertexBufferSize(DWORD dwSize);
	VOID*	LockDynamicVertexBuffer(DWORD dwVertexCount, DWORD dwStride, DWORD* pStartVertex, LPD3DVB* pVertexBuffer);
	VOID	UnlockDynamicVertexBuffer();

	//global render states
	VOID	SetRenderTarget(LPD3DSURFACE rt)	{m_pDevice->SetRenderTarget(0, rt);}
	VOID	GetRenderTarget(LPD3DSURFACE* rt) {m_pDevice->GetRenderTarget(0, rt);}
	VOID	SetScissorRect(const RECT* rect) { rect ? (m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true), m_pDevice->SetScissorRect(rect)) : m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);}

	//static render states
	DWORD	DefineStaticRenderStates(const KStaticRenderStates* pIn);
	VOID	SetStaticRenderStates(DWORD dwId);

	//dynamic render states
	VOID	SetStreamSource(LPD3DVB vbVertexBuffer, DWORD dwStride);
	VOID	SetIndices(LPD3DIB ibIndexBuffer);
	VOID	SetFvf(DWORD dwFvf);
	VOID	SetTexture(LPD3DTEXTURE pTexture);

	//draw
	VOID	DrawPrimitive(D3DPRIMITIVETYPE ePrimitiveType, DWORD dwStartVertex, DWORD dwPrimitiveCount) {m_pDevice->DrawPrimitive(ePrimitiveType, dwStartVertex, dwPrimitiveCount);}
	VOID	DrawIndexedPrimitive(D3DPRIMITIVETYPE ePrimitiveType, INT nBaseVertexIndex, DWORD dwMinVertexIndex, DWORD dwNumVertices, DWORD dwStartIndex, DWORD dwPrimitiveCount) {m_pDevice->DrawIndexedPrimitive(ePrimitiveType, nBaseVertexIndex, dwMinVertexIndex, dwNumVertices, dwStartIndex, dwPrimitiveCount);}

	//surface operations
	BOOL	ClearTarget(DWORD dwCount, const D3DRECT* pRects, DWORD dwFlags, D3DCOLOR clrColor, FLOAT fZ, DWORD dwStencil) {return SUCCEEDED(m_pDevice->Clear(0, 0, dwFlags, clrColor, fZ, dwStencil));}
	BOOL	UpdateTexture(LPD3DTEXTURE pSrcTexture, LPD3DTEXTURE pDstTexture) {return SUCCEEDED(m_pDevice->UpdateTexture(pSrcTexture, pDstTexture));}
	BOOL	ColorFill(LPD3DSURFACE pSurface, const RECT* pRect, D3DCOLOR clrColor) {return SUCCEEDED(m_pDevice->ColorFill(pSurface, pRect, clrColor));}
	BOOL	GetRenderTargetData(LPD3DSURFACE pRt, LPD3DSURFACE pDst) {return SUCCEEDED(m_pDevice->GetRenderTargetData(pRt, pDst));}
	BOOL	GetFrontBufferData(LPD3DSURFACE pDst) {return SUCCEEDED(m_pDevice->GetFrontBufferData(0, pDst));}
};

extern KGpu g_Gpu;

inline IKGpu* Gpu() {return &g_Gpu;}
BOOL InitGpu(KE_GPU_ERROR* pError);
VOID ShutdownGpu();


VOID TranslateCaps(KGpuCaps* pGpuCaps, const D3DCAPS9* pD3dCaps);

VOID TranslateInitializationParameters(D3DDEVICE_CREATION_PARAMETERS* pD3dCP, 
										 D3DPRESENT_PARAMETERS* pD3dPP, 
										 const KGpuCreationParameters* pCP, 
										 const KGpuPresentationParameters* pPP);

VOID InitTransitionInfo(KTransitionInfo* pInfo, const KStaticRenderStates* pSrc, const KStaticRenderStates* pDst);

VOID ApplyTransitionInfo(LPD3DDEVICE pDevice, KTransitionInfo* pInfo);

VOID ForceDefaultDynamicRenderStates(LPD3DDEVICE pDevice, KDynamicRenderStates* pDrs);

#endif

/* -------------------------------------------------------------------------
//	文件名		：	gpu.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	渲染器基类的定义及其相关的全局函数
//
// -----------------------------------------------------------------------*/
#ifndef __GPU_T_H__
#define __GPU_T_H__
#include "../represent_common/common.h"

//vertex buffer data structures
struct KVertex1
{
	D3DXVECTOR4	sPos;
	DWORD		dwColor;
	D3DXVECTOR2	sUV;
};

struct KVertex2
{
	D3DXVECTOR4	sPos;
	DWORD	dwColor1;
	DWORD	dwColor2;
	D3DXVECTOR2	sUV;
};

struct KVertex0
{
	D3DXVECTOR4	sPos;
	DWORD	dwColor;
};

enum KE_GPU_ERROR
{
	emGPU_ERROR_UNKNOWN,
	emGPU_ERROR_INIT_D3D_FAILED,
	emGPU_ERROR_HARDWARE_ACCELERATION_NOT_AVAILABLE,
	emGPU_ERROR_OUT_OF_VIDEO_MEMORY,
	emGPU_ERROR_INVALID_PARAMS,
	emGPU_ERROR_DEVICE_LOST,
	emGPU_ERROR_DEVICE_NOT_RESET,
	emGPU_ERROR_DRIVER_INTERNAL_ERROR,
};

// 有些显卡只能用2的n次方大小的贴图， 有的显卡没限制， 有的是有条件的限制
enum KE_GPU_TEXTURE_POW2_RESTRICTION
{
	emGPU_TEXTURE_POW2_RESTRICTION_NONE,
	emGPU_TEXTURE_POW2_RESTRICTION_CONDITIONAL,
	emGPU_TEXTURE_POW2_RESTRICTION_ALL,
};

enum KE_GPU_VERTEX_PROCESSING_METHOD
{
	emGPU_VERTEX_PROCESSING_METHOD_SOFEWARE,
	emKE_GPU_VERTEX_PROCESSING_METHOD_HARDWARE,
	emKE_GPU_VERTEX_PROCESSING_METHOD_PUREHARDWARE,
};

struct KGpuCaps
{
	BOOL							bCanClipTnlVerts;
	DWORD							dwMaxTextureAspectRatio;
	DWORD							dwMaxTextureWidth;
	DWORD							dwMaxTextureHeight;
	KE_GPU_TEXTURE_POW2_RESTRICTION	eTexturePow2Restriction;
	BOOL							bScissorTestAvaiable;
	KE_GPU_VERTEX_PROCESSING_METHOD	eMaxVertexProcessingLevel;
};

struct KGpuCreationParameters
{
	HWND							hWnd;
	BOOL							bMultithread;
	KE_GPU_VERTEX_PROCESSING_METHOD	eVertexProcessingMethod;
};

struct KGpuPresentationParameters
{
	BOOL					bWindowed;
	BOOL					bGdiDialogboxSupport;
	D3DDISPLAYMODE			sDisplayMode;
	D3DFORMAT				eBackbufferFormat;
	DWORD					dwBackbufferWidth;
	DWORD					dwBackbufferHeight;
	DWORD					dwBackbufferCount;
	BOOL					bVerticalSync;
};

struct KStaticRenderStates
{
	DWORD	dwLighting;
	DWORD	dwCullMode;
	DWORD	dwAlphaTestEnable;
	DWORD	dwAlphaRef;
	DWORD	dwAlphaFunc;
	DWORD	dwZEnable;
	DWORD	dwZFunc;
	DWORD	dwZWriteEnable;
	DWORD	dwAlphaBlendEnable;
	DWORD	dwBlendOp;
	DWORD	dwSrcBlend;
	DWORD	dwDestBlend;
	DWORD	dwColorOp;
	DWORD	dwColorArg1;
	DWORD	dwColorArg2;
	DWORD	dwAlphaOp;
	DWORD	dwAlphaArg1;
	DWORD	dwAlphaArg2;
	DWORD	dwAddressU;
	DWORD	dwAddressV;
	DWORD	dwMagFilter;
	DWORD	dwMinFilter;
};

class IKGpu
{
public:
	//current display mode (variable)
	virtual VOID	GetCurrentDisplayMode(D3DDISPLAYMODE* pMode) = 0;

	//static caps
	virtual const	KGpuCaps* GetHardwareCaps() = 0;
	virtual DWORD	BuildValidPresentationParameterCombos(LPD3DBUFFER* ppOut) = 0;

	//pipeline control 
	virtual BOOL	TurnOn(const KGpuCreationParameters* pCp, const KGpuPresentationParameters* pPp, KE_GPU_ERROR* pError) = 0;	//all the methods below are valid only after it is successfully turned on
	virtual VOID	TurnOff() = 0;	//ensure that all gpu objects are destroyed before you turn it off
	virtual BOOL	Reset(const KGpuPresentationParameters* pp, KE_GPU_ERROR* error) = 0;	//ensure that all video objects are descarded before resetting it
	virtual BOOL	BeginScene() = 0;
	virtual BOOL	EndScene() = 0;
	virtual BOOL	Present(KE_GPU_ERROR* pError) = 0;
	virtual BOOL	TestCooperativeLevel(KE_GPU_ERROR* pError) = 0;
	virtual BOOL	ValidateDevice(DWORD* pPasses) = 0;

	//creation parameters
	virtual const KGpuCreationParameters*		GetCreationParameters() = 0;
	virtual const KGpuPresentationParameters*	GetPresentationParameters() = 0;

	//dynamic caps
	virtual DWORD	BuildValidRenderTargetTextureFormats(LPD3DBUFFER* ppOut) = 0;
	virtual DWORD	BuildValidOffscrTextureFormats(LPD3DBUFFER* ppOut) = 0;

	//objects
	virtual BOOL	CreateVertexBuffer(DWORD dwLength, DWORD dwUsage, DWORD dwFvf, D3DPOOL ePool, LPD3DVB* ppOut) = 0;
	virtual BOOL	CreateIndexBuffer(DWORD dwLength, DWORD dwUsage, D3DFORMAT eFormat, D3DPOOL ePool, LPD3DIB* ppOut) = 0;
	virtual BOOL	CreateOffscrSurface(DWORD dwWidth, DWORD dwHeight, D3DFORMAT eFormat, D3DPOOL ePool, LPD3DSURFACE* ppOut) = 0;
	virtual BOOL	CreateTexture(DWORD dwWidth, DWORD dwHeight, DWORD dwLevels, DWORD dwUsage, D3DFORMAT eFormat, D3DPOOL ePool, LPD3DTEXTURE* ppOut) = 0;

	//dynamic vertex buffer
	virtual VOID	SetDynamicVertexBufferSize(DWORD dwSize) = 0;	//don't do this between lock/unlock !
	virtual VOID*	LockDynamicVertexBuffer(DWORD dwVertexCount, DWORD dwStride, DWORD* pStartVertex, LPD3DVB* pVertexBuffer) = 0;
	virtual VOID	UnlockDynamicVertexBuffer() = 0;

	//global render states
	virtual VOID	SetRenderTarget(LPD3DSURFACE pRenderTarget) = 0;
	virtual VOID	GetRenderTarget(LPD3DSURFACE* ppRenderTarget) = 0;
	virtual VOID	SetScissorRect(const RECT* pRect) = 0;

	//static render states
	virtual DWORD	DefineStaticRenderStates(const KStaticRenderStates* pIn) = 0;
	virtual VOID	SetStaticRenderStates(DWORD dwId) = 0;

	//dynamic render states
	virtual VOID	SetStreamSource(LPD3DVB pvbVertexBuffer, DWORD dwStride) = 0;
	virtual VOID	SetIndices(LPD3DIB pibIndexBuffer) = 0;
	virtual VOID	SetFvf(DWORD dwFvf) = 0;
	virtual VOID	SetTexture(LPD3DTEXTURE pTexture) = 0;

	//draw
	virtual VOID	DrawPrimitive(D3DPRIMITIVETYPE ePrimitiveType, DWORD dwStartVertex, DWORD dwPrimitiveCount) = 0;
	virtual VOID	DrawIndexedPrimitive(D3DPRIMITIVETYPE ePrimitiveType, INT nBaseVertexIndex, DWORD dwMinVertexIndex, DWORD dwNumVertices, DWORD dwStartIndex, DWORD dwPrimitiveCount) = 0;

	//surface operations
	virtual BOOL	ClearTarget(DWORD dwCount, const D3DRECT* pRects, DWORD dwFlags, D3DCOLOR clrColor, float fZ, DWORD dwStencil) = 0;
	virtual BOOL	UpdateTexture(LPD3DTEXTURE pSrcTexture, LPD3DTEXTURE pDstTexture) = 0;
	virtual BOOL	ColorFill(LPD3DSURFACE pSurFace, const RECT* pRect, D3DCOLOR clrColor) = 0;
	virtual BOOL	GetRenderTargetData(LPD3DSURFACE pRt, LPD3DSURFACE pDst) = 0;
	virtual BOOL	GetFrontBufferData(LPD3DSURFACE pDst) = 0;
};

BOOL	InitGpu(KE_GPU_ERROR* pError);	//if failed, the error may be emGPU_ERROR_INIT_D3D_FAILED or _gpu_error_hardware_acceleration_not_available_
VOID	ShutdownGpu();		//ensure that the gpu is not turned on when shutting down
IKGpu*	Gpu();	//use this ptr only after init_gpu is successfully called

#endif
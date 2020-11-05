/* -------------------------------------------------------------------------
//	文件名		：	kernel_rect.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_rect的定义,矩形的定义
//
// -----------------------------------------------------------------------*/
#include "kernel_rect.h"
#include "igpu.h"

//buf
vector<KRectDrawable>			g_vecRectDrawable;

//gpu stuff
LPD3DIB							g_pRectMultiquadIndexBuffer = 0;
DWORD							g_dwRectSrs = -1;

BOOL InitRectKernel(void* pArg)
{
	KRectKernelInitParam* pParam = (KRectKernelInitParam*)pArg;
	if(!pParam->pMultiquadIb)
		return FALSE;
	g_pRectMultiquadIndexBuffer = pParam->pMultiquadIb;
	g_pRectMultiquadIndexBuffer->AddRef();

	//static states
	KStaticRenderStates sRectSrsDef;

	sRectSrsDef.dwLighting				= FALSE;
	sRectSrsDef.dwCullMode				= D3DCULL_NONE;
	sRectSrsDef.dwAlphaTestEnable		= FALSE;
	sRectSrsDef.dwAlphaRef				= 0;
	sRectSrsDef.dwAlphaFunc				= D3DCMP_GREATER;
	sRectSrsDef.dwZEnable				= D3DZB_FALSE;
	sRectSrsDef.dwZFunc					= D3DCMP_LESSEQUAL;
	sRectSrsDef.dwZWriteEnable			= TRUE;
	sRectSrsDef.dwAlphaBlendEnable		= TRUE;
	sRectSrsDef.dwBlendOp				= D3DBLENDOP_ADD;
	sRectSrsDef.dwSrcBlend				= D3DBLEND_SRCALPHA;
	sRectSrsDef.dwDestBlend				= D3DBLEND_INVSRCALPHA;
	sRectSrsDef.dwColorOp				= D3DTOP_SELECTARG2;
	sRectSrsDef.dwColorArg1				= D3DTA_TEXTURE;
	sRectSrsDef.dwColorArg2				= D3DTA_DIFFUSE;
	sRectSrsDef.dwAlphaOp				= D3DTOP_SELECTARG2;
	sRectSrsDef.dwAlphaArg1				= D3DTA_TEXTURE;
	sRectSrsDef.dwAlphaArg2				= D3DTA_DIFFUSE;
	sRectSrsDef.dwAddressU				= D3DTADDRESS_CLAMP;
	sRectSrsDef.dwAddressV				= D3DTADDRESS_CLAMP;
	sRectSrsDef.dwMagFilter				= D3DTEXF_POINT;
	sRectSrsDef.dwMinFilter				= D3DTEXF_POINT;

	g_dwRectSrs = Gpu()->DefineStaticRenderStates(&sRectSrsDef);

	return TRUE;
}

void ShutdownRectKernel()
{
	g_vecRectDrawable.clear();
	SAFE_RELEASE(g_pRectMultiquadIndexBuffer);
	g_dwRectSrs = -1;
}

void SnapshotRect(KRectCmd* in, KRectDrawable* out)
{
	out->aPos[0].x = in->sPos1.x;
	out->aPos[0].y = in->sPos1.y;

	out->aPos[1].x = in->sPos2.x;
	out->aPos[1].y = in->sPos2.y;

	//patch alpha
	out->dwColor = ((255 - ((in->dwColor >> 24) << 3)) << 24) | (in->dwColor & 0x00ffffff);
}

//batch
void BatchRectCmd(void* cmd, void* arg)
{
	KRectDrawable elem;
	SnapshotRect((KRectCmd*)cmd, &elem);
	g_vecRectDrawable.push_back(elem);
}

//flush
void FlushRectBuf()
{
	if( !g_vecRectDrawable.empty() )
	{
		DWORD dwRectCount = (DWORD)g_vecRectDrawable.size();
		DWORD dwBaseVertexIndex;
		LPD3DVB	pVertexBuffer;
		KVertex0* v = (KVertex0*)Gpu()->LockDynamicVertexBuffer(dwRectCount * 4, sizeof(KVertex0), &dwBaseVertexIndex, &pVertexBuffer);
		if(v)
		{
			//lock into vertex buffer
			for( DWORD dwIndex = 0, i = 0; i < dwRectCount; i++ )
			{
				KRectDrawable& rect = g_vecRectDrawable[i];
				//top-left
				v[dwIndex].sPos = D3DXVECTOR4(rect.aPos[0].x, rect.aPos[0].y, 0, 1);
				v[dwIndex++].dwColor = rect.dwColor;

				//top-right
				v[dwIndex].sPos = D3DXVECTOR4(rect.aPos[1].x, rect.aPos[0].y, 0, 1);
				v[dwIndex++].dwColor = rect.dwColor;

				//bottom-left
				v[dwIndex].sPos = D3DXVECTOR4(rect.aPos[0].x, rect.aPos[1].y, 0, 1);
				v[dwIndex++].dwColor = rect.dwColor;

				//bottom-right
				v[dwIndex].sPos = D3DXVECTOR4(rect.aPos[1].x, rect.aPos[1].y, 0, 1);
				v[dwIndex++].dwColor = rect.dwColor;
			}
			Gpu()->UnlockDynamicVertexBuffer();
			//dynamic states
			//stream source
			Gpu()->SetStreamSource(pVertexBuffer, sizeof(KVertex0));
			//indices
			Gpu()->SetIndices(g_pRectMultiquadIndexBuffer);
			//fvf/decl
			Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
			//texture (unused)

			//static states	
			Gpu()->SetStaticRenderStates(g_dwRectSrs);
			//draw
			Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, dwBaseVertexIndex, 0, dwRectCount * 4, 0, dwRectCount * 2);
		}

		//clear buf
		g_vecRectDrawable.clear();
	}
}


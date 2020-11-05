/* -------------------------------------------------------------------------
//	文件名		：	kernel_line.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_line的定义,文本行的定义
//
// -----------------------------------------------------------------------*/
#include "kernel_line.h"
#include "igpu.h"
#include "kernel_text.h"

//buf
vector<KLineDrawable>		g_vecLineDrawable;

//gpu stuff
DWORD						g_dwLineSrs = -1;

BOOL InitLineKernel(void* pArg)
{
	KStaticRenderStates sLineSrsDef;

	sLineSrsDef.dwLighting				= FALSE;
	sLineSrsDef.dwCullMode				= D3DCULL_NONE;
	sLineSrsDef.dwAlphaTestEnable		= FALSE;
	sLineSrsDef.dwAlphaRef				= 0;
	sLineSrsDef.dwAlphaFunc				= D3DCMP_GREATER;
	sLineSrsDef.dwZEnable				= D3DZB_FALSE;
	sLineSrsDef.dwZFunc					= D3DCMP_LESSEQUAL;
	sLineSrsDef.dwZWriteEnable			= TRUE;
	sLineSrsDef.dwAlphaBlendEnable		= TRUE;
	sLineSrsDef.dwBlendOp				= D3DBLENDOP_ADD;
	sLineSrsDef.dwSrcBlend				= D3DBLEND_SRCALPHA;
	sLineSrsDef.dwDestBlend				= D3DBLEND_INVSRCALPHA;
	sLineSrsDef.dwColorOp				= D3DTOP_SELECTARG2;
	sLineSrsDef.dwColorArg1				= D3DTA_TEXTURE;
	sLineSrsDef.dwColorArg2				= D3DTA_DIFFUSE;
	sLineSrsDef.dwAlphaOp				= D3DTOP_SELECTARG2;
	sLineSrsDef.dwAlphaArg1				= D3DTA_TEXTURE;
	sLineSrsDef.dwAlphaArg2				= D3DTA_DIFFUSE;
	sLineSrsDef.dwAddressU				= D3DTADDRESS_CLAMP;
	sLineSrsDef.dwAddressV				= D3DTADDRESS_CLAMP;
	sLineSrsDef.dwMagFilter				= D3DTEXF_POINT;
	sLineSrsDef.dwMinFilter				= D3DTEXF_POINT;

	g_dwLineSrs = Gpu()->DefineStaticRenderStates(&sLineSrsDef);

	return TRUE;
}

void ShutdownLineKernel() {g_dwLineSrs = -1;}

void SnapshotLine(KLineCmd* in, KLineDrawable* out)
{
	out->sPos[0].x = in->sPos1.x;
	out->sPos[0].y = in->sPos1.y;

	out->sPos[1].x = in->sPos2.x;
	out->sPos[1].y = in->sPos2.y;

	out->dwColor[0] = in->dwColor1;
	out->dwColor[1] = in->dwColor2;
}

void BatchLineCmd(void* cmd, void* arg)
{
	KLineDrawable elem;
	SnapshotLine((KLineCmd*)cmd, &elem);
	g_vecLineDrawable.push_back(elem);
}

//flush
void FlushLineBuf()
{
	if( !g_vecLineDrawable.empty() )
	{
		DWORD	dwLineCount = (DWORD)g_vecLineDrawable.size();
		DWORD	dwStartVertex;
		LPD3DVB	pVertexBuffer;
		KVertex0* pVertex = (KVertex0*)Gpu()->LockDynamicVertexBuffer(dwLineCount * 2, sizeof(KVertex0), &dwStartVertex, &pVertexBuffer);
		if( pVertex )
		{
			//lock into vertex buffer
			for( DWORD dwIndex = 0, i = 0; i < dwLineCount; i++ )
			{
				KLineDrawable& line = g_vecLineDrawable[i];
				pVertex[dwIndex].sPos = D3DXVECTOR4(line.sPos[0].x, line.sPos[0].y, 0, 1);
				pVertex[dwIndex++].dwColor = line.dwColor[0];

				pVertex[dwIndex].sPos = D3DXVECTOR4(line.sPos[1].x, line.sPos[1].y, 0, 1);
				pVertex[dwIndex++].dwColor = line.dwColor[1];
			}
			Gpu()->UnlockDynamicVertexBuffer();
			//dynamic states
			//stream source
			Gpu()->SetStreamSource(pVertexBuffer, sizeof(KVertex0));
			//indices (unused)

			//fvf/decl
			Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
			//texture (unused)

			//static states
			Gpu()->SetStaticRenderStates(g_dwLineSrs);
			//draw
			Gpu()->DrawPrimitive(D3DPT_LINELIST, dwStartVertex, dwLineCount);
		}

		//clear buf
		g_vecLineDrawable.clear();
	}
}
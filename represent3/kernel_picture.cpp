/* -------------------------------------------------------------------------
//	文件名		：	kernel_picture.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_picture的定义,图片的定义
//
// -----------------------------------------------------------------------*/
#include "kernel_picture.h"
#include "igpu.h"
#include "media_center.h"
#include "sprite.h"

//kernel - pic
D3DXVECTOR2	g_sPicPostScaling = D3DXVECTOR2(1, 1);
BOOL	g_bForceDrawPic = FALSE;

//buf
vector<KPicDrawable>		g_vecPicDrawable;

//gpu objects
LPD3DIB								g_pPicMultiquadIndexBuffer = 0;
DWORD								g_dwPicSrsAlpha;
DWORD								g_dwPicSrsCopy;
DWORD								g_dwPicSrsScreen;


BOOL InitPicKernel(void* pArg)
{
	KPicKernelInitParam* pParam = (KPicKernelInitParam*)pArg;

	//multiquad
	if( !pParam->pMultiquadIb )
		return FALSE;
	g_pPicMultiquadIndexBuffer = pParam->pMultiquadIb;
	g_pPicMultiquadIndexBuffer->AddRef();

	//alpha
	KStaticRenderStates sPicSrsDefAlpha;

	sPicSrsDefAlpha.dwLighting				= FALSE;
	sPicSrsDefAlpha.dwCullMode				= D3DCULL_NONE;
	sPicSrsDefAlpha.dwAlphaTestEnable		= TRUE;
	sPicSrsDefAlpha.dwAlphaRef				= 0;
	sPicSrsDefAlpha.dwAlphaFunc			= D3DCMP_GREATER;
	sPicSrsDefAlpha.dwZEnable				= D3DZB_FALSE;
	sPicSrsDefAlpha.dwZFunc				= D3DCMP_LESSEQUAL;
	sPicSrsDefAlpha.dwZWriteEnable		= TRUE;
	sPicSrsDefAlpha.dwAlphaBlendEnable	= TRUE;
	sPicSrsDefAlpha.dwBlendOp				= D3DBLENDOP_ADD;
	sPicSrsDefAlpha.dwSrcBlend				= D3DBLEND_SRCALPHA;
	sPicSrsDefAlpha.dwDestBlend			= D3DBLEND_INVSRCALPHA;
	sPicSrsDefAlpha.dwColorOp				= D3DTOP_MODULATE;
	sPicSrsDefAlpha.dwColorArg1			= D3DTA_TEXTURE;
	sPicSrsDefAlpha.dwColorArg2			= D3DTA_DIFFUSE;
	sPicSrsDefAlpha.dwAlphaOp				= D3DTOP_MODULATE;
	sPicSrsDefAlpha.dwAlphaArg1			= D3DTA_TEXTURE;
	sPicSrsDefAlpha.dwAlphaArg2			= D3DTA_DIFFUSE;
	sPicSrsDefAlpha.dwAddressU				= D3DTADDRESS_CLAMP;
	sPicSrsDefAlpha.dwAddressV				= D3DTADDRESS_CLAMP;
	sPicSrsDefAlpha.dwMagFilter			= pParam->bLinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT;
	sPicSrsDefAlpha.dwMinFilter			= pParam->bLinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT;

	g_dwPicSrsAlpha = Gpu()->DefineStaticRenderStates(&sPicSrsDefAlpha);

	//copy
	KStaticRenderStates sPicSrsDefCopy;

	sPicSrsDefCopy.dwLighting			= FALSE;
	sPicSrsDefCopy.dwCullMode			= D3DCULL_NONE;
	sPicSrsDefCopy.dwAlphaTestEnable	= TRUE;
	sPicSrsDefCopy.dwAlphaRef			= 0;
	sPicSrsDefCopy.dwAlphaFunc			= D3DCMP_GREATER;
	sPicSrsDefCopy.dwZEnable			= D3DZB_FALSE;
	sPicSrsDefCopy.dwZFunc				= D3DCMP_LESSEQUAL;
	sPicSrsDefCopy.dwZWriteEnable		= TRUE;
	sPicSrsDefCopy.dwAlphaBlendEnable	= FALSE;
	sPicSrsDefCopy.dwBlendOp			= D3DBLENDOP_ADD;
	sPicSrsDefCopy.dwSrcBlend			= D3DBLEND_SRCALPHA;
	sPicSrsDefCopy.dwDestBlend			= D3DBLEND_INVSRCALPHA;
	sPicSrsDefCopy.dwColorOp			= D3DTOP_MODULATE;
	sPicSrsDefCopy.dwColorArg1			= D3DTA_TEXTURE;
	sPicSrsDefCopy.dwColorArg2			= D3DTA_DIFFUSE;
	sPicSrsDefCopy.dwAlphaOp			= D3DTOP_MODULATE;
	sPicSrsDefCopy.dwAlphaArg1			= D3DTA_TEXTURE;
	sPicSrsDefCopy.dwAlphaArg2			= D3DTA_DIFFUSE;
	sPicSrsDefCopy.dwAddressU			= D3DTADDRESS_CLAMP;
	sPicSrsDefCopy.dwAddressV			= D3DTADDRESS_CLAMP;
	sPicSrsDefCopy.dwMagFilter			= pParam->bLinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT;
	sPicSrsDefCopy.dwMinFilter			= pParam->bLinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT;

	g_dwPicSrsCopy = Gpu()->DefineStaticRenderStates(&sPicSrsDefCopy);

	//screen blend
	KStaticRenderStates sPicSrsDefScreen;

	sPicSrsDefScreen.dwLighting				= FALSE;
	sPicSrsDefScreen.dwCullMode			= D3DCULL_NONE;
	sPicSrsDefScreen.dwAlphaTestEnable	= TRUE;
	sPicSrsDefScreen.dwAlphaRef			= 0;
	sPicSrsDefScreen.dwAlphaFunc			= D3DCMP_GREATER;
	sPicSrsDefScreen.dwZEnable				= D3DZB_FALSE;
	sPicSrsDefScreen.dwZFunc				= D3DCMP_LESSEQUAL;
	sPicSrsDefScreen.dwZWriteEnable		= TRUE;
	sPicSrsDefScreen.dwAlphaBlendEnable	= TRUE;
	sPicSrsDefScreen.dwBlendOp				= D3DBLENDOP_ADD;
	sPicSrsDefScreen.dwSrcBlend			= D3DBLEND_ONE;
	sPicSrsDefScreen.dwDestBlend			= D3DBLEND_INVSRCCOLOR;
	sPicSrsDefScreen.dwColorOp				= D3DTOP_MODULATE;
	sPicSrsDefScreen.dwColorArg1			= D3DTA_TEXTURE;
	sPicSrsDefScreen.dwColorArg2			= D3DTA_DIFFUSE;
	sPicSrsDefScreen.dwAlphaOp				= D3DTOP_MODULATE;
	sPicSrsDefScreen.dwAlphaArg1			= D3DTA_TEXTURE;
	sPicSrsDefScreen.dwAlphaArg2			= D3DTA_DIFFUSE;
	sPicSrsDefScreen.dwAddressU			= D3DTADDRESS_CLAMP;
	sPicSrsDefScreen.dwAddressV			= D3DTADDRESS_CLAMP;
	sPicSrsDefScreen.dwMagFilter			= pParam->bLinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT;
	sPicSrsDefScreen.dwMinFilter			= pParam->bLinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT;

	g_dwPicSrsScreen = Gpu()->DefineStaticRenderStates(&sPicSrsDefScreen);

	return TRUE;
}

void ShutdownPicKernel()
{
	SAFE_RELEASE( g_pPicMultiquadIndexBuffer );
	g_vecPicDrawable.clear();
}

void CorrectPicHandle(KPicCmd* pIn)
{
	switch( pIn->dwPicType )
	{
	case emPIC_TYPE_SPR:
		{
			if( CheckHandle(pIn->dwHandle1) != g_dwMediaTypeSpr )
			{
				if( !pIn->dwExchangeColor )
				{
					pIn->dwHandle1 = pIn->dwHandle2 = FindOrCreateMedia(g_dwMediaTypeSpr, pIn->szFileName, 0);
				}
				else
				{
					char identifier[300];
					DWORD i = 0;
					while( pIn->szFileName[i] != 0 )
					{
						identifier[i] = pIn->szFileName[i];
						i++;
					}
					identifier[i] = 0;
					DWORD real_handle = FindOrCreateMedia(g_dwMediaTypeSpr, identifier, 0);
					KSprite* real_spr = 0;
					if( !QueryProduct(real_handle, FALSE, (void**)&real_spr) || !real_spr )
					{
						pIn->dwHandle1 = pIn->dwHandle2 = 0;
					}
					else
					{
						if( real_spr->GetExchangeColorCount() == 0 )
						{
							pIn->dwHandle1 = pIn->dwHandle2 = real_handle;
						}
						else
						{
							identifier[i] = '|';
							sprintf(&identifier[i+1], "%d", pIn->dwExchangeColor);
							pIn->dwHandle1 = pIn->dwHandle2 = FindOrCreateMedia(g_dwMediaTypeSpr, identifier, 0);
						}
					}
				}
			}
		}
		break;
	case emPIC_TYPE_JPG_OR_LOCKABLE:
		{
			DWORD type = CheckHandle( pIn->dwHandle1 );
			if ( type != g_dwMediaTypeJpg && type != g_dwMediaTypeLockable )
			{
				DWORD handle;
				if(!(handle = FindMedia(g_dwMediaTypeLockable, pIn->szFileName)))
				{
					handle = FindOrCreateMedia(g_dwMediaTypeJpg, pIn->szFileName, 0);
				}
				pIn->dwHandle1 = pIn->dwHandle2 = handle;
			}
		}
		break;
	case emPIC_TYPE_RT:
		{
			if( CheckHandle( pIn->dwHandle1 ) != g_dwMediaTypeRt )
			{
				pIn->dwHandle1 = pIn->dwHandle2 = FindMedia( g_dwMediaTypeRt, pIn->szFileName );
			}
		}
		break;
	default:
		pIn->dwHandle1 = pIn->dwHandle2 = 0;
		break;
	}
}

BOOL SnapshotPic(KPicCmd* pIn, KPicCmdArg* pArg, KPicDrawable* pOut)
{
	//correct handle
	CorrectPicHandle(pIn);

	KSprite* pSpr = 0;
	KSpriteFrame* pFrame = 0;

	BOOL bForceDrawPicOld = g_bForceDrawPic;
	DWORD dwPicType = CheckHandle(pIn->dwHandle1);
	if( dwPicType == g_dwMediaTypeRt || dwPicType == g_dwMediaTypeLockable )
		g_bForceDrawPic = TRUE;

	//query spr
	if( QueryProduct(pIn->dwHandle1, g_bForceDrawPic, (void**)&pSpr) )
	{
		if( !pSpr )
			return FALSE;

		if( pIn->wFrameIndex >= (WORD)pSpr->GetFrameCount() ) 
			return FALSE;

		if( pSpr->QueryFrame(pIn->dwHandle1, pIn->wFrameIndex, g_bForceDrawPic, (void**)&pFrame) )
		{
			if( !pFrame )
				return FALSE;
		}
	}

	INT nRealRenderStyle = pIn->byRenderStyle;
	if( !pFrame )
	{
		//use stand by list
		KPRIMITIVE_INDEX_LIST* node = pArg->pStandByList;
		while( node )
		{
			if( (CheckHandle(node->sIndex.uImage) == g_dwMediaTypeSpr) && 
				QueryProduct(node->sIndex.uImage, g_bForceDrawPic, (void**)&pSpr) && pSpr &&
				(node->sIndex.nFrame >= 0) && (node->sIndex.nFrame < (INT)pSpr->GetFrameCount()) &&
				pSpr->QueryFrame(node->sIndex.uImage, node->sIndex.nFrame, g_bForceDrawPic, (void**)&pFrame) && pFrame )
			{
				if( node->sIndex.nRenderStyle != 0 )
					nRealRenderStyle = node->sIndex.nRenderStyle;
				break;
			}
			node = node->pNext;
		}
	}

	g_bForceDrawPic = bForceDrawPicOld;

	// 渲染备用资源
	if( !pFrame )
	{	
		DWORD dwHandle;
		if(!(dwHandle = FindMedia(g_dwMediaTypeSpr, pIn->szSubstrituteFileName)))
		{
			// 这里应该一早就被创建好，如果找不到。则是创建有问题
			return FALSE;
		}

		if(QueryProduct(dwHandle, 1, (void**)&pSpr))
		{
			if( !pSpr )
				return FALSE;

			if( pSpr->QueryFrame(dwHandle, 0, 1, (void**)&pFrame) )
			{
				if( !pFrame )
					return FALSE;
			}
		}
	}

	if( !pFrame )
		return FALSE;

	//check texture
	LPD3DTEXTURE texture = pFrame->GetTexture();
	if(!texture)
		return FALSE;

	//texture
	pOut->pTexture = texture;

	//color & blending mode
	switch( nRealRenderStyle )
	{
	case emRENDER_STYLE_ALPHA_MODULATE_ALPHA_BLENDING:
		pOut->dwColor = D3DCOLOR_ARGB(pIn->dwColor>>24, 255, 255, 255);
		pOut->eAlphaBlendingMode = emKE_ALPHA_BLENDING_MODE_ALPHA;
		break;
	case emRENDER_STYLE_ALPHA_COLOR_MODULATE_ALPHA_BLENDING:
		pOut->dwColor = pIn->dwColor;
		pOut->eAlphaBlendingMode = emKE_ALPHA_BLENDING_MODE_ALPHA;
		break;
	case emRENDER_STYLE_ALPHA_BLENDING:
		pOut->dwColor = 0xffffffff;
		pOut->eAlphaBlendingMode = emKE_ALPHA_BLENDING_MODE_ALPHA;
		break;
	case emRENDER_STYLE_COPY:
		pOut->dwColor = 0xffffffff;
		pOut->eAlphaBlendingMode = emKE_ALPHA_BLENDING_MODE_COPY;
		break;
	case emRENDER_STYLE_SCREEN:
		pOut->dwColor = D3DCOLOR_ARGB(pIn->dwColor>>24, pIn->dwColor>>24, pIn->dwColor>>24, pIn->dwColor>>24);
		pOut->eAlphaBlendingMode = emKE_ALPHA_BLENDING_MODE_SCREEN;
		break;
	default:
		pOut->dwColor = 0xffffffff;
		pOut->eAlphaBlendingMode = emKE_ALPHA_BLENDING_MODE_ALPHA;
		break;
	}

	//pos[0]
	switch( pIn->byRefMethod )
	{
	case emREF_METHOD_TOP_LEFT:
		pOut->asPos[0].x = pIn->sPos1.x + pFrame->GetFrameOffset()->x;
		pOut->asPos[0].y = pIn->sPos1.y + pFrame->GetFrameOffset()->y;
		break;
	case emREF_METHOD_CENTER:
		{
			KPOINT center = *pSpr->GetCenter();
			if( center == g_PointZero && pSpr->GetSize()->x > 160 )
			{
				center = KPOINT(160, 208);
			}
			pOut->asPos[0].x = pIn->sPos1.x - center.x + pFrame->GetFrameOffset()->x;
			pOut->asPos[0].y = pIn->sPos1.y - center.y + pFrame->GetFrameOffset()->y;
		}
		break;
	case emREF_METHOD_FRAME_TOP_LEFT:
		pOut->asPos[0].x = pIn->sPos1.x;
		pOut->asPos[0].y = pIn->sPos1.y;
		break;
	default:
		pOut->asPos[0].x = pIn->sPos1.x;
		pOut->asPos[0].y = pIn->sPos1.y;
		break;
	}

	//pos[1] & uv
	if( pArg->bPart )
	{
		KPicCmdPart* in_ex = (KPicCmdPart*)pIn;

		RECT sSrcClipRect;
		sSrcClipRect.left = in_ex->cTopLeft.x - pFrame->GetFrameOffset()->x;
		sSrcClipRect.top = in_ex->cTopLeft.y - pFrame->GetFrameOffset()->y;
		sSrcClipRect.right = in_ex->cBottomRight.x - pFrame->GetFrameOffset()->x;
		sSrcClipRect.bottom = in_ex->cBottomRight.y - pFrame->GetFrameOffset()->y;

		RECT sClipRect;
		sClipRect.left = max( 0, sSrcClipRect.left );
		sClipRect.right = min( pFrame->GetFrameSize()->x, sSrcClipRect.right );
		sClipRect.top = max( 0, sSrcClipRect.top );
		sClipRect.bottom = min( pFrame->GetFrameSize()->y, sSrcClipRect.bottom );

		if( sClipRect.right <= sClipRect.left || sClipRect.bottom <= sClipRect.top )
			return FALSE;

		//adjust pos[0]
		pOut->asPos[0].x += sClipRect.left;
		pOut->asPos[0].y += sClipRect.top;

		pOut->asPos[1].x = pOut->asPos[0].x + (float)(sClipRect.right - sClipRect.left);
		pOut->asPos[1].y = pOut->asPos[0].y + (float)(sClipRect.bottom - sClipRect.top);

		pOut->asUV[0] = D3DXVECTOR2(sClipRect.left / (float)pFrame->GetFrameSize()->x * pFrame->GetUVScale()->x, sClipRect.top / (float)pFrame->GetFrameSize()->y * pFrame->GetUVScale()->y);
		pOut->asUV[1] = D3DXVECTOR2(sClipRect.right / (float)pFrame->GetFrameSize()->x * pFrame->GetUVScale()->x, sClipRect.bottom / (float)pFrame->GetFrameSize()->y * pFrame->GetUVScale()->y);
	}
	else
	{
		pOut->asPos[1].x = pOut->asPos[0].x + (float)pFrame->GetFrameSize()->x;
		pOut->asPos[1].y = pOut->asPos[0].y + (float)pFrame->GetFrameSize()->y;

		pOut->asUV[0] = D3DXVECTOR2(0, 0);
		pOut->asUV[1] = *pFrame->GetUVScale();
	}

	//apply post scaling
	pOut->asPos[0].x *= g_sPicPostScaling.x;
	pOut->asPos[0].y *= g_sPicPostScaling.y;
	pOut->asPos[1].x *= g_sPicPostScaling.x;
	pOut->asPos[1].y *= g_sPicPostScaling.y;

	//patch pos
	pOut->asPos[0] -= D3DXVECTOR2(0.5, 0.5);
	pOut->asPos[1] -= D3DXVECTOR2(0.5, 0.5);

	return TRUE;
}

//batch
void BatchPicCmd(void* pCmd, void* pArg)
{
	KPicCmd* pIn = (KPicCmd*)pCmd;

	KPicDrawable elem;
	if(!SnapshotPic(pIn, (KPicCmdArg*)pArg, &elem))
	{
		return;
	}
	g_vecPicDrawable.push_back(elem);
}

//flush
void FlushPicBuf()
{
	if(!g_vecPicDrawable.empty())
	{
		DWORD dwPicCount = (DWORD)g_vecPicDrawable.size();

		DWORD dwBaseVertexIndex;
		LPD3DVB pVertexBuffer;
		KVertex1* pVertex = (KVertex1*)Gpu()->LockDynamicVertexBuffer(dwPicCount * 4, sizeof(KVertex1), &dwBaseVertexIndex, &pVertexBuffer);
		if(pVertex) 
		{
			//lock into vertex buffer
			for(DWORD nIndex = 0, i = 0; i < dwPicCount; i++)
			{
				KPicDrawable& rPic = g_vecPicDrawable[i];

				//top-left
				pVertex[nIndex].sPos = D3DXVECTOR4(rPic.asPos[0].x, rPic.asPos[0].y, 0, 1);
				pVertex[nIndex].dwColor = rPic.dwColor;
				pVertex[nIndex++].sUV = rPic.asUV[0];

				//top-right
				pVertex[nIndex].sPos = D3DXVECTOR4(rPic.asPos[1].x, rPic.asPos[0].y, 0, 1);
				pVertex[nIndex].dwColor = rPic.dwColor;
				pVertex[nIndex++].sUV = D3DXVECTOR2(rPic.asUV[1].x, rPic.asUV[0].y);

				//bottom-left
				pVertex[nIndex].sPos = D3DXVECTOR4(rPic.asPos[0].x, rPic.asPos[1].y, 0, 1);
				pVertex[nIndex].dwColor = rPic.dwColor;
				pVertex[nIndex++].sUV = D3DXVECTOR2(rPic.asUV[0].x, rPic.asUV[1].y);

				//bottom-right
				pVertex[nIndex].sPos = D3DXVECTOR4(rPic.asPos[1].x, rPic.asPos[1].y, 0, 1);
				pVertex[nIndex].dwColor = rPic.dwColor;
				pVertex[nIndex++].sUV = rPic.asUV[1];
			}
			Gpu()->UnlockDynamicVertexBuffer();

			//static states (see loop)

			//dynamic states 
			//stream source
			Gpu()->SetStreamSource(pVertexBuffer, sizeof(KVertex1));
			//indices
			Gpu()->SetIndices(g_pPicMultiquadIndexBuffer);
			//fvf
			Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
			//texture (see loop)

			//subset building & loop
			LPD3DTEXTURE			pTexture = g_vecPicDrawable[0].pTexture;
			KE_ALPHA_BLENDING_MODE	eAlphaBlendingMode = g_vecPicDrawable[0].eAlphaBlendingMode;
			DWORD					dwVertexStart = 0;
			DWORD					dwVertexCount = 4;
			DWORD					dwFaceStart = 0;
			DWORD					dwFaceCount = 2;

			//push back a dummy entry
			KPicDrawable elem;
			elem.pTexture = 0;
			g_vecPicDrawable.push_back(elem);

			for(DWORD i = 1; i < g_vecPicDrawable.size(); i++)
			{
				if(g_vecPicDrawable[i].pTexture != pTexture || g_vecPicDrawable[i].eAlphaBlendingMode != eAlphaBlendingMode)
				{
					//texture
					Gpu()->SetTexture(pTexture);

					//blending mode
					switch( eAlphaBlendingMode )
					{
					case emKE_ALPHA_BLENDING_MODE_ALPHA:
						Gpu()->SetStaticRenderStates( g_dwPicSrsAlpha );
						break;
					case emKE_ALPHA_BLENDING_MODE_COPY:
						Gpu()->SetStaticRenderStates( g_dwPicSrsCopy );
						break;
					case emKE_ALPHA_BLENDING_MODE_SCREEN:
						Gpu()->SetStaticRenderStates( g_dwPicSrsScreen );
						break;
					}

					//draw
					Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, dwBaseVertexIndex, dwVertexStart, dwVertexCount, dwFaceStart * 3, dwFaceCount);

					pTexture = g_vecPicDrawable[i].pTexture;
					eAlphaBlendingMode = g_vecPicDrawable[i].eAlphaBlendingMode;
					dwVertexStart += dwVertexCount;
					dwVertexCount = 4;
					dwFaceStart += dwFaceCount;
					dwFaceCount = 2;
				}
				else
				{
					dwVertexCount += 4;
					dwFaceCount += 2;
				}
			}
		}

		//clear buf
		g_vecPicDrawable.clear();
	}
}
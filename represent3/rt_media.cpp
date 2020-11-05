/* -------------------------------------------------------------------------
//	文件名		：	rt_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	rt_media_t的定义
//
// -----------------------------------------------------------------------*/
#include "rt_media.h"
#include "igpu.h"
#include "gpu_helper.h"

KRTConfig g_sRTConfig;

KRTMedia::KRTMedia(const KPOINT* pSize)
{
	m_cSize = *pSize;
	m_pBackupTexture = 0;
	m_pTexture = 0;
}

void KRTMedia::Process() 
{
	//texture size
	AdjustTextureSize(Gpu(), g_sRTConfig.bEnableConditionalNonPow2, false, g_sRTConfig.dwMipmap, &m_cSize, &m_cTextureSize);

	//uv scale
	m_sUVScale.x = (m_cTextureSize.x > m_cSize.x) ? (float)m_cSize.x/m_cTextureSize.x : 1;
	m_sUVScale.y = (m_cTextureSize.y > m_cSize.y) ? (float)m_cSize.y/m_cTextureSize.y : 1;
}

void KRTMedia::Clear()
{
	//backup
	if( m_pTexture )
	{
		if( !m_pBackupTexture )
		{
			Gpu()->CreateTexture(m_cTextureSize.x, m_cTextureSize.y, 1, 0, g_sRTConfig.eFmt, D3DPOOL_SYSTEMMEM, &m_pBackupTexture);
		}
		if( m_pBackupTexture )
		{
			LPD3DSURFACE pRTSurf = 0;
			LPD3DSURFACE bk_surf = 0;
			m_pTexture->GetSurfaceLevel(0, &pRTSurf);
			m_pBackupTexture->GetSurfaceLevel(0, &bk_surf);
			Gpu()->GetRenderTargetData(pRTSurf, bk_surf);
			SAFE_RELEASE(pRTSurf);
			SAFE_RELEASE(bk_surf);
		}
	}

	SAFE_RELEASE(m_pTexture); 
}

LPD3DTEXTURE KRTMedia::GetTexture() 
{
	//create video objects lazily
	if( !m_pTexture )
	{
		Gpu()->CreateTexture(m_cTextureSize.x, m_cTextureSize.y, 1, D3DUSAGE_RENDERTARGET, g_sRTConfig.eFmt, D3DPOOL_DEFAULT, &m_pTexture);

		//update texture
		if( m_pTexture && m_pBackupTexture )
		{
			m_pBackupTexture->AddDirtyRect(0);
			Gpu()->UpdateTexture(m_pBackupTexture, m_pTexture);
			SAFE_RELEASE(m_pBackupTexture);
		}
	}

	return m_pTexture;
}

BOOL InitRTCreator(void* pArg)
{
	KRTInitParam* param = (KRTInitParam*)pArg;
	g_sRTConfig.dwMipmap = param->dwMipmap;
	g_sRTConfig.bEnableConditionalNonPow2 = param->bEnableConditionalNonPow2;

	vector<D3DFORMAT> vecFmt;
	if( param->bUse16bitColor )
	{
		//prefer formats with alpha bits
		vecFmt.push_back( D3DFMT_A1R5G5B5 );
		vecFmt.push_back( D3DFMT_A4R4G4B4 );
		vecFmt.push_back( D3DFMT_A8R8G8B8 );
		vecFmt.push_back( D3DFMT_R5G6B5 );
		vecFmt.push_back( D3DFMT_X1R5G5B5 );
		vecFmt.push_back( D3DFMT_X8R8G8B8 );
	}
	else
	{
		//prefer formats with alpha bits
		vecFmt.push_back( D3DFMT_A8R8G8B8 );
		vecFmt.push_back( D3DFMT_A1R5G5B5 );
		vecFmt.push_back( D3DFMT_A4R4G4B4 );
		vecFmt.push_back( D3DFMT_X8R8G8B8 );
		vecFmt.push_back( D3DFMT_R5G6B5 );
		vecFmt.push_back( D3DFMT_X1R5G5B5 );
	}
	g_sRTConfig.eFmt = FindFirstAvaiableRenderTargetTextureFormat( Gpu(), &vecFmt[0], (DWORD)vecFmt.size() );

	return g_sRTConfig.eFmt != D3DFMT_UNKNOWN;
};

void ShutdownRTCreator()
{

}

KMedia* CreateRT(LPCSTR pszIdentifier, void* pArg) 
{
	return new KRTMedia((KPOINT*)pArg);
}
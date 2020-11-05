/* -------------------------------------------------------------------------
//	文件名		：	jpg_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	jpg_media_t定义
//
// -----------------------------------------------------------------------*/
#include "../represent_common/common.h"
#include "jpg_media.h"
#include "igpu.h"
#include "gpu_helper.h"

KJpgConfig g_JpgConfig;

KJpgMedia::KJpgMedia(LPCSTR pszFilename)
{
	m_strFileName = pszFilename;
	m_pFileData = 0;
	m_pTexture = 0;
}

void KJpgMedia::Process()
{
	//check load() step
	if(!m_pFileData)
		return;

	//size
	D3DXIMAGE_INFO img_info;
	if( FAILED(D3DXGetImageInfoFromFileInMemory(m_pFileData->GetBufferPointer(), m_pFileData->GetBufferSize(), &img_info)) )
	{
		Clear();
		return;
	}
	m_sSize = KPOINT(img_info.Width, img_info.Height);

	//texture size
	AdjustTextureSize(Gpu(), g_JpgConfig.m_bEnableConditionalNonPow2, g_JpgConfig.m_sFmt == D3DFMT_DXT1, g_JpgConfig.m_dwMipmap, &m_sSize, &m_sTextureSize);

	//uv scale
	if( m_sTextureSize.x >= m_sSize.x && m_sTextureSize.y >= m_sSize.y )
		m_sUVScale = D3DXVECTOR2((float)m_sSize.x/m_sTextureSize.x, (float)m_sSize.y/m_sTextureSize.y);
	else
		m_sUVScale = D3DXVECTOR2(1, 1);
}

void KJpgMedia::Produce()
{
	//check process() step
	if(!m_pFileData)
		return;

	//texture
	if( !Gpu()->CreateTexture(m_sTextureSize.x, m_sTextureSize.y, 1, 0, g_JpgConfig.m_sFmt, D3DPOOL_MANAGED, &m_pTexture) )
	{
		Clear();
		return;
	}

	PlusMediaBytes( (DWORD)(m_sTextureSize.x * m_sTextureSize.y * FormatBytes(g_JpgConfig.m_sFmt)) );

	LPD3DSURFACE surface;
	m_pTexture->GetSurfaceLevel(0, &surface);

	if( m_sTextureSize.x >= m_sSize.x && m_sTextureSize.y >= m_sSize.y )
	{
		RECT rc = {0, 0, m_sSize.x, m_sSize.y};
		if( FAILED(D3DXLoadSurfaceFromFileInMemory(surface, 0, &rc, m_pFileData->GetBufferPointer(), m_pFileData->GetBufferSize(), &rc, D3DX_FILTER_NONE, 0, 0)) )
		{
			surface->Release();
			Clear();
			return;
		}
	}
	else
	{
		RECT src_rc = {0, 0, m_sSize.x, m_sSize.y};
		RECT dst_rc = {0, 0, m_sTextureSize.x, m_sTextureSize.y};
		if( FAILED(D3DXLoadSurfaceFromFileInMemory(surface, 0, &dst_rc, m_pFileData->GetBufferPointer(), m_pFileData->GetBufferSize(), &src_rc, D3DX_FILTER_LINEAR, 0, 0)) )
		{
			surface->Release();
			Clear();
			return;
		}
	}

	surface->Release();

	//free process() step
	SAFE_RELEASE(m_pFileData);
}

void KJpgMedia::Clear()
{
	SAFE_RELEASE(m_pFileData);
	if( m_pTexture )
	{
		m_pTexture->Release();
		m_pTexture = 0;
		MinusMediaBytes( (DWORD)(m_sTextureSize.x * m_sTextureSize.y * FormatBytes(g_JpgConfig.m_sFmt)) );
	}
}

BOOL InitJpgCreator(void* arg)
{
	KJpgInitParam* pParam = (KJpgInitParam*)arg;

	g_JpgConfig.m_bEnableConditionalNonPow2 = pParam->m_bEnableConditionalNonPow2;
	g_JpgConfig.m_dwMipmap = pParam->m_dwMipmap;

	//fmt
	vector<D3DFORMAT> vecFmtVec;
	if( pParam->m_bUseCompressedTextureFormat )
	{
		if( pParam->m_bUse16bitColor )
		{
			vecFmtVec.push_back( D3DFMT_DXT1 );
			vecFmtVec.push_back( D3DFMT_R5G6B5 );
			vecFmtVec.push_back( D3DFMT_X1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A4R4G4B4 );
			vecFmtVec.push_back( D3DFMT_X8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_A8R8G8B8 );
		}
		else
		{
			vecFmtVec.push_back( D3DFMT_DXT1 );
			vecFmtVec.push_back( D3DFMT_X8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_A8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_R5G6B5 );
			vecFmtVec.push_back( D3DFMT_X1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A4R4G4B4 );
		}
	}
	else
	{
		if( pParam->m_bUse16bitColor )
		{
			vecFmtVec.push_back( D3DFMT_R5G6B5 );
			vecFmtVec.push_back( D3DFMT_X1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A4R4G4B4 );
			vecFmtVec.push_back( D3DFMT_X8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_A8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_DXT1 );
		}
		else
		{
			vecFmtVec.push_back( D3DFMT_X8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_A8R8G8B8 );
			vecFmtVec.push_back( D3DFMT_R5G6B5 );
			vecFmtVec.push_back( D3DFMT_X1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A1R5G5B5 );
			vecFmtVec.push_back( D3DFMT_A4R4G4B4 );
			vecFmtVec.push_back( D3DFMT_DXT1 );
		}
	}
	g_JpgConfig.m_sFmt = FindFirstAvaiableOffscrTextureFormat( Gpu(), &vecFmtVec[0], (DWORD)vecFmtVec.size() );

	return g_JpgConfig.m_sFmt != D3DFMT_UNKNOWN;
}


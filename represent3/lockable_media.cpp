/* -------------------------------------------------------------------------
//	文件名		：	lockable_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	lockable_media_t和lockable_frame_t的定义
//
// -----------------------------------------------------------------------*/
#include "lockable_media.h"
#include "igpu.h"
#include "gpu_helper.h"

KLockableConfig g_sLockableConfig;

KLockableMedia::KLockableMedia( const KPOINT* _size )
{
	m_cSize = *_size;
	m_pTexture = 0;
	AdjustTextureSize(Gpu(), g_sLockableConfig.bEnableConditionalNonPow2, FALSE, 0, &m_cSize, &m_cTextureSize);
	if(m_cTextureSize.x < m_cSize.x || m_cTextureSize.y < m_cSize.y)	//enlarge only
		return;
	m_sUVScale = D3DXVECTOR2((FLOAT)m_cSize.x/m_cTextureSize.x, (FLOAT)m_cSize.y/m_cTextureSize.y);
	Gpu()->CreateTexture(m_cTextureSize.x, m_cTextureSize.y, 1, 0, g_sLockableConfig.eFmt, D3DPOOL_MANAGED, &m_pTexture);

	if( m_pTexture )
	{
		PlusMediaBytes( (DWORD)(m_cTextureSize.x * m_cTextureSize.y * FormatBytes(g_sLockableConfig.eFmt)) );
	}
}

void KLockableMedia::Destroy()
{
	Clear();
	if( m_pTexture )
	{
		MinusMediaBytes( (DWORD)(m_cTextureSize.x * m_cTextureSize.y * FormatBytes(g_sLockableConfig.eFmt)) );
		SAFE_RELEASE(m_pTexture);
	}
	delete this;
}

void* KLockableMedia::Lock(DWORD* pitch)
{
	D3DLOCKED_RECT locked_rect;
	RECT rc = {0, 0, m_cSize.x, m_cSize.y};
	if( FAILED(m_pTexture->LockRect(0, &locked_rect, &rc, 0)) )
		return 0;
	*pitch = locked_rect.Pitch;
	return locked_rect.pBits;
}

void KLockableMedia::UnLock()
{
	m_pTexture->UnlockRect(0);
}

BOOL InitLockableCreator(void* arg)
{
	KLockableInitParam* param = (KLockableInitParam*)arg;

	g_sLockableConfig.bEnableConditionalNonPow2 = param->bEnableConditionalNonPow2;

	vector<D3DFORMAT> vecFmt;
	vecFmt.push_back( D3DFMT_R5G6B5 );
	vecFmt.push_back( D3DFMT_X1R5G5B5 );
	vecFmt.push_back( D3DFMT_A1R5G5B5 );
	g_sLockableConfig.eFmt = FindFirstAvaiableOffscrTextureFormat( Gpu(), &vecFmt[0], (DWORD)vecFmt.size() );

	return g_sLockableConfig.eFmt != D3DFMT_UNKNOWN;
}

KMedia* CreateLockable(LPCSTR pszIdentifier, void* pArg) 
{
	return new KLockableMedia((KPOINT*)pArg);
}

void ShutdownLockableCreator()
{

}
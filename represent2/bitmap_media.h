/* -------------------------------------------------------------------------
//	文件名		：	bitmap_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	bitmap_media_t定义
//
// -----------------------------------------------------------------------*/
#ifndef __BITMAP_MEDIA_H__
#define __BITMAP_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "../represent_common/ifile.h"
#include "bitmap.h"
//media - bitmap
class KBitmapMedia : public KMedia, public IKBitmap_t
{
public:
	//src
	KPOINT		m_cSize;
	LPD3DBUFFER	m_pData;
	DWORD		m_dwFormat;

	KBitmapMedia(const KPOINT* pSize, DWORD dwFormat)
	{
		m_cSize = *pSize;
		m_pData = 0;
		D3DXCreateBuffer( m_cSize.x * m_cSize.y * 2, &m_pData );
		if( m_pData )
		{
			PlusMediaBytes( m_pData->GetBufferSize() );
		}
		m_dwFormat = dwFormat;
	}

	//media
	void Load() {}
	void Process() {}
	void Produce() {}
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear() {}
	void Destroy() 
	{
		if( m_pData )
		{
			MinusMediaBytes( m_pData->GetBufferSize() );
			SAFE_RELEASE(m_pData); 
		}
		delete this;
	}
	void* Product() {return m_pData ? (IKBitmap_t*)this : 0;}

	//bitmap_t
	const KPOINT*	GetSize() {return &m_cSize;}
	void*			GetData() {return m_pData->GetBufferPointer();}
	DWORD			GetFormat() {return m_dwFormat;}
};

struct KCreateBitmapParam
{
	KPOINT m_pSize;
	DWORD m_dwFormat;
};

BOOL InitBitmapCreator(void* pArg);
void ShutdownBitmapCreator();
KMedia* CreateBitmap(LPCSTR pszIdentifier, void* pArg);
#endif
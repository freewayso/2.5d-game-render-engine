/* -------------------------------------------------------------------------
//	文件名		：	spr_frame_media.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	spr_frame_media_t定义
//
// -----------------------------------------------------------------------*/
#ifndef __SPR_FRAME_MEDIA_H__
#define __SPR_FRAME_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "../represent_common/ifile.h"
//media - spr
struct KSprConfig
{
	D3DFORMAT	eFmt;
	BOOL		bEnableConditionalNonPow2;
	DWORD		dwMipmap;
};

extern KSprConfig g_sSprConfig;

class KSprFrameMedia : public KMedia, public KSpriteFrame
{
public:
	//src
	BOOL			m_bPacked;
	LPD3DBUFFER 	m_pPal;

	LPCSTR			m_pszFileName;		//for packed only
	DWORD			m_dwFrameIndex;		//for packed only	
	DWORD			m_dwInfodataSize;	//for packed only

	LPD3DBUFFER 	m_pInfoData;		//for unpacked only

	//loaded
	//	buffer_t		info_data;		//for packed only

	//processed
	KPOINT			m_cOffset;
	KPOINT			m_cSize;
	KPOINT			m_cTextureSize;
	D3DXVECTOR2		m_sUVScale;
	LPD3DBUFFER		m_pTextureRawBits;

	//produced
	//	int2			offset;
	//	int2			size;
	//	int2			texture_size;
	//	D3DXVECTOR2			uv_scale;
	LPD3DTEXTURE	m_pTexture;

	//ctor
	KSprFrameMedia(LPD3DBUFFER pPal, LPD3DBUFFER pInfoData);	//unpacked
	KSprFrameMedia(LPD3DBUFFER pPal, LPCSTR pszFileName, DWORD dwFrameIndex, DWORD dwInfodataSize);	//packed

	//media
	void Load();
	void Process();
	void Produce();
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear();
	void Destroy() 
	{
		Clear(); 
		if( m_pPal )
		{
			DWORD dwPalSize = m_pPal->GetBufferSize();
			if( m_pPal->Release() == 0 )
				MinusMediaBytes(dwPalSize);
			m_pPal = 0;
		}
		if( !m_bPacked ) 
		{
			if( m_pInfoData )
			{
				DWORD dwSize = m_pInfoData->GetBufferSize();
				if(m_pInfoData->Release() == 0)
					MinusMediaBytes(dwSize);
				m_pInfoData = 0;
			}
		}
		delete this;
	}
	void* Product() {return m_pTexture ? (KSpriteFrame*)this : 0;}

	//sprite frame
	const KPOINT*		GetFrameSize()					{return &m_cSize;}
	const KPOINT*		GetFrameOffset()				{return &m_cOffset;}
	const KPOINT*		GetTextureSize()				{return &m_cTextureSize;}
	LPD3DTEXTURE		GetTexture()					{return m_pTexture;}
	const D3DXVECTOR2*	GetUVScale()					{return &m_sUVScale;}
};

KMedia* CreateUnpackedSprFrame(LPD3DBUFFER pPal, LPD3DBUFFER pInfoData);
KMedia* CreatePackedSprFrame(LPD3DBUFFER pPal, LPCSTR pszFileName, DWORD dwFrameIndex, DWORD dwFiledataSize);

void DecompressSpr32bit(WORD* pDest, int nPitch, BYTE* pSrc, int width, int height, BYTE* pPalette);
void DecompressSpr16bit(WORD* pDest, int nPitch, BYTE* pSrc, int width, int height, BYTE* pPalette);

#endif
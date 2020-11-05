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

struct KSprConfig
{
	BOOL	m_bRgb565;
	DWORD	m_dwAlphaLevel;
};
extern KSprConfig g_sSprConfig;

class KSprFrameMedia : public KMedia, public KSpriteFrame
{
public:
	//src
	BOOL			m_bPacked;

	LPD3DBUFFER		m_pDataBuf;			//for unpacked only
	KPOINT			m_cOffset;			//for unpacked only
	KPOINT			m_cSize;			//for unpacked only

	LPCSTR			m_pszFileName;		//for packed only
	DWORD			m_dwFrameIndex;		//for packed only	
	DWORD			m_dwInfodataSize;	//for packed only	

	//loaded
	LPD3DBUFFER 	m_pInfoDataBuf;		//for packed only

	//processed
	//	int2			offset;
	//	int2			size;
	//	buffer_t		data_buf;

	//produced = processed

	//ctor 
	KSprFrameMedia(LPD3DBUFFER pDataBuf, const KPOINT* pOffset, const KPOINT* pSize);	//for unpacked frame
	KSprFrameMedia(LPCSTR pszFilename, DWORD dwFrameIndex, DWORD dwInfodataSize);	//for packed frame

	//media
	void Load();
	void Process();
	void Produce() {}
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear();
	void Destroy() 
	{
		Clear();
		if( !m_bPacked ) 
		{
			if( m_pDataBuf )
				MinusMediaBytes( m_pDataBuf->GetBufferSize() );
			SAFE_RELEASE(m_pDataBuf); 
		}
		delete this;
	}
	void* Product() {return m_pDataBuf ? (KSpriteFrame*)this : 0;}

	//sprite frame
	const KPOINT*		GetFrameSize()				{return &m_cSize;}
	const KPOINT*		GetFrameOffset()				{return &m_cOffset;}
	void*			GetFrameData()				{return m_pDataBuf->GetBufferPointer();}
};

KMedia* CreateUnpackedSprFrame(LPD3DBUFFER pDataBuf, const KPOINT* pOffset, const KPOINT* pSize);
KMedia* CreatePackedSprFrame( LPCSTR pszFilename, DWORD dwFrameIndex, DWORD dwInfodataSize );
void CompressSpr(void* pIn, DWORD dwInSize, INT nWidth, INT nHeight, INT nAlphaLevel, LPD3DBUFFER* pOut);
#endif
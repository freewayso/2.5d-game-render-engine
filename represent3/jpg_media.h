 /* -------------------------------------------------------------------------
//	文件名		：	jpg_media_t.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	jpg_media_t定义
//
// -----------------------------------------------------------------------*/
#ifndef __JPG_MEDIA_H__
#define __JPG_MEDIA_H__
#include "../represent_common/common.h"
#include "../represent_common/media.h"
#include "sprite.h"
#include "media_center.h"
#include "../represent_common/ifile.h"

//media - jpg
struct KJpgConfig
{
	D3DFORMAT	m_sFmt;
	BOOL		m_bEnableConditionalNonPow2;
	DWORD		m_dwMipmap;
};

extern KJpgConfig g_JpgConfig;

struct KJpgInitParam
{
	BOOL	m_bUseCompressedTextureFormat;
	BOOL	m_bUse16bitColor;
	BOOL	m_bEnableConditionalNonPow2;
	DWORD	m_dwMipmap;
};

class KJpgMedia : public KMedia, public KSprite, public KSpriteFrame
{
public:
	//src
	string			m_strFileName;

	//loaded
	LPD3DBUFFER		m_pFileData;

	//processed
	//	LPD3DBUFFER		m_pFileData;
	KPOINT				m_sSize;
	KPOINT				m_sTextureSize;
	D3DXVECTOR2			m_sUVScale;

	//produced
	//	int2			m_sSize;
	//	int2			m_sTextureSize;
	//	float2			m_sUVScale;
	LPD3DTEXTURE		m_pTexture;

	//ctor
	KJpgMedia(LPCSTR pszFileName);

	//media
	void Load() {LoadFile(m_strFileName.c_str(), &m_pFileData);}
	void Process();
	void Produce();
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear();
	void Destroy() {Clear(); delete this;}
	void* Product() {return m_pTexture ? (KSprite*)this : 0;}

	//sprite
	const KPOINT*	GetSize()					{return &m_sSize;}
	const KPOINT*	GetCenter()				{return &g_PointZero;}
	DWORD		GetExchangeColorCount()	{return 0;}
	DWORD		GetBlendStyle()			{return 0;}
	DWORD		GetDirectionCount()		{return 1;}
	DWORD		GetInterval()				{return 1;}
	DWORD		GetFrameCount()			{return 1;}
	BOOL		QueryFrame(DWORD dwMyHandle, DWORD dwIndex, BOOL bWait, void** ppFrame) {*ppFrame = (KSpriteFrame*)this; return TRUE;}

	//sprite frame
	const KPOINT*		GetFrameSize()				{return &m_sSize;}
	const KPOINT*		GetFrameOffset()				{return &g_PointZero;}
	const KPOINT*		GetTextureSize()				{return &m_sTextureSize;}
	LPD3DTEXTURE		GetTexture()					{return m_pTexture;}
	const D3DXVECTOR2*	GetUVScale()					{return &m_sUVScale;}
};

BOOL InitJpgCreator(void* pArg);
inline void ShutdownJpgCreator() {}
inline KMedia* CreateJpg(LPCSTR pszIdentifier, void* pArg) {return new KJpgMedia(pszIdentifier);}

#endif
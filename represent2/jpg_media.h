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
#include "bitmap.h"
//media - jpg
struct KJpgConfig
{
	BOOL m_bRgb565;
};
extern KJpgConfig g_JpgConfig;

struct KJpgInitParam
{
	BOOL m_bRgb565;
};

struct KJpgMedia : public KMedia, public IKBitmap_t
{
	//src
	string		m_strFileName;

	//load
	LPD3DBUFFER	m_pFileData;

	//process
	KPOINT		m_sSize;
	LPD3DBUFFER	m_pData;

	KJpgMedia(LPCSTR pszFilename)
	{
		m_strFileName = pszFilename;
		m_pFileData = 0;
		m_pData = 0;
	}

	//media
	void Load() {LoadFile(m_strFileName.c_str(), &m_pFileData);}
	void Process();
	void Produce() {}
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear();
	void Destroy() {Clear(); delete this;}
	void* Product() {return m_pData ? (IKBitmap_t*)this : 0;}

	//bitmap_t
	const KPOINT* GetSize() {return &m_sSize;}
	void*		GetData() {return m_pData->GetBufferPointer();}
	DWORD		GetFormat() {return 0;}
};

BOOL InitJpgCreator(void* pArg);
void ShutdownJpgCreator();
KMedia* CreateJpg(LPCSTR pszIdentifier, void* pArg);

#endif
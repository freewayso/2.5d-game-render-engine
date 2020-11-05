/* -------------------------------------------------------------------------
//	文件名		：	jpg_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	jpg_media_t定义
//
// -----------------------------------------------------------------------*/
#include "../represent_common/common.h"
#include "jpg_media.h"
#include "jpglib.h"
//media - jpg
KJpgConfig g_JpgConfig;
BOOL InitJpgCreator(void* pArg)
{
	KJpgInitParam* pParam = (KJpgInitParam*)pArg;
	g_JpgConfig.m_bRgb565 = pParam->m_bRgb565;
	return true;
}

void ShutdownJpgCreator() {}

void KJpgMedia::Process()
{
	//check
	if( !m_pFileData )
	{
		Clear();
		return;
	}

	EnterJpglib();

	//process
	if( !jpeg_decode_init( !g_JpgConfig.m_bRgb565, true) )
	{
		LeaveJpglib();
		Clear();
		return;
	}

	JPEG_INFO info;
	if( !jpeg_decode_info( (BYTE*)m_pFileData->GetBufferPointer(), &info ) )
	{
		LeaveJpglib();
		Clear();
		return;
	}

	m_sSize.x = info.nWidth;
	m_sSize.y = info.nHeight;

	if( FAILED(D3DXCreateBuffer(info.nWidth * info.nHeight * 2, &m_pData)) )
	{
		LeaveJpglib();
		Clear();
		return;
	}

	PlusMediaBytes( m_pData->GetBufferSize() );

	if( !jpeg_decode_data( (WORD*)m_pData->GetBufferPointer(), &info ) )
	{
		LeaveJpglib();
		Clear();
		return;
	}

	LeaveJpglib();

	//free
	SAFE_RELEASE(m_pFileData);
}

void KJpgMedia::Clear()
{
	SAFE_RELEASE(m_pFileData);
	if( m_pData )
	{
		MinusMediaBytes( m_pData->GetBufferSize() );
		m_pData->Release();
		m_pData = 0;
	}
}

KMedia* CreateJpg(LPCSTR identifier, void* arg) {return new KJpgMedia(identifier);}
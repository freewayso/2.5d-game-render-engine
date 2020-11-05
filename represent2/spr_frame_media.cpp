/* -------------------------------------------------------------------------
//	文件名		：	spr_frame_media.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	spr帧定义
//
// -----------------------------------------------------------------------*/
#include "spr_frame_media.h"
KSprConfig g_sSprConfig;

KSprFrameMedia::KSprFrameMedia(LPD3DBUFFER _data_buf, const KPOINT* _offset, const KPOINT* _size)
{
	m_bPacked = false;
	m_pDataBuf = _data_buf;
	if( _data_buf ) 
	{
		PlusMediaBytes(_data_buf->GetBufferSize());
		_data_buf->AddRef();
	}
	m_cOffset = _offset ? (*_offset) : KPOINT(0, 0);
	m_cSize = _size ? (*_size) : KPOINT(0, 0);
}

KSprFrameMedia::KSprFrameMedia(LPCSTR pszFilename, DWORD dwFrameIndex, DWORD dwInfodataSize)
{
	m_bPacked = true;
	m_pszFileName = pszFilename;			//just save the ptr
	m_dwFrameIndex = dwFrameIndex;
	m_dwInfodataSize = dwInfodataSize;
	m_pInfoDataBuf = 0;				//not loaded yet
	m_pDataBuf = 0;					//not processed yet
}

void KSprFrameMedia::Load()
{
	//ok for packed
	if( !m_bPacked )
		return; 

	if( m_pszFileName )
	{
		LoadFileFragment(m_pszFileName, m_dwFrameIndex + 2, &m_pInfoDataBuf);
	}
}

void KSprFrameMedia::Process()
{
	//ok for packed
	if( !m_bPacked )
		return; 

	//previous step check
	if( !m_pInfoDataBuf )
	{
		Clear();
		return;
	}

	struct KFrameInfo
	{
		WORD	wWidth;
		WORD	wHeight;
		WORD	wOffsetX;
		WORD	wOffsetY;
	};

	//size check
	if( m_dwInfodataSize )
	{
		if( m_dwInfodataSize < sizeof(KFrameInfo) )
		{
			Clear();
			return;
		}

		if( (m_pInfoDataBuf->GetBufferSize() != m_dwInfodataSize) )
		{
			Clear();
			return;
		}
	}

	//process
	KFrameInfo* pFrameInfo = (KFrameInfo*)m_pInfoDataBuf->GetBufferPointer();
	m_cSize.x = (INT)pFrameInfo->wWidth;
	m_cSize.y = (INT)pFrameInfo->wHeight;
	m_cOffset.x = (INT)pFrameInfo->wOffsetX;
	m_cOffset.y = (INT)pFrameInfo->wOffsetY;
	CompressSpr((void*)&pFrameInfo[1], m_pInfoDataBuf->GetBufferSize() - sizeof(KFrameInfo), m_cSize.x, m_cSize.y, g_sSprConfig.m_dwAlphaLevel, &m_pDataBuf);
	if( m_pDataBuf )
		PlusMediaBytes( m_pDataBuf->GetBufferSize() );

	SAFE_RELEASE(m_pInfoDataBuf);
}

void KSprFrameMedia::Clear()
{
	//ok for packed
	if( !m_bPacked )
		return;

	if( m_pDataBuf )
		MinusMediaBytes(m_pDataBuf->GetBufferSize());
	SAFE_RELEASE(m_pDataBuf);
	SAFE_RELEASE(m_pInfoDataBuf);
}

KMedia* CreateUnpackedSprFrame(LPD3DBUFFER pszDataBuf, const KPOINT* pOffset, const KPOINT* pSize) { return new KSprFrameMedia( pszDataBuf, pOffset, pSize ); }
KMedia* CreatePackedSprFrame( LPCSTR pszFilename, DWORD dwFrameIndex, DWORD dwInfodataSize ) { return new KSprFrameMedia( pszFilename, dwFrameIndex, dwInfodataSize ); }

void CompressSpr(void* pIn, DWORD dwInSize, INT nWidth, INT nHeight, INT nAlphaLevel, LPD3DBUFFER* pOut)
{
	if( !pIn || !dwInSize || !nWidth || !nHeight )
	{
		*pOut = 0;
		return;
	}

	if( nAlphaLevel >= 256 )
	{
		LPD3DBUFFER pBuf = 0;
		D3DXCreateBuffer(dwInSize, &pBuf);
		if(pBuf)
			memcpy((void*)pBuf->GetBufferPointer(), pIn, dwInSize);
		*pOut = pBuf;
		return;
	}

	if( nAlphaLevel <= 1 )
		nAlphaLevel = 2;

	BYTE* pSrc = (BYTE*)pIn;
	BYTE* pDst = pSrc;
	for( INT row = 0; row < nHeight; row++ )
	{
		INT nPixelCount = nWidth;
		nPixelCount -= *pSrc;

		if( pDst == pSrc )
		{
			pSrc = pSrc + 2 + ((*(pSrc+1)) ? (*pSrc) : 0);
			INT alpha = *(pDst+1);
			if( alpha == 255 )
				alpha = 254;
			*(pDst+1) = (alpha * nAlphaLevel / 255) * 255 / (nAlphaLevel - 1);
		}
		else
		{
			INT nAlpha = *(pSrc+1);
			if( nAlpha == 255 )
				nAlpha = 254;
			INT nNewAlpha = (nAlpha * nAlphaLevel / 255) * 255 / (nAlphaLevel - 1);

			if( nNewAlpha == 0 )
			{
				INT length = 2 + ((*(pSrc+1)) ? (*pSrc) : 0);
				*pDst = *pSrc;
				*(pDst+1) = 0;
				pSrc += length;
			}
			else
			{
				INT nLength = 2 + ((*(pSrc+1)) ? (*pSrc) : 0);
				for( INT i = 0; i < nLength; i++ )
				{
					pDst[i] = pSrc[i];
				}
				pDst[1] = nNewAlpha;
				pSrc += nLength;
			}
		}

		while( nPixelCount > 0 )
		{
			nPixelCount -= (*pSrc);
			INT nAlpha = *(pSrc+1);
			if( nAlpha == 255 )
				nAlpha = 254;
			INT nNewAlpha = (nAlpha * nAlphaLevel / 255) * 255 / (nAlphaLevel - 1);

			if( nNewAlpha == *(pDst+1) )
			{
				INT nNewCount = (*pDst) + (*pSrc);
				INT nNewCount1, nNewCount2;
				if( nNewCount > 255 )
				{
					nNewCount1 = 255;
					nNewCount2 = nNewCount - 255;
				}
				else
				{
					nNewCount1 = nNewCount;
					nNewCount2 = 0;
				}

				if( (*(pDst+1)) == 0 )
				{
					pSrc = pSrc + 2 + ((*(pSrc+1))? (*pSrc) : 0);
					*pDst = nNewCount1;
					if( nNewCount2 > 0 )
					{
						pDst += 2;
						*pDst = nNewCount2;
						*(pDst+1) = 0;
					}
				}
				else
				{
					INT nLength1Add = nNewCount1 - (*pDst);
					BYTE* dst_add = pDst + 2 + (*pDst); 
					*pDst = nNewCount1;
					for( INT i = 0; i < nLength1Add; i++ )
					{
						dst_add[i] = pSrc[2+i];
					}
					if( nNewCount2 > 0 )
					{
						pDst = pDst + 2 + (*pDst); 
						*pDst = nNewCount2;
						*(pDst+1) = nNewAlpha;
						for( INT i = 0; i < nNewCount2; i++ )
						{
							pDst[i+2] = pSrc[2+i+nLength1Add];
						}
					}
					pSrc = pSrc+2 + nLength1Add + nNewCount2;
				}
			}
			else
			{
				pDst = pDst + 2 + ((*(pDst+1)) ? (*pDst) : 0);

				if( pDst == pSrc )
				{
					pSrc = pSrc + 2 + ((*(pSrc+1)) ? (*pSrc) : 0);
					INT alpha = *(pDst+1);
					if( alpha == 255 )
						alpha = 254;
					*(pDst+1) = (alpha * nAlphaLevel / 255) * 255 / (nAlphaLevel - 1);
				}
				else
				{
					if( nNewAlpha == 0 )
					{
						INT nLength = 2 + ((*(pSrc+1)) ? (*pSrc) : 0);
						*pDst = *pSrc;
						*(pDst+1) = 0;
						pSrc += nLength;
					}
					else
					{
						INT nLength = 2 + ((*(pSrc+1)) ? (*pSrc) : 0);
						for( INT i = 0; i < nLength; i++ )
						{
							pDst[i] = pSrc[i];
						}
						pDst[1] = nNewAlpha;
						pSrc += nLength;
					}
				}
			}
		}

		pDst = pDst + 2 + ((*(pDst+1)) ? (*pDst) : 0);
	}

	INT nBufSize = (INT)(pDst - (BYTE*)pIn);

	LPD3DBUFFER pBuf = 0;
	D3DXCreateBuffer(nBufSize, &pBuf);
	if( pBuf )
		memcpy(pBuf->GetBufferPointer(), pIn, pBuf->GetBufferSize());
	*pOut = pBuf;
}

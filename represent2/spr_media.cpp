/* -------------------------------------------------------------------------
//	文件名		：	spr_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	spr_media_t定义
//
// -----------------------------------------------------------------------*/
#include "spr_media.h"

KSprMedia::KSprMedia(LPCSTR pszFileName)
{
	if( pszFileName )
		strcpy(m_szFilename, pszFileName);
	else
		m_szFilename[0] = 0;
	m_pFiledata = 0;				//not loaded yet
	m_pOffsetTableBuf = 0;		//not loaded yet
	m_pPalBuf = 0;				//not processed yet
	m_pExchangePalBuf = 0;
}

void KSprMedia::Load()
{
	IFile* file = g_pfnOpenFile(m_szFilename, false, false);
	if( !file )
		return;
	if( file->IsPackedByFragment() )
	{
		m_bPacked = TRUE;
		LoadFileFragment(file, 0, &m_pFiledata);
		LoadFileFragment(file, 1, &m_pOffsetTableBuf);
	}
	else
	{
		m_bPacked = FALSE;
		LoadFile(file, &m_pFiledata);
	}
	file->Release();
}

void KSprMedia::Process()
{
	//previous step check
	if( !m_pFiledata )
	{
		Clear();
		return;
	}

	struct KHead
	{
		BYTE	byComment[4];	
		WORD	wWidth;		
		WORD	wHeight;		
		WORD	wCenterX;	
		WORD	wCenterY;	
		WORD	wFrameCount;
		WORD	wPalEntryCount;
		WORD	wDirectionCount;
		WORD	wInterval;
		WORD	wReserved[6];
	};

	struct KOffsetTable
	{
		DWORD	wOffset;
		DWORD	wSize;
	};

	//check completment for head
	if( m_pFiledata->GetBufferSize() < sizeof(KHead) )
	{
		Clear();
		return;
	}

	//load head info
	KHead* pHead = (KHead*)m_pFiledata->GetBufferPointer();

	if( (*(int*)&pHead->byComment[0] != 0x525053) || pHead->wPalEntryCount  != 256 )
	{
		Clear();
		return;
	}

	m_cSize					= KPOINT(pHead->wWidth, pHead->wHeight);
	m_cCenter					= KPOINT(pHead->wCenterX, pHead->wCenterY);
	m_dwColorCount				= pHead->wPalEntryCount;
	m_dwExchangeColorCount	= pHead->wReserved[4] & 0xff;
	m_dwBlendStyle				= pHead->wReserved[1] & 0xff;
	m_dwDirectionCount			= pHead->wDirectionCount;
	m_dwInterval				= pHead->wInterval;

	//check completment for palette
	if( m_bPacked )
	{
		if( m_pFiledata->GetBufferSize() != (sizeof(KHead) + 3 * pHead->wPalEntryCount) )
		{
			Clear();
			return;
		}
	}
	else
	{
		if( m_pFiledata->GetBufferSize() < (sizeof(KHead) + 3 * pHead->wPalEntryCount) )
		{
			Clear();
			return;
		}
	}

	//get palette ptr
	void* pPalPtr = (void*)&pHead[1];

	if( m_dwExchangeColorCount > 0 )
	{
		D3DXCreateBuffer( 3 * m_dwExchangeColorCount, &m_pExchangePalBuf );
		memcpy( m_pExchangePalBuf->GetBufferPointer(), (BYTE*)pPalPtr + (m_dwColorCount - m_dwExchangeColorCount) * 3, m_pExchangePalBuf->GetBufferSize() );
	}

	//convert to 16bit palette in place
	Pal24ToPal16( pPalPtr, pPalPtr, m_dwColorCount, g_sSprConfig.m_bRgb565 );

	//create pal_buf
	D3DXCreateBuffer( m_dwColorCount * 2, &m_pPalBuf );
	memcpy(m_pPalBuf->GetBufferPointer(), pPalPtr, m_pPalBuf->GetBufferSize());
	PlusMediaBytes(m_pPalBuf->GetBufferSize());

	//check for offset_table
	if( m_bPacked )
	{
		if( m_pOffsetTableBuf && (m_pOffsetTableBuf->GetBufferSize() != pHead->wFrameCount * sizeof(KOffsetTable)) )
		{
			Clear();
			return;
		}
	}
	else
	{
		if( m_pFiledata->GetBufferSize() < (sizeof(KHead) + 3 * pHead->wPalEntryCount + pHead->wFrameCount * sizeof(KOffsetTable)) )
		{
			Clear();
			return;
		}
	}

	KOffsetTable* pOffsetTable = m_bPacked ? (m_pOffsetTableBuf ? (KOffsetTable*)m_pOffsetTableBuf->GetBufferPointer() : 0) : (KOffsetTable*)((BYTE*)pPalPtr + 3 * pHead->wPalEntryCount);

	//create frames
	m_vecFrame.resize(pHead->wFrameCount);

	if( m_bPacked )
	{
		for( DWORD i = 0; i < pHead->wFrameCount; i++ )
		{
			m_vecFrame[i] = CreatePackedSprFrame( m_szFilename, i, pOffsetTable ? pOffsetTable[i].wSize : 0 );
		}
	}
	else
	{
		if( m_pFiledata->GetBufferSize() != (sizeof(KHead) + 3 * pHead->wPalEntryCount + pHead->wFrameCount * sizeof(KOffsetTable) + pOffsetTable[pHead->wFrameCount-1].wOffset + pOffsetTable[pHead->wFrameCount-1].wSize ))
		{
			m_vecFrame.resize(0);
			Clear();
			return;
		}

		for( DWORD i = 0; i < pHead->wFrameCount; i++ )
		{
			if( m_pFiledata->GetBufferSize() < (sizeof(KHead) + 3 * pHead->wPalEntryCount + pHead->wFrameCount * sizeof(KOffsetTable) + pOffsetTable[i].wOffset + pOffsetTable[i].wSize ))
			{
				m_vecFrame[i] = CreateUnpackedSprFrame(0, 0, 0);
				continue;
			}
			struct KFrameInfo
			{
				WORD	wWidth;
				WORD	wHeight;
				WORD	wOffsetX;
				WORD	wOffsetY;
			};
			KFrameInfo* pFrameInfo = (KFrameInfo*)((BYTE*)&pOffsetTable[pHead->wFrameCount] + pOffsetTable[i].wOffset);
			KPOINT cFrameSize = KPOINT((int)pFrameInfo->wWidth, (int)pFrameInfo->wHeight);
			KPOINT cFrameOffset = KPOINT((int)pFrameInfo->wOffsetX, (int)pFrameInfo->wOffsetY);
			LPD3DBUFFER pFrameDataBuf = 0;
			CompressSpr((void*)&pFrameInfo[1], pOffsetTable[i].wSize - sizeof(KFrameInfo), cFrameSize.x, cFrameSize.y, g_sSprConfig.m_dwAlphaLevel, &pFrameDataBuf);
			m_vecFrame[i] = CreateUnpackedSprFrame(pFrameDataBuf, &cFrameOffset, &cFrameSize);
			SAFE_RELEASE(pFrameDataBuf);
		}
	}

	//clear
	SAFE_RELEASE(m_pFiledata);
	SAFE_RELEASE(m_pOffsetTableBuf);
}

void KSprMedia::Clear()
{
	SAFE_RELEASE(m_pFiledata);
	SAFE_RELEASE(m_pOffsetTableBuf);
	if(m_pPalBuf)
		MinusMediaBytes(m_pPalBuf->GetBufferSize());
	SAFE_RELEASE(m_pPalBuf);
	SAFE_RELEASE(m_pExchangePalBuf);
	for(DWORD i = 0; i < m_vecFrame.size(); i++)
	{
		m_vecFrame[i]->RequestDestroyed(TRUE);
	}
	m_vecFrame.clear();
}

KMedia* CreateSpr(LPCSTR pszIdentifier, void* pArg) 
{
	return new KSprMedia(pszIdentifier);
}

BOOL InitSprCreator(void* pArg)
{
	KSprInitParam* pParam = (KSprInitParam*)pArg;
	g_sSprConfig.m_bRgb565 = pParam->bRGB565;
	g_sSprConfig.m_dwAlphaLevel = pParam->dwAlphaLevel;
	return TRUE;
}

void ShutdownSprCreator() 
{

}

void Pal24ToPal16(void* pPal24, void* pPal16, int nColors, BOOL bRGB565)
{
	if( pPal24 && pPal16 && nColors > 0 )
	{
		if( bRGB565 )
		{
			_asm
			{
				mov		ecx, nColors
					mov		esi, pPal24
					mov		edi, pPal16
Start_Convert_565:
				{
					xor		ebx, ebx		//ebx清0
						mov		dx, [esi + 1]	//读如GB
					mov		al, [esi]		//读入R
					mov		bl, dl			//把G移动到[bl]
						shr		eax, 3
						shr		ebx, 2
						shl		eax, 11			//目标r生成了
						shl		ebx, 5
						add		esi, 3
						add		eax, ebx		//目标RG都合成到ax了
						xor		ebx, ebx		//把ebx清0
						mov		bl, dh			//把B移动[bl]
						shr		ebx, 3
						add		eax, ebx		//把目标
						mov		[edi], ax
						add		edi, 2
						dec		ecx				//减少count记数
						jg		Start_Convert_565
				}
			}
		}
		else
		{
			_asm
			{
				mov		ecx, nColors
					mov		esi, pPal24
					mov		edi, pPal16
Start_Convert_555:
				{
					//ax用于保存目标结果，假设第15bit对结果无影响
					xor		ebx, ebx		//ebx清0
						mov		dx, [esi + 1]	//读如GB
					mov		al, [esi]		//读入R
					mov		bl, dl			//把G移动到[bl]
						shr		eax, 3
						shr		ebx, 3
						shl		eax, 10			//目标r生成了
						shl		ebx, 5
						add		esi, 3
						add		eax, ebx		//目标RG都合成到ax了
						xor		ebx, ebx		//把ebx清0
						mov		bl, dh			//把B移动[bl]
						shr		ebx, 3
						add		eax, ebx		//把目标
						mov		[edi], ax
						add		edi, 2
						dec		ecx				//减少count记数
						jg		Start_Convert_555
				}
			}
		}
	}
}

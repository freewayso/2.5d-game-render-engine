/* -------------------------------------------------------------------------
//	文件名		：	spr_frame_media.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	spr帧定义
//
// -----------------------------------------------------------------------*/
#include "spr_frame_media.h"
#include "igpu.h"
#include "gpu_helper.h"
KSprConfig g_sSprConfig;

KSprFrameMedia::KSprFrameMedia(LPD3DBUFFER pPal, LPD3DBUFFER pInfoData)
{
	m_bPacked = FALSE;
	m_pPal = pPal;
	if(pPal) pPal->AddRef();
	m_pInfoData = pInfoData;
	if(pInfoData) pInfoData->AddRef();
	m_pTextureRawBits = 0;	//unprocessed
	m_pTexture = 0;			//unproduced
}

KSprFrameMedia::KSprFrameMedia(LPD3DBUFFER pPal, LPCSTR pszFileName, DWORD dwFrameIndex, DWORD dwInfodataSize)
{
	m_bPacked = TRUE;
	m_pszFileName = pszFileName;	//just save the ptr
	m_dwFrameIndex = dwFrameIndex;
	m_dwInfodataSize = dwInfodataSize;
	m_pPal = pPal;				//partly loaded
	if(pPal) pPal->AddRef();
	m_pInfoData = 0;			
	m_pTextureRawBits = 0;	//unprocessed
	m_pTexture = 0;			//unproduced
}

void KSprFrameMedia::Load()
{
	if( m_bPacked )
		LoadFileFragment(m_pszFileName, m_dwFrameIndex + 2, &m_pInfoData);
}

void KSprFrameMedia::Process()
{
	//check load() step
	if( !m_pPal || !m_pInfoData )
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

	//check size for info
	if( m_pInfoData->GetBufferSize() < sizeof(KFrameInfo) )
	{
		Clear();
		return;
	}

	//check size for packed frame
	if( m_bPacked && m_dwInfodataSize > 0 && (m_pInfoData->GetBufferSize() != m_dwInfodataSize) )
	{
		Clear();
		return;
	}

	//size & offset
	KFrameInfo* pFrameInfo = (KFrameInfo*)m_pInfoData->GetBufferPointer();

	m_cSize.x = (int)pFrameInfo->wWidth;
	m_cSize.y = (int)pFrameInfo->wHeight;
	m_cOffset.x = (int)pFrameInfo->wOffsetX;
	m_cOffset.y = (int)pFrameInfo->wOffsetY;

	//texture size
	AdjustTextureSize(Gpu(), g_sSprConfig.bEnableConditionalNonPow2, g_sSprConfig.eFmt == D3DFMT_DXT3, g_sSprConfig.dwMipmap, &m_cSize, &m_cTextureSize);

	//uv scale
	if( m_cTextureSize.x >= m_cSize.x && m_cTextureSize.y >= m_cSize.y )
		m_sUVScale = D3DXVECTOR2((float)m_cSize.x/m_cTextureSize.x, (float)m_cSize.y/m_cTextureSize.y);
	else
		m_sUVScale = D3DXVECTOR2(1, 1);

	//texture raw bits
	if( g_sSprConfig.eFmt != D3DFMT_A4R4G4B4 )
	{
		D3DXCreateBuffer(m_cSize.x * m_cSize.y * 4, &m_pTextureRawBits);
		DecompressSpr32bit((WORD*)m_pTextureRawBits->GetBufferPointer(), m_cSize.x * 4, (BYTE*)&pFrameInfo[1], m_cSize.x, m_cSize.y, (BYTE*)m_pPal->GetBufferPointer());
	}
	else
	{
		D3DXCreateBuffer(m_cSize.x * m_cSize.y * 2, &m_pTextureRawBits);
		DecompressSpr16bit((WORD*)m_pTextureRawBits->GetBufferPointer(), m_cSize.x * 2, (BYTE*)&pFrameInfo[1], m_cSize.x, m_cSize.y, (BYTE*)m_pPal->GetBufferPointer());
	}

	//free load() step
	if( m_bPacked )
		SAFE_RELEASE(m_pInfoData);
}

void KSprFrameMedia::Produce()
{
	//check process() step
	if( !m_pTextureRawBits )
		return;

	//texture
	if( !Gpu()->CreateTexture(m_cTextureSize.x, m_cTextureSize.y, 1, 0, g_sSprConfig.eFmt, D3DPOOL_MANAGED, &m_pTexture) )
	{
		Clear();
		return;
	}

	PlusMediaBytes( (DWORD)(m_cTextureSize.x * m_cTextureSize.y * FormatBytes(g_sSprConfig.eFmt)) );

	if( m_cTextureSize.x >= m_cSize.x && m_cTextureSize.y >= m_cSize.y )
	{
		if( g_sSprConfig.eFmt == D3DFMT_DXT3 )
		{
			LPD3DSURFACE surf = 0;
			m_pTexture->GetSurfaceLevel(0, &surf);
			RECT rc = {0, 0, m_cSize.x, m_cSize.y};
			HRESULT hr = D3DXLoadSurfaceFromMemory( surf, NULL, &rc, m_pTextureRawBits->GetBufferPointer(), D3DFMT_A8R8G8B8, m_cSize.x * 4, NULL, &rc, D3DX_FILTER_NONE, 0);
			SAFE_RELEASE(surf);

			if( FAILED(hr) )
			{
				Clear();
				return;
			}
		}
		else
		{
			RECT rc = {0, 0, m_cSize.x, m_cSize.y};
			D3DLOCKED_RECT locked_rc;
			m_pTexture->LockRect(0, &locked_rc, &rc, 0);

			if( g_sSprConfig.eFmt == D3DFMT_A8R8G8B8 )
			{
				for(int row = 0; row < m_cSize.y; row++)
				{
					memcpy((BYTE*)locked_rc.pBits + row * locked_rc.Pitch, (BYTE*)m_pTextureRawBits->GetBufferPointer() + row * m_cSize.x * 4, m_cSize.x * 4);
				}
			}
			else	//A4R4G4B4
			{
				for(int row = 0; row < m_cSize.y; row++)
				{
					memcpy((BYTE*)locked_rc.pBits + row * locked_rc.Pitch, (BYTE*)m_pTextureRawBits->GetBufferPointer() + row * m_cSize.x * 2, m_cSize.x * 2);
				}
			}

			m_pTexture->UnlockRect(0);
		}
	}
	else	//need filter
	{
		LPD3DSURFACE surf = 0;
		m_pTexture->GetSurfaceLevel(0, &surf);
		RECT src_rc = {0, 0, m_cSize.x, m_cSize.y};
		RECT dst_rc = {0, 0, m_cTextureSize.x, m_cTextureSize.y};
		HRESULT hr = D3DXLoadSurfaceFromMemory( surf, NULL, &dst_rc, m_pTextureRawBits->GetBufferPointer(), (g_sSprConfig.eFmt == D3DFMT_A4R4G4B4) ? D3DFMT_A4R4G4B4 : D3DFMT_A8R8G8B8, (g_sSprConfig.eFmt == D3DFMT_A4R4G4B4) ? m_cSize.x * 2 : m_cSize.x * 4, NULL, &src_rc, D3DX_FILTER_LINEAR, 0);
		SAFE_RELEASE(surf);
		if( FAILED( hr ) )
		{
			Clear();
			return;
		}
	}

	//free process() step
	SAFE_RELEASE(m_pTextureRawBits);
}

void KSprFrameMedia::Clear()
{
	if( m_bPacked )
		SAFE_RELEASE(m_pInfoData);
	SAFE_RELEASE(m_pTextureRawBits);
	if( m_pTexture )
		MinusMediaBytes( (DWORD)(m_cTextureSize.x * m_cTextureSize.y * FormatBytes(g_sSprConfig.eFmt)) );
	SAFE_RELEASE(m_pTexture);
}

KMedia* CreateUnpackedSprFrame(LPD3DBUFFER pPal, LPD3DBUFFER pInfoData) 
{ 
	return new KSprFrameMedia(pPal, pInfoData); 
}

KMedia* CreatePackedSprFrame(LPD3DBUFFER pPal, LPCSTR pszFileName, DWORD dwFrameIndex, DWORD dwFiledataSize) 
{ 
	return new KSprFrameMedia(pPal, pszFileName, dwFrameIndex, dwFiledataSize); 
}


void DecompressSpr32bit(WORD* pDest, int nPitch, BYTE* pSrc, int width, int height, BYTE* pPalette)
{
	int nLineAdd = nPitch - width * 4;
	__asm
	{
		mov		edi,	pDest
		mov		esi,	pSrc
		mov		edx,	width

loc_DrawSprite_0100:

loc_DrawSprite_0101:

		lodsw
		movzx	ebx,	ah
		movzx	ecx,	al
		sub		edx,	ecx
		or		ebx,	ebx
		jnz		loc_DrawSprite_0102
		xor		eax,	eax
		rep		stosd
		jmp		loc_DrawSprite_0104

loc_DrawSprite_0102:
		push	edx 
		mov		edx,	pPalette

loc_DrawSprite_0103:
		lodsb
		and		eax,	11111111b
		lea		eax,	[eax * 2 + eax]
		mov		bh,		BYTE ptr[edx + eax + 0]
		mov		BYTE ptr[edi + 0], bh
		mov		bh,		BYTE ptr[edx + eax + 1]
		mov		BYTE ptr[edi + 1], bh
		mov		bh,		BYTE ptr[edx + eax + 2]
		mov		BYTE ptr[edi + 2], bh
		mov		BYTE ptr[edi + 3], bl
		add		edi,	4
		loop	loc_DrawSprite_0103
		pop		edx 

loc_DrawSprite_0104:
		cmp		edx,	0
		jg		loc_DrawSprite_0101
		add		edx,	width
		add		edi,	nLineAdd
		dec		height
		jnz		loc_DrawSprite_0100
	}
}

void DecompressSpr16bit(WORD* pDest, int nPitch, BYTE* pSrc, int width, int height, BYTE* pPalette)
{
	int nLineAdd = nPitch - width * 2;
	__asm
	{
		mov		edi,	pDest
		mov		esi,	pSrc
		mov		edx,	width

loc_DrawSprite_0100:

loc_DrawSprite_0101:

		lodsw
		movzx	ebx,	ah
		movzx	ecx,	al
		sub		edx,	ecx
		or		ebx,	ebx
		jnz		loc_DrawSprite_0102
		xor		eax,	eax
		rep		stosw
		jmp		loc_DrawSprite_0104

loc_DrawSprite_0102:
		and		ebx,	11110000b
		shl		ebx,	8
		push	edx
		mov		edx,	pPalette

loc_DrawSprite_0103:
		lodsb
		and		eax,	11111111b
		movzx	eax,	word ptr[edx + eax * 2]
		or		eax,	ebx                   
		stosw
		loop	loc_DrawSprite_0103
		pop		edx                           

loc_DrawSprite_0104:
		cmp		edx,	0
		jg		loc_DrawSprite_0101
		add		edx,	width
		add		edi,	nLineAdd
		dec		height
		jnz		loc_DrawSprite_0100
	}
}

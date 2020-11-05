/* -------------------------------------------------------------------------
//	文件名		：	spr_media_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	spr_media_t定义
//
// -----------------------------------------------------------------------*/
#include "spr_media.h"
#include "../represent_common/ifile.h"
#include "igpu.h"
#include "gpu_helper.h"


KSprMedia::KSprMedia(LPCSTR pszFilename, DWORD dwExchangeColor)
{
	strcpy(m_szFilename, pszFilename);
	m_dwExchangeColor = dwExchangeColor;
	m_bFrameGenerated = false;
	m_pFiledata = 0;
	m_pOffsetTableBuf = 0;
}

void KSprMedia::Load()
{
	IFile* pFile = g_pfnOpenFile(m_szFilename, false, false);
	if(!pFile)
		return;
	if(pFile->IsPackedByFragment())
	{
		m_bPacked = true;
		LoadFileFragment(pFile, 0, &m_pFiledata);
		LoadFileFragment(pFile, 1, &m_pOffsetTableBuf);
	}
	else
	{
		m_bPacked = false;
		LoadFile(pFile, &m_pFiledata);
	}
	pFile->Release();
}

void KSprMedia::Process()
{
	//check load() step
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
		DWORD	dwOffset;
		DWORD	dwSize;
	};

	//check size for head
	if( m_pFiledata->GetBufferSize() < sizeof(KHead) )
	{
		Clear();
		return;
	}

	//load head info
	KHead* pHead = (KHead*)m_pFiledata->GetBufferPointer();

	m_dwDirectionCount			= pHead->wDirectionCount;
	m_dwInterval				= pHead->wInterval;
	m_cSize					= KPOINT(pHead->wWidth, pHead->wHeight);
	m_cCenter					= KPOINT(pHead->wCenterX, pHead->wCenterY);
	m_dwBlendStyle				= pHead->wReserved[1] & 0xff;
	m_dwExchangeColorCount	= pHead->wReserved[4] & 0xff;

	//palette count should be 256
	if( pHead->wPalEntryCount  != 256 )
	{
		Clear();
		return;
	}

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

	//create palette
	BYTE*		pbyPal = (BYTE*)&pHead[1];
	LPD3DBUFFER	pPal = 0;

	if( m_dwExchangeColorCount > 0 )
	{
		BYTE* pbyExPalBegin = pbyPal + 3 * (pHead->wPalEntryCount - m_dwExchangeColorCount);
		BYTE btRed = (BYTE)(m_dwExchangeColor & 0xff);
		BYTE btGreen = (BYTE)((m_dwExchangeColor >> 8) & 0xff);
		BYTE btBlue = (BYTE)((m_dwExchangeColor >> 16) & 0xff);
		BYTE bSourceCy = (btRed * 76 + btGreen * 150 + btBlue * 29) >> 8;
		for( DWORD i = 0; i < m_dwExchangeColorCount; i++, pbyExPalBegin+=3 )
		{
			int nDesCy = pbyExPalBegin[0];

			//  使用hsl颜色空间
			float H, S, L ;    

			L = (float)nDesCy / 255 ;

			int temp_R,temp_G,temp_B;    //合成RGB（中间量，未做亮度补偿）

			float var_R = (float)btRed /255 ;                      //RGB values = 0 ~ 255
			float var_G = (float)btGreen /255;
			float var_B = (float)btBlue /255;

			//float btL = 0.3 * var_R + 0.59 * var_G + 0.11 * var_B ;

			float var_Min = min( var_R, min (var_G, var_B ) );    // RGB最小值
			float var_Max = max( var_R, max (var_G, var_B ) );    // RGB最大值
			float del_Max = var_Max - var_Min  ;         //Delta RGB value

			float btL = ( var_Max + var_Min ) / 2 ;

			// L = (double)nDesCy;  //混合亮度计算

			if ( del_Max == 0 )                     //This is a gray, no chroma...
			{
				H = 0 ;                               //HSL results = 0 ÷ 1
				S = 0 ;
			}
			else                                    //Chromatic data...
			{
				if ( btL < 0.5 ) 
					S = (float)del_Max / ( var_Max + var_Min ) ;
				else           
					S = (float)del_Max / ( 2 - var_Max - var_Min ) ;

				float del_R = ( ( ( var_Max - var_R ) / 6 ) + ( del_Max / 2 ) ) / del_Max ;
				float del_G = ( ( ( var_Max - var_G ) / 6 ) + ( del_Max / 2 ) ) / del_Max ;
				float del_B = ( ( ( var_Max - var_B ) / 6 ) + ( del_Max / 2 ) ) / del_Max ;

				if  ( var_R == var_Max ) 
					H = del_B - del_G ;
				else if ( var_G == var_Max ) 
					H = float(( 1.0 / 3 ) + del_R - del_B) ;
				else if ( var_B == var_Max ) 
					H = float(( 2.0 / 3 ) + del_G - del_R) ;

				if ( H < 0 ) H += 1;
				if ( H > 1 ) H -= 1;

			}

			//反计算
			float var_1,var_2,delta;

			// btL = 0.3 * var_R + 0.59 * var_G + 0.11 * var_B;

			//先进行亮度补偿

			if ( btL > 0.5 )
			{
				delta = ( btL - 0.5f ) * 255;
				L = ( delta + ( 1.0f - delta / 255.0f ) * nDesCy ) / 255;
			}
			else {
				delta = ( 0.5f - btL ) * 255 ;
				L = ( 1 - delta / 255.0f ) * nDesCy  / 255  ;
			}
			// L = btL * 255 -  * 2  * L ;
			if ( S == 0 )                       //HSL values = 0 ÷ 1
			{
				temp_R = (int)(L * 255) ;                     //RGB results = 0 ÷ 255
				temp_G = (int)(L * 255) ;
				temp_B = (int)(L * 255) ;
			}
			else
			{
				if ( L < 0.5 ) 
					var_2 = L * ( 1 + S ) ;
				else           
					var_2 = ( L + S ) - ( S * L )  ;

				var_1 = 2 * L - var_2 ;

				float t1,t2,t3;

				t1=H + ( 1.0f / 3 ) ;
				t2=H;
				t3=H - ( 1.0f / 3 ) ;

				//计算R值

				if ( t1 < 0 ) t1 += 1 ;
				if ( t1 > 1 ) t1 -= 1 ;
				if ( ( 6 * t1 ) < 1 ) temp_R = (int)(255 * ( var_1 + ( var_2 - var_1 ) * 6 * t1 ));
				else if ( ( 2 * t1 ) < 1 ) temp_R = (int)(255 * var_2) ;
				else if ( ( 3 * t1 ) < 2 ) temp_R = (int)(255 * ( var_1 + ( var_2 - var_1 ) * ( ( 2.0 / 3 ) - t1 ) * 6 )) ;
				else temp_R = (int)(255 * var_1);

				//计算G值

				if ( t2 < 0 ) t2 += 1 ;
				if ( t2 > 1 ) t2 -= 1 ;
				if ( ( 6 * t2 ) < 1 ) temp_G = (int)(255 * ( var_1 + ( var_2 - var_1 ) * 6 * t2 ));
				else if ( ( 2 * t2 ) < 1 ) temp_G = (int)(255 * var_2) ;
				else if ( ( 3 * t2 ) < 2 ) temp_G = (int)(255 * ( var_1 + ( var_2 - var_1 ) * ( ( 2.0 / 3 ) - t2 ) * 6 )) ;
				else temp_G = (int)(255 * var_1);

				//计算B值

				if ( t3 < 0 ) t3 += 1 ;
				if ( t3 > 1 ) t3 -= 1 ;
				if ( ( 6 * t3 ) < 1 ) temp_B = (int)(255 * ( var_1 + ( var_2 - var_1 ) * 6 * t3 ));
				else if ( ( 2 * t3 ) < 1 ) temp_B = (int)(255 * var_2) ;
				else if ( ( 3 * t3 ) < 2 ) temp_B = (int)(255 * ( var_1 + ( var_2 - var_1 ) * ( ( 2.0 / 3 ) - t3 ) * 6 )) ;
				else temp_B = (int)(255 * var_1);								   
			} 

			pbyExPalBegin[0]  = (BYTE)(temp_R);
			pbyExPalBegin[1] = (BYTE)(temp_G);
			pbyExPalBegin[2] = (BYTE)(temp_B);
		}
	}

	if( g_sSprConfig.eFmt == D3DFMT_A8R8G8B8 || g_sSprConfig.eFmt == D3DFMT_DXT3 )
	{
		D3DXCreateBuffer(pHead->wPalEntryCount * 3, &pPal);
		BYTE* dst = (BYTE*)pPal->GetBufferPointer();
		for(int i = 0; i < pHead->wPalEntryCount; i++)
		{
			dst[3*i] = pbyPal[3*i+2];
			dst[3*i+1] = pbyPal[3*i+1];
			dst[3*i+2] = pbyPal[3*i];
		}
	}
	else
	{
		D3DXCreateBuffer(pHead->wPalEntryCount * 2, &pPal);
		WORD* dst = (WORD*)pPal->GetBufferPointer();
		for(int i = 0; i < pHead->wPalEntryCount; i++)
		{
			dst[i] = x888To4444(0, pbyPal[3*i], pbyPal[3*i+1], pbyPal[3*i+2]);
		}
	}

	//check for offset_table
	if( m_bPacked )
	{
		if( m_pOffsetTableBuf && (m_pOffsetTableBuf->GetBufferSize() != pHead->wFrameCount * sizeof(KOffsetTable)) )
		{
			pPal->Release();
			Clear();
			return;
		}
	}
	else
	{
		if( m_pFiledata->GetBufferSize() < (sizeof(KHead) + 3 * pHead->wPalEntryCount + pHead->wFrameCount * sizeof(KOffsetTable)) )
		{
			pPal->Release();
			Clear();
			return;
		}
	}

	KOffsetTable* offset_table = m_bPacked ? (m_pOffsetTableBuf ? (KOffsetTable*)m_pOffsetTableBuf->GetBufferPointer() : 0) : (KOffsetTable*)(pbyPal + 3 * pHead->wPalEntryCount);

	//create frames
	if( !m_bFrameGenerated )
	{
		m_vecFrame.resize(pHead->wFrameCount);
		m_vecFrameHandle.resize(pHead->wFrameCount);
	}

	if( m_bPacked )
	{
		PlusMediaBytes( pPal->GetBufferSize() );
		for( DWORD i = 0; i < pHead->wFrameCount; i++ )
		{
			if( !m_bFrameGenerated )
			{
				m_vecFrame[i] = CreatePackedSprFrame( pPal, m_szFilename, i, offset_table ? offset_table[i].dwSize : 0 );
			}
			else
			{
				KSprFrameMedia* pFrame = (KSprFrameMedia*)m_vecFrame[i];
				pFrame->m_pPal = pPal;
				pPal->AddRef();
			}
		}
	}
	else
	{
		if( m_pFiledata->GetBufferSize() != (sizeof(KHead) + 3 * pHead->wPalEntryCount + pHead->wFrameCount * sizeof(KOffsetTable) + offset_table[pHead->wFrameCount-1].dwOffset + offset_table[pHead->wFrameCount-1].dwSize ))
		{
			m_vecFrame.clear();
			m_vecFrameHandle.clear();
			m_bFrameGenerated = true;
			pPal->Release();
			Clear();
			return;
		}

		PlusMediaBytes( pPal->GetBufferSize() );
		for( DWORD i = 0; i < pHead->wFrameCount; i++ )
		{
			if( m_pFiledata->GetBufferSize() < (sizeof(KHead) + 3 * pHead->wPalEntryCount + pHead->wFrameCount * sizeof(KOffsetTable) + offset_table[i].dwOffset + offset_table[i].dwSize ))
			{
				if( !m_bFrameGenerated )
				{
					m_vecFrame[i] = CreateUnpackedSprFrame(pPal, 0);
				}
				continue;
			}
			LPD3DBUFFER pInfoData;
			D3DXCreateBuffer(offset_table[i].dwSize, &pInfoData);
			memcpy(pInfoData->GetBufferPointer(), (BYTE*)&offset_table[pHead->wFrameCount] + offset_table[i].dwOffset, offset_table[i].dwSize);
			PlusMediaBytes(pInfoData->GetBufferSize());

			if( !m_bFrameGenerated )
			{
				m_vecFrame[i] = CreateUnpackedSprFrame(pPal, pInfoData);
			}
			else
			{
				KSprFrameMedia* frame = (KSprFrameMedia*)m_vecFrame[i];
				frame->m_pPal = pPal;
				pPal->AddRef();
				frame->m_pInfoData = pInfoData;
				pInfoData->AddRef();
			}

			DWORD _info_data_size = pInfoData->GetBufferSize();
			if( pInfoData->Release() == 0 )
				MinusMediaBytes( _info_data_size );
		}
	}

	DWORD pal_size = pPal->GetBufferSize();
	if( pPal->Release() == 0 )
		MinusMediaBytes(pal_size);

	//free load() step
	SAFE_RELEASE(m_pFiledata);
	SAFE_RELEASE(m_pOffsetTableBuf);
}

void KSprMedia::PostProcess()
{
	if( m_bFrameGenerated )
		return; 

	for( DWORD i = 0; i < m_vecFrame.size(); i++)
	{
		m_vecFrameHandle[i] = AppendMedia(m_vecFrame[i]);
	}

	m_bFrameGenerated = true;
}

void KSprMedia::Clear()
{
	SAFE_RELEASE(m_pFiledata);
	SAFE_RELEASE(m_pOffsetTableBuf);

	for (std::vector<KMedia*>::iterator it = m_vecFrame.begin(); it != m_vecFrame.end(); ++it)
	{
		KSprFrameMedia* pFrame = (KSprFrameMedia*)*it;
		if(pFrame && pFrame->m_pPal )
		{
			DWORD size = pFrame->m_pPal->GetBufferSize();
			if( pFrame->m_pPal->Release() == 0 )
				MinusMediaBytes( size );
			pFrame->m_pPal = 0;
		}
		if(pFrame && pFrame->m_pInfoData )
		{
			DWORD size = pFrame->m_pInfoData->GetBufferSize();
			if( pFrame->m_pInfoData->Release() == 0 )
				MinusMediaBytes( size );
			pFrame->m_pInfoData = 0;
		}
	}
}

BOOL InitSprCreator(void* pArg)
{
	KSprInitParam* pParam = (KSprInitParam*)pArg;

	g_sSprConfig.bEnableConditionalNonPow2 = pParam->bEnableConditionalNonPow2;
	g_sSprConfig.dwMipmap = pParam->dwMipmap;

	//fmt
	vector<D3DFORMAT> vecFmt;
	if( pParam->bUseCompressedTextureFormat )
	{
		if( pParam->bUse16bitColor )
		{
			vecFmt.push_back( D3DFMT_DXT3 );
			vecFmt.push_back( D3DFMT_A4R4G4B4 );
			vecFmt.push_back( D3DFMT_A8R8G8B8 );
		}
		else
		{
			vecFmt.push_back( D3DFMT_DXT3 );
			vecFmt.push_back( D3DFMT_A8R8G8B8 );
			vecFmt.push_back( D3DFMT_A4R4G4B4 );
		}
	}
	else
	{
		if( pParam->bUse16bitColor )
		{
			vecFmt.push_back( D3DFMT_A4R4G4B4 );
			vecFmt.push_back( D3DFMT_A8R8G8B8 );
			vecFmt.push_back( D3DFMT_DXT3 );
		}
		else
		{
			vecFmt.push_back( D3DFMT_A8R8G8B8 );
			vecFmt.push_back( D3DFMT_A4R4G4B4 );
			vecFmt.push_back( D3DFMT_DXT3 );
		}
	}
	g_sSprConfig.eFmt = FindFirstAvaiableOffscrTextureFormat( Gpu(), &vecFmt[0], (DWORD)vecFmt.size() );

	return g_sSprConfig.eFmt != D3DFMT_UNKNOWN;
}

void ShutdownSprCreator() 
{

}

KMedia* CreateSpr(LPCSTR pszIdentifier, void* arg)
{
	char* pDiv = strstr((char*)pszIdentifier, "|");
	if( pDiv )
	{
		DWORD exchange_color = atoi(pDiv+1);

		char filename[256];
		DWORD i = 0;
		while(pszIdentifier[i] != '|')
		{
			filename[i] = pszIdentifier[i];
			i++;
		}
		filename[i] = 0;
		return new KSprMedia(filename, exchange_color);
	}
	else
		return new KSprMedia(pszIdentifier, 0);
}

WORD x888To4444(BYTE a, BYTE r, BYTE g, BYTE b) 
{
	return (((((WORD)(a))<<8)&0xf000) | ((((WORD)(r))<<4)&0x0f00) | (((WORD)(g))&0x00f0) | (((WORD)(b))>>4));
}


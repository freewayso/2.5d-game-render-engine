/* -------------------------------------------------------------------------
//	文件名		：	_gpu_t.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:11
//	功能描述	：	D3D渲染器类的定义及其相关的全局函数
//
// -----------------------------------------------------------------------*/
#include "gpu.h"

KGpu g_Gpu;

//static caps
DWORD KGpu::BuildValidPresentationParameterCombos(LPD3DBUFFER* ppOut)
{
	//current display mode 取当前Adapter的信息
	D3DDISPLAYMODE sCurDisplayMode;
	m_pD3d->GetAdapterDisplayMode(0, &sCurDisplayMode);

	//build the set of all valid display_modes (ignore refresh-rate)
	vector<D3DDISPLAYMODE>	vecDisplayMode;

	//ensure that the current display mode is in
	vecDisplayMode.push_back(sCurDisplayMode);
	vecDisplayMode[0].RefreshRate = 0;				//ignore refresh-rate

	const D3DFORMAT aeAllowedDisplayFmt[] =
	{
		D3DFMT_X1R5G5B5, 
		D3DFMT_R5G6B5,
		D3DFMT_X8R8G8B8, 
		D3DFMT_A2R10G10B10,
	};
	const DWORD dwAllowedDisplayFmtCount  = sizeof(aeAllowedDisplayFmt) / sizeof(aeAllowedDisplayFmt[0]);

	for( DWORD ifmt = 0; ifmt < dwAllowedDisplayFmtCount; ifmt++ )
	{
		DWORD dwModeCount = m_pD3d->GetAdapterModeCount(0, aeAllowedDisplayFmt[ifmt]); // Adapter个数
		for( DWORD imode = 0; imode < dwModeCount; imode++ )
		{
			D3DDISPLAYMODE sDisplayMode;
			m_pD3d->EnumAdapterModes(0, aeAllowedDisplayFmt[ifmt], imode, &sDisplayMode);

			//ignore refresh-rate
			BOOL exist = FALSE;
			for( DWORD inow = 0; inow < vecDisplayMode.size(); inow++ )
			{
				if(	vecDisplayMode[inow].Format == sDisplayMode.Format &&
					vecDisplayMode[inow].Width == sDisplayMode.Width	 &&
					vecDisplayMode[inow].Height == sDisplayMode.Height )
				{
					exist = TRUE;
					break;
				}
			}
			if( !exist )
			{
				sDisplayMode.RefreshRate = 0;
				vecDisplayMode.push_back(sDisplayMode);
			}
		}
	}

	//build the set of all possible backbuffer formats
	const D3DFORMAT aeBackBufFmt[] = 
	{   
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_R5G6B5,
		D3DFMT_X8R8G8B8,
		D3DFMT_A8R8G8B8,
		D3DFMT_A2R10G10B10,
	};
	const DWORD bkbuf_fmt_count = sizeof(aeBackBufFmt) / sizeof(aeBackBufFmt[0]);

	//build the set of all windowed-fullscreen states
	const BOOL abWindowed[] = {TRUE, FALSE};

	//build the set of all gdi-dialogbox states
	const BOOL abDialogboxSupport[] = {TRUE, FALSE};

	//loop all the combos, filter out invalid ones, and collect the remains
	vector<KGpuPresentationParameters>	vecValidPP;

	KGpuPresentationParameters sPP;

	sPP.dwBackbufferCount = 0;	//backbuffer count is only constrained by video memory (we use discard swap-effect)
	sPP.bVerticalSync = 0;		//presentation interval is ignored

	for( int iwin = 0; iwin < 2; iwin++ )
	{
		sPP.bWindowed = abWindowed[iwin];

		for( int idb = 0; idb < 2; idb++ )
		{
			sPP.bGdiDialogboxSupport = abDialogboxSupport[idb];

			//dialog box is always supported in windowd mode
			if( abWindowed[iwin] && !abDialogboxSupport[idb] )
				continue;

			for( DWORD imode = 0; imode < vecDisplayMode.size(); imode++ )
			{
				sPP.sDisplayMode = vecDisplayMode[imode];

				if( abWindowed[iwin] )
				{
					//in windowed mode, backbuffer size is arbitrary
					sPP.dwBackbufferWidth = 0;
					sPP.dwBackbufferHeight = 0;
				}
				else
				{
					//in fullscreen mode, backbuffer size should be the same as the display resolution
					sPP.dwBackbufferWidth =  vecDisplayMode[imode].Width;
					sPP.dwBackbufferHeight =  vecDisplayMode[imode].Height;
				}

				//windowed = TRUE ==> sDisplayMode = cur_display_mode
				if( abWindowed[iwin] )
				{
					if( vecDisplayMode[imode].Format != sCurDisplayMode.Format ||
						vecDisplayMode[imode].Width != sCurDisplayMode.Width ||
						vecDisplayMode[imode].Height != sCurDisplayMode.Height )
					{
						continue;
					}
				}

				for( DWORD ibk = 0; ibk < bkbuf_fmt_count; ibk++ )
				{
					sPP.eBackbufferFormat = aeBackBufFmt[ibk];

					//in fullscreen mode, if dialog box is supported, bkbuf_fmt should be one of the {D3DFMT_X1R5G5B5, D3DFMT_R5G6B5, D3DFMT_X8R8G8B8}
					if( !abWindowed[iwin] && abDialogboxSupport[idb] )
					{
						if(	aeBackBufFmt[ibk] != D3DFMT_X1R5G5B5 &&
							aeBackBufFmt[ibk] != D3DFMT_R5G6B5 &&
							aeBackBufFmt[ibk] != D3DFMT_X8R8G8B8 )
						{
							continue;
						}
					}

					//the backbuffer should be a render target
					if( FAILED(m_pD3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, vecDisplayMode[imode].Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, aeBackBufFmt[ibk])) ) 
						continue;

					//post pixel shader blending is required
					if( FAILED(m_pD3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, vecDisplayMode[imode].Format, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, aeBackBufFmt[ibk])) ) 
						continue;

					//(windowed, display_fmt, bkbuf_fmt) combo should pass the CheckDeviceType() check
					if( FAILED(m_pD3d->CheckDeviceType(0, D3DDEVTYPE_HAL, vecDisplayMode[imode].Format, aeBackBufFmt[ibk], abWindowed[iwin])) )
						continue;

					//find one, push it
					vecValidPP.push_back(sPP);
				}
			}
		}
	}

	if( vecValidPP.empty() )
		return 0;

	LPD3DBUFFER pBuf;
	if( FAILED(D3DXCreateBuffer((DWORD)vecValidPP.size() * sizeof(KGpuPresentationParameters), &pBuf)) )
		return 0;

	memcpy( pBuf->GetBufferPointer(), &vecValidPP[0], pBuf->GetBufferSize() );
	*ppOut = pBuf;
	return (DWORD)vecValidPP.size();
}

//pipeline controll
BOOL KGpu::TurnOn(const KGpuCreationParameters* pCP, const KGpuPresentationParameters* pPP, KE_GPU_ERROR* pError)
{
	//create device
	D3DDEVICE_CREATION_PARAMETERS	sD3dCP;
	//D3DPRESENT_PARAMETERS			sD3dCP;			//zjq del 09.3.9	改为成员变量

	TranslateInitializationParameters(&sD3dCP, &m_d3dPresentParameters, pCP, pPP);

	HRESULT hr = m_pD3d->CreateDevice(sD3dCP.AdapterOrdinal, sD3dCP.DeviceType, sD3dCP.hFocusWindow, sD3dCP.BehaviorFlags, &m_d3dPresentParameters, &m_pDevice);
	if( FAILED(hr) )
	{
		switch( hr )
		{
		case D3DERR_INVALIDCALL:		*pError = emGPU_ERROR_INVALID_PARAMS; break;
		case D3DERR_OUTOFVIDEOMEMORY:	*pError = emGPU_ERROR_OUT_OF_VIDEO_MEMORY; break;
		case D3DERR_NOTAVAILABLE:		*pError = emGPU_ERROR_HARDWARE_ACCELERATION_NOT_AVAILABLE; break;
		default:						*pError = emGPU_ERROR_UNKNOWN; break;
		}

		return FALSE;
	}

	//apply dialog box mode
	if( !pPP->bWindowed )
	{
		m_pDevice->SetDialogBoxMode( pPP->bGdiDialogboxSupport );
	}

	//save creation/presentation parameters;
	m_sCurCP = *pCP;
	m_sCurPP = *pPP;

	//dynamic vertex buffer
	m_pvbDynVb = 0;
	m_dwDynVbSize = 0;
	m_dwDynVbUsedBytes = 0;

	//define default static states and set it as the current one
	KStaticRenderStates sDefaultSrs;

	sDefaultSrs.dwLighting				= TRUE;
	sDefaultSrs.dwCullMode				= D3DCULL_CCW;
	sDefaultSrs.dwAlphaTestEnable		= FALSE;
	sDefaultSrs.dwAlphaRef				= 0;
	sDefaultSrs.dwAlphaFunc				= D3DCMP_ALWAYS;
	sDefaultSrs.dwZEnable				= D3DZB_TRUE;
	sDefaultSrs.dwZFunc					= D3DCMP_LESSEQUAL;
	sDefaultSrs.dwZWriteEnable			= TRUE;
	sDefaultSrs.dwAlphaBlendEnable		= FALSE;
	sDefaultSrs.dwBlendOp				= D3DBLENDOP_ADD;
	sDefaultSrs.dwSrcBlend				= D3DBLEND_ONE;
	sDefaultSrs.dwDestBlend				= D3DBLEND_ZERO;
	sDefaultSrs.dwColorOp				= D3DTOP_MODULATE;
	sDefaultSrs.dwColorArg1				= D3DTA_TEXTURE;
	sDefaultSrs.dwColorArg2				= D3DTA_CURRENT;
	sDefaultSrs.dwAlphaOp				= D3DTOP_SELECTARG1;
	sDefaultSrs.dwAlphaArg1				= D3DTA_TEXTURE;
	sDefaultSrs.dwAlphaArg2				= D3DTA_CURRENT;
	sDefaultSrs.dwAddressU				= D3DTADDRESS_WRAP;
	sDefaultSrs.dwAddressV				= D3DTADDRESS_WRAP;
	sDefaultSrs.dwMagFilter				= D3DTEXF_POINT;
	sDefaultSrs.dwMinFilter				= D3DTEXF_POINT;

	DefineStaticRenderStates(&sDefaultSrs);		//the returned id will be 0
	m_dwCurSrs = 0;

	//dynamic states
	ForceDefaultDynamicRenderStates(m_pDevice, &m_sCurDrs);

	return TRUE;
}

VOID KGpu::TurnOff()
{
	//release dynamic vertex buffer
	SAFE_RELEASE(m_pvbDynVb);
	m_dwDynVbSize = 0;
	m_dwDynVbUsedBytes = 0;

	//clear static states
	m_vecSrs.clear();
	m_vecTransitionTable.clear();
	m_dwCurSrs = -1;

	//clear dynamic states
	memset(&m_sCurDrs, 0, sizeof(m_sCurDrs));

	//creation/presentation is undefined now
	memset(&m_sCurCP, 0, sizeof(m_sCurCP));
	memset(&m_sCurPP, 0, sizeof(m_sCurPP));

	//now all the device objects are destroyed, we can release the device
	SAFE_RELEASE(m_pDevice);
}

BOOL KGpu::Reset(const KGpuPresentationParameters* pPP, KE_GPU_ERROR* pError)
{
	//release dynamic vertex buffer (it will be recreated lazily after resetting successfully)
	SAFE_RELEASE(m_pvbDynVb);
	m_dwDynVbUsedBytes = 0;

	//now all the video objects are discarded, we can reset the device
	//D3DPRESENT_PARAMETERS m_d3dPresentParameters;				//zjq del 09.3.9	改为成员变量
	TranslateInitializationParameters(0, &m_d3dPresentParameters, &m_sCurCP, pPP);

	HRESULT hr = m_pDevice->Reset(&m_d3dPresentParameters);
	if( FAILED(hr) )
	{
		switch(hr)
		{
		case D3DERR_INVALIDCALL:			*pError = emGPU_ERROR_INVALID_PARAMS; break;
		case D3DERR_OUTOFVIDEOMEMORY:		*pError = emGPU_ERROR_OUT_OF_VIDEO_MEMORY; break;
		case D3DERR_DEVICELOST:				*pError = emGPU_ERROR_DEVICE_LOST; break;
		case D3DERR_DRIVERINTERNALERROR:	*pError = emGPU_ERROR_DRIVER_INTERNAL_ERROR; break;
		default:							*pError = emGPU_ERROR_UNKNOWN; break;
		}
		return FALSE;
	}

	//apply dialog box mode
	if( !pPP->bWindowed )
	{
		m_pDevice->SetDialogBoxMode( pPP->bGdiDialogboxSupport );
	}

	//update cur_pp after successfully resetting
	m_sCurPP = *pPP;

	//force to default static states
	m_pDevice->SetRenderState(D3DRS_LIGHTING,				TRUE);
	m_pDevice->SetRenderState(D3DRS_CULLMODE,				D3DCULL_CCW);
	m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE,		FALSE);
	m_pDevice->SetRenderState(D3DRS_ALPHAREF,				0);
	m_pDevice->SetRenderState(D3DRS_ALPHAFUNC,				D3DCMP_ALWAYS);
	m_pDevice->SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE);
	m_pDevice->SetRenderState(D3DRS_ZFUNC,					D3DCMP_LESSEQUAL);
	m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE,			TRUE);
	m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,		FALSE);
	m_pDevice->SetRenderState(D3DRS_BLENDOP,				D3DBLENDOP_ADD);
	m_pDevice->SetRenderState(D3DRS_SRCBLEND,				D3DBLEND_ONE);
	m_pDevice->SetRenderState(D3DRS_DESTBLEND,				D3DBLEND_ZERO);

	m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP,		D3DTOP_MODULATE);
	m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1,	D3DTA_TEXTURE);
	m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2,	D3DTA_CURRENT);
	m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,		D3DTOP_SELECTARG1);
	m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE);
	m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,	D3DTA_CURRENT);

	m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU,		D3DTADDRESS_WRAP);
	m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV,		D3DTADDRESS_WRAP);
	m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER,		D3DTEXF_POINT);
	m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER,		D3DTEXF_POINT);

	m_dwCurSrs = 0;

	//force to default dynamic states
	ForceDefaultDynamicRenderStates(m_pDevice, &m_sCurDrs);

	return TRUE;
}

BOOL KGpu::Present(KE_GPU_ERROR* pError) 
{
	HRESULT hr = m_pDevice->Present(0, 0, 0, 0);
	if( FAILED(hr) )
	{
		switch( hr )
		{
		case D3DERR_DEVICELOST:				*pError = emGPU_ERROR_DEVICE_LOST; break;
		case D3DERR_DRIVERINTERNALERROR:	*pError = emGPU_ERROR_DRIVER_INTERNAL_ERROR; break;
		default:							*pError = emGPU_ERROR_UNKNOWN; break;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL KGpu::TestCooperativeLevel(KE_GPU_ERROR* pError) 
{
	HRESULT hr = m_pDevice->TestCooperativeLevel();
	if( FAILED(hr) )
	{
		switch( hr )
		{
		case D3DERR_DEVICELOST:				*pError = emGPU_ERROR_DEVICE_LOST; break;
		case D3DERR_DEVICENOTRESET:			*pError = emGPU_ERROR_DEVICE_NOT_RESET; break;
		case D3DERR_DRIVERINTERNALERROR:	*pError = emGPU_ERROR_DRIVER_INTERNAL_ERROR; break;
		default:							*pError = emGPU_ERROR_UNKNOWN; break;
		}
		return FALSE;
	}
	return TRUE;
}

//dynamic caps
DWORD KGpu::BuildValidRenderTargetTextureFormats(LPD3DBUFFER* ppOut)
{
	const D3DFORMAT aeFmt[] = 
	{   
		D3DFMT_R8G8B8,
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_R5G6B5,
		D3DFMT_X1R5G5B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_A4R4G4B4,
		D3DFMT_R3G3B2,
		D3DFMT_A8R3G3B2,
		D3DFMT_X4R4G4B4,
		D3DFMT_A8B8G8R8,
		D3DFMT_X8B8G8R8,
	};
	const DWORD dwFmtCount = sizeof(aeFmt) / sizeof(aeFmt[0]);

	vector<D3DFORMAT>	vecRtTexFmt;
	for( DWORD ifmt = 0; ifmt < dwFmtCount; ifmt++ )
	{
		//it should be a render target
		if( FAILED(m_pD3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, m_sCurPP.sDisplayMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, aeFmt[ifmt])) ) 
			continue;

		//find one, add it
		vecRtTexFmt.push_back(aeFmt[ifmt]);
	}

	if(vecRtTexFmt.empty())
		return 0;

	LPD3DBUFFER pBuf;
	if( FAILED(D3DXCreateBuffer((DWORD)vecRtTexFmt.size() * sizeof(D3DFORMAT), &pBuf)) )
		return 0;

	memcpy(pBuf->GetBufferPointer(), &vecRtTexFmt[0], pBuf->GetBufferSize());
	*ppOut = pBuf;
	return (DWORD)vecRtTexFmt.size();
}

DWORD KGpu::BuildValidOffscrTextureFormats(LPD3DBUFFER* ppOut)
{
	const D3DFORMAT aeFmt[] = 
	{   
		D3DFMT_R8G8B8				,
		D3DFMT_A8R8G8B8             ,
		D3DFMT_X8R8G8B8             ,
		D3DFMT_R5G6B5               ,
		D3DFMT_X1R5G5B5             ,
		D3DFMT_A1R5G5B5             ,
		D3DFMT_A4R4G4B4             ,
		D3DFMT_R3G3B2               ,
		D3DFMT_A8                   ,
		D3DFMT_A8R3G3B2             ,
		D3DFMT_X4R4G4B4             ,
		D3DFMT_A8B8G8R8             ,
		D3DFMT_X8B8G8R8             ,
		D3DFMT_DXT1					,
		D3DFMT_DXT2					,
		D3DFMT_DXT3					,
		D3DFMT_DXT4					,
		D3DFMT_DXT5					,
	};
	const DWORD dwFmtCount = sizeof(aeFmt) / sizeof(aeFmt[0]);

	vector<D3DFORMAT>	vecOffscrTexFmt;
	for( DWORD ifmt = 0; ifmt < dwFmtCount; ifmt++ )
	{
		//it should be a texture
		if(FAILED(m_pD3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, m_sCurPP.sDisplayMode.Format, 0, D3DRTYPE_TEXTURE, aeFmt[ifmt]))) 
			continue;

		//find one, add it
		vecOffscrTexFmt.push_back(aeFmt[ifmt]);
	}

	if(vecOffscrTexFmt.empty())
		return 0;

	LPD3DBUFFER pBuf;
	if( FAILED(D3DXCreateBuffer((DWORD)vecOffscrTexFmt.size() * sizeof(D3DFORMAT), &pBuf)) )
		return 0;

	memcpy(pBuf->GetBufferPointer(), &vecOffscrTexFmt[0], pBuf->GetBufferSize());
	*ppOut = pBuf;
	return (DWORD)vecOffscrTexFmt.size();
}

//dynamic vertex buffer
VOID KGpu::SetDynamicVertexBufferSize(DWORD dwSize)
{
	SAFE_RELEASE(m_pvbDynVb);
	m_dwDynVbSize = dwSize;
	m_dwDynVbUsedBytes = 0;
}

VOID* KGpu::LockDynamicVertexBuffer(DWORD dwVertexCount, DWORD dwStride, DWORD* pdwStartVertex, LPD3DVB* pvbVertexBuffer)
{
	//create dynvb lazily
	if( !m_pvbDynVb && m_dwDynVbSize > 0 )
		m_pDevice->CreateVertexBuffer(m_dwDynVbSize, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_pvbDynVb, 0);

	//check dynvb
	if( !m_pvbDynVb )
		return 0;

	//align to a mutliple of 'stride'
	m_dwDynVbUsedBytes += ((dwStride - m_dwDynVbUsedBytes % dwStride) % dwStride);

	DWORD nLockSize = dwStride * dwVertexCount;

	if( m_dwDynVbUsedBytes + nLockSize <= m_dwDynVbSize )
	{
		//lock with no-overwrite
		VOID* v;
		if( FAILED(m_pvbDynVb->Lock(m_dwDynVbUsedBytes, nLockSize, &v, D3DLOCK_NOOVERWRITE)) )
			return 0;

		*pdwStartVertex = m_dwDynVbUsedBytes / dwStride;
		m_dwDynVbUsedBytes += nLockSize;
		*pvbVertexBuffer = m_pvbDynVb;
		return v;
	}
	else
	{
		//fail if the buffer is not large enough
		if( m_dwDynVbSize < nLockSize )
			return 0;

		//lock with discard
		VOID* v;
		if( FAILED(m_pvbDynVb->Lock(0, 0, &v, D3DLOCK_DISCARD)) )
			return 0;

		*pdwStartVertex = 0;
		m_dwDynVbUsedBytes = nLockSize;
		*pvbVertexBuffer = m_pvbDynVb;
		return v;
	}
}

VOID KGpu::UnlockDynamicVertexBuffer()
{
	m_pvbDynVb->Unlock();
}

//static render states
DWORD KGpu::DefineStaticRenderStates( const KStaticRenderStates* pIn )
{
	DWORD dwOldSize = (DWORD)m_vecSrs.size();
	m_vecSrs.push_back( *pIn );
	m_vecTransitionTable.resize( dwOldSize + 1 );
	for( DWORD i = 0; i < dwOldSize + 1; i++ )
	{
		m_vecTransitionTable[i].resize( dwOldSize + 1 );
	}
	for( DWORD i = 0; i < dwOldSize + 1; i++ )
	{
		InitTransitionInfo( &m_vecTransitionTable[i][dwOldSize], &m_vecSrs[i], &m_vecSrs[dwOldSize] );
	}
	for( DWORD i = 0; i < dwOldSize; i++ )
	{
		InitTransitionInfo( &m_vecTransitionTable[dwOldSize][i], &m_vecSrs[dwOldSize], &m_vecSrs[i] );
	}
	return dwOldSize;
}

VOID KGpu::SetStaticRenderStates( DWORD dwId )
{
	ApplyTransitionInfo( m_pDevice, &m_vecTransitionTable[m_dwCurSrs][dwId] );
	m_dwCurSrs = dwId;
}

//dynamic render states
VOID KGpu::SetStreamSource( LPD3DVB pvbVertexBuffer, DWORD dwStride )
{
	if( m_sCurDrs.vbVertexBuffer != pvbVertexBuffer || m_sCurDrs.dwStride != dwStride )
	{
		m_pDevice->SetStreamSource( 0, pvbVertexBuffer, 0, dwStride );
		m_sCurDrs.vbVertexBuffer = pvbVertexBuffer;
		m_sCurDrs.dwStride = dwStride;
	}
}

VOID KGpu::SetIndices( LPD3DIB nIndexBuffer )
{
	if( m_sCurDrs.ibIndexBuffer != nIndexBuffer )
	{
		m_pDevice->SetIndices( nIndexBuffer );
		m_sCurDrs.ibIndexBuffer = nIndexBuffer;
	}
}

VOID KGpu::SetFvf( DWORD dwFvf )
{
	if( m_sCurDrs.dwfvf != dwFvf )
	{
		m_pDevice->SetFVF( dwFvf );
		m_sCurDrs.dwfvf = dwFvf;
	}
}

VOID KGpu::SetTexture( LPD3DTEXTURE pTexture )
{
	if( m_sCurDrs.pTexture != pTexture )
	{
		m_pDevice->SetTexture( 0, pTexture );
		m_sCurDrs.pTexture = pTexture;
	}
}

BOOL InitGpu(KE_GPU_ERROR* pError)
{
	LPD3D pD3d = Direct3DCreate9( D3D_SDK_VERSION );
	if( !pD3d )
	{
		*pError = emGPU_ERROR_INIT_D3D_FAILED;
		SAFE_RELEASE(pD3d);
		return FALSE;
	}

	D3DCAPS9 eD3dcaps;
	if( 0 == pD3d->GetAdapterCount() ||  FAILED(pD3d->GetDeviceCaps(0, D3DDEVTYPE_HAL, &eD3dcaps)) )
	{
		*pError = emGPU_ERROR_HARDWARE_ACCELERATION_NOT_AVAILABLE;
		SAFE_RELEASE(pD3d);
		return FALSE;
	}

	//pD3d
	g_Gpu.m_pD3d = pD3d;

	//hardware caps
	TranslateCaps(&g_Gpu.m_sCaps, &eD3dcaps);

	//device
	g_Gpu.m_pDevice = 0;

	//cur_cp, cup_pp is undefined now
	memset(&g_Gpu.m_sCurCP, 0, sizeof(g_Gpu.m_sCurCP));
	memset(&g_Gpu.m_sCurPP, 0, sizeof(g_Gpu.m_sCurPP));

	//no dynamic vertex buffer
	g_Gpu.m_pvbDynVb = 0;
	g_Gpu.m_dwDynVbSize = 0;
	g_Gpu.m_dwDynVbUsedBytes = 0;

	//static states is undefined now
	g_Gpu.m_dwCurSrs = -1;

	//dynamic states is undefined now
	memset(&g_Gpu.m_sCurDrs, 0, sizeof(g_Gpu.m_sCurDrs));

	return TRUE;
}

VOID ShutdownGpu() { SAFE_RELEASE( g_Gpu.m_pD3d ); }

VOID TranslateCaps(KGpuCaps* pGpuCaps, const D3DCAPS9* pD3dCaps)
{
	pGpuCaps->bCanClipTnlVerts			= (pD3dCaps->PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS) != 0;
	pGpuCaps->dwMaxTextureAspectRatio	= (pD3dCaps->TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) ? 1 : pD3dCaps->MaxTextureAspectRatio;
	pGpuCaps->dwMaxTextureWidth			= pD3dCaps->MaxTextureWidth;
	pGpuCaps->dwMaxTextureHeight			= pD3dCaps->MaxTextureHeight;
	pGpuCaps->eTexturePow2Restriction	= (pD3dCaps->TextureCaps & D3DPTEXTURECAPS_POW2) ? ((pD3dCaps->TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) ? emGPU_TEXTURE_POW2_RESTRICTION_CONDITIONAL : emGPU_TEXTURE_POW2_RESTRICTION_ALL) : emGPU_TEXTURE_POW2_RESTRICTION_NONE;
	pGpuCaps->bScissorTestAvaiable		= (pD3dCaps->RasterCaps & D3DPRASTERCAPS_SCISSORTEST ) != 0;
	pGpuCaps->eMaxVertexProcessingLevel = (pD3dCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ? ((pD3dCaps->DevCaps & D3DDEVCAPS_PUREDEVICE) ? emKE_GPU_VERTEX_PROCESSING_METHOD_PUREHARDWARE : emKE_GPU_VERTEX_PROCESSING_METHOD_HARDWARE) : emGPU_VERTEX_PROCESSING_METHOD_SOFEWARE;
}

VOID TranslateInitializationParameters(D3DDEVICE_CREATION_PARAMETERS* pD3dCP, D3DPRESENT_PARAMETERS* pD3dPP, const KGpuCreationParameters* pCP, const KGpuPresentationParameters* pPP)
{
	if( pD3dCP )
	{
		pD3dCP->AdapterOrdinal	= 0;
		pD3dCP->DeviceType		= D3DDEVTYPE_HAL;
		pD3dCP->hFocusWindow		= pCP->hWnd;
		pD3dCP->BehaviorFlags	= D3DCREATE_FPU_PRESERVE | D3DCREATE_DISABLE_DRIVER_MANAGEMENT;
		if( pCP->bMultithread ) 
			pD3dCP->BehaviorFlags |= D3DCREATE_MULTITHREADED;
		switch( pCP->eVertexProcessingMethod )
		{
		case emGPU_VERTEX_PROCESSING_METHOD_SOFEWARE:		pD3dCP->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING; break;
		case emKE_GPU_VERTEX_PROCESSING_METHOD_HARDWARE:		pD3dCP->BehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING; break;
		case emKE_GPU_VERTEX_PROCESSING_METHOD_PUREHARDWARE:	pD3dCP->BehaviorFlags |= (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE); break;
		}
	}

	if( pD3dPP )
	{
		pD3dPP->BackBufferWidth				= pPP->bWindowed ? pPP->dwBackbufferWidth : pPP->sDisplayMode.Width;
		pD3dPP->BackBufferHeight				= pPP->bWindowed ? pPP->dwBackbufferHeight : pPP->sDisplayMode.Height;
		pD3dPP->BackBufferFormat				= pPP->eBackbufferFormat;
		pD3dPP->BackBufferCount				= pPP->dwBackbufferCount;
		pD3dPP->MultiSampleType				= D3DMULTISAMPLE_NONE;
		pD3dPP->MultiSampleQuality			= 0;
		pD3dPP->SwapEffect					= D3DSWAPEFFECT_DISCARD;	//this swap effect will always be the most efficient in terms of memory consumption and performance
		pD3dPP->hDeviceWindow				= pCP ? pCP->hWnd : 0;
		pD3dPP->Windowed						= pPP->bWindowed;
		pD3dPP->EnableAutoDepthStencil		= FALSE;
		pD3dPP->AutoDepthStencilFormat		= D3DFMT_UNKNOWN;
		pD3dPP->Flags						= 0;
		if( !pPP->bWindowed && pPP->bGdiDialogboxSupport )
			pD3dPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;	// to show GDI dialog-box in fullscreen mode, the backbuffer should be lockable
		pD3dPP->FullScreen_RefreshRateInHz	= 0;
		pD3dPP->PresentationInterval			= pPP->bVerticalSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	}
}

VOID InitTransitionInfo(KTransitionInfo* pInfo, const KStaticRenderStates* pSrc, const KStaticRenderStates* pDst)
{
	pInfo->vecrRenderState.clear();
	pInfo->vecTextureStageState.clear();
	pInfo->vecSamplerState.clear();

	if(pSrc->dwLighting			!= pDst->dwLighting)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_LIGHTING,			pDst->dwLighting));
	if(pSrc->dwCullMode			!= pDst->dwCullMode)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_CULLMODE,			pDst->dwCullMode));
	if(pSrc->dwAlphaTestEnable		!= pDst->dwAlphaTestEnable)	pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ALPHATESTENABLE,		pDst->dwAlphaTestEnable));
	if(pSrc->dwAlphaRef			!= pDst->dwAlphaRef)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ALPHAREF,			pDst->dwAlphaRef));
	if(pSrc->dwAlphaFunc			!= pDst->dwAlphaFunc)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ALPHAFUNC,			pDst->dwAlphaFunc));
	if(pSrc->dwZEnable				!= pDst->dwZEnable)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ZENABLE,				pDst->dwZEnable));
	if(pSrc->dwZFunc				!= pDst->dwZFunc)				pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ZFUNC,				pDst->dwZFunc));
	if(pSrc->dwZWriteEnable		!= pDst->dwZWriteEnable)		pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ZWRITEENABLE,		pDst->dwZWriteEnable));
	if(pSrc->dwAlphaBlendEnable	!= pDst->dwAlphaBlendEnable)	pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_ALPHABLENDENABLE,	pDst->dwAlphaBlendEnable));
	if(pSrc->dwBlendOp				!= pDst->dwBlendOp)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_BLENDOP,				pDst->dwBlendOp));
	if(pSrc->dwSrcBlend			!= pDst->dwSrcBlend)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_SRCBLEND,			pDst->dwSrcBlend));
	if(pSrc->dwDestBlend			!= pDst->dwDestBlend)			pInfo->vecrRenderState.push_back( pair<DWORD, DWORD>(D3DRS_DESTBLEND,			pDst->dwDestBlend));

	if(pSrc->dwColorOp				!= pDst->dwColorOp)			pInfo->vecTextureStageState.push_back( pair<DWORD, DWORD>(D3DTSS_COLOROP,		pDst->dwColorOp));
	if(pSrc->dwColorArg1			!= pDst->dwColorArg1)			pInfo->vecTextureStageState.push_back( pair<DWORD, DWORD>(D3DTSS_COLORARG1,	pDst->dwColorArg1));
	if(pSrc->dwColorArg2			!= pDst->dwColorArg2)			pInfo->vecTextureStageState.push_back( pair<DWORD, DWORD>(D3DTSS_COLORARG2,	pDst->dwColorArg2));
	if(pSrc->dwAlphaOp				!= pDst->dwAlphaOp)			pInfo->vecTextureStageState.push_back( pair<DWORD, DWORD>(D3DTSS_ALPHAOP,		pDst->dwAlphaOp));
	if(pSrc->dwAlphaArg1			!= pDst->dwAlphaArg1)			pInfo->vecTextureStageState.push_back( pair<DWORD, DWORD>(D3DTSS_ALPHAARG1,	pDst->dwAlphaArg1));
	if(pSrc->dwAlphaArg2			!= pDst->dwAlphaArg2)			pInfo->vecTextureStageState.push_back( pair<DWORD, DWORD>(D3DTSS_ALPHAARG2,	pDst->dwAlphaArg2));

	if(pSrc->dwAddressU			!= pDst->dwAddressU)			pInfo->vecSamplerState.push_back( pair<DWORD, DWORD>(D3DSAMP_ADDRESSU,			pDst->dwAddressU));
	if(pSrc->dwAddressV			!= pDst->dwAddressV)			pInfo->vecSamplerState.push_back( pair<DWORD, DWORD>(D3DSAMP_ADDRESSV,			pDst->dwAddressV));
	if(pSrc->dwMagFilter			!= pDst->dwMagFilter)			pInfo->vecSamplerState.push_back( pair<DWORD, DWORD>(D3DSAMP_MAGFILTER,		pDst->dwMagFilter));
	if(pSrc->dwMinFilter			!= pDst->dwMinFilter)			pInfo->vecSamplerState.push_back( pair<DWORD, DWORD>(D3DSAMP_MINFILTER,		pDst->dwMinFilter));
}

VOID ApplyTransitionInfo(LPD3DDEVICE pDevice, KTransitionInfo* pInfo)
{
	for( DWORD i = 0; i < pInfo->vecrRenderState.size(); i++ )
	{
		pDevice->SetRenderState((D3DRENDERSTATETYPE)pInfo->vecrRenderState[i].first, pInfo->vecrRenderState[i].second);
	}
	for( DWORD i = 0; i < pInfo->vecTextureStageState.size(); i++ )
	{
		pDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)pInfo->vecTextureStageState[i].first, pInfo->vecTextureStageState[i].second);
	}
	for( DWORD i = 0; i < pInfo->vecSamplerState.size(); i++ )
	{
		pDevice->SetSamplerState(0, (D3DSAMPLERSTATETYPE)pInfo->vecSamplerState[i].first, pInfo->vecSamplerState[i].second);
	}
}

VOID ForceDefaultDynamicRenderStates(LPD3DDEVICE pDevice, KDynamicRenderStates* pDrs)
{
	pDevice->SetStreamSource(0, 0, 0, 0);
	pDrs->vbVertexBuffer = 0;
	pDrs->dwStride = 0;

	pDevice->SetIndices(0);
	pDrs->ibIndexBuffer = 0;

	pDevice->SetFVF(0);
	pDrs->dwfvf = 0;

	pDevice->SetTexture(0, 0);
	pDrs->pTexture = 0;
}
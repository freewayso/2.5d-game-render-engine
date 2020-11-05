/* -------------------------------------------------------------------------
//	文件名		：	gpu_helper.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-19 11:11:11
//	功能描述	：	渲染器工具函数 
//
// -----------------------------------------------------------------------*/
#include "gpu_helper.h"

BOOL				g_bDisableFullscreenIme;
BOOL				g_bInputWindowed;
DWORD				g_dwInputWidth;
DWORD				g_dwInputHeight;
vector<D3DFORMAT>	g_vecDisplayFmtPriority; 
vector<D3DFORMAT>	g_vecBkbufFmtPriority;

//gpu helper functions
float FormatBytes( D3DFORMAT eFmt )
{
	switch( eFmt )
	{
	case D3DFMT_R8G8B8:				return 3;
	case D3DFMT_A8R8G8B8:			return 4;
	case D3DFMT_X8R8G8B8:			return 4;
	case D3DFMT_R5G6B5:				return 2;
	case D3DFMT_X1R5G5B5:			return 2;
	case D3DFMT_A1R5G5B5:			return 2;
	case D3DFMT_A4R4G4B4:			return 2;
	case D3DFMT_R3G3B2:				return 1;
	case D3DFMT_A8:					return 1;
	case D3DFMT_A8R3G3B2:			return 2;
	case D3DFMT_X4R4G4B4:			return 2;
	case D3DFMT_A8B8G8R8:			return 4;
	case D3DFMT_X8B8G8R8:			return 4;
	case D3DFMT_DXT1:				return 0.5;
	case D3DFMT_DXT2:				return 1;
	case D3DFMT_DXT3:				return 1;
	case D3DFMT_DXT4:				return 1;
	case D3DFMT_DXT5:				return 1;
	default:						return 0;
	}
}

D3DFORMAT FindFirstAvaiableOffscrTextureFormat( IKGpu* gpu, const D3DFORMAT* fmts, DWORD count )
{
	LPD3DBUFFER all_fmt_buf;
	DWORD all_fmt_count = gpu->BuildValidOffscrTextureFormats( &all_fmt_buf );
	if( all_fmt_count > 0 )
	{
		D3DFORMAT* all_fmt_vec = (D3DFORMAT*)all_fmt_buf->GetBufferPointer();
		for( DWORD i = 0; i < count; i++ )
		{
			if( find(&all_fmt_vec[0], &all_fmt_vec[all_fmt_count], fmts[i]) !=  &all_fmt_vec[all_fmt_count] )
			{
				return fmts[i];
			}
		}
	}

	return D3DFMT_UNKNOWN;
}

D3DFORMAT FindFirstAvaiableRenderTargetTextureFormat( IKGpu* gpu, const D3DFORMAT* fmts, DWORD count )
{
	LPD3DBUFFER all_fmt_buf;
	DWORD all_fmt_count = gpu->BuildValidRenderTargetTextureFormats( &all_fmt_buf );
	if( all_fmt_count > 0 )
	{
		D3DFORMAT* all_fmt_vec = (D3DFORMAT*)all_fmt_buf->GetBufferPointer();
		for( DWORD i = 0; i < count; i++ )
		{
			if( find(&all_fmt_vec[0], &all_fmt_vec[all_fmt_count], fmts[i]) !=  &all_fmt_vec[all_fmt_count] )
			{
				return fmts[i];
			}
		}
	}

	return D3DFMT_UNKNOWN;
}

void AdjustTextureSize(IKGpu* pGpu, BOOL bEnableConditionalNonPow2, BOOL bDxtn, DWORD dwMipmapLevel, const KPOINT* in, KPOINT* out)
{
	DWORD width = (DWORD)in->x;
	DWORD height = (DWORD)in->y;

	DWORD _w;
	DWORD _h;

	BOOL pow2 = (pGpu->GetHardwareCaps()->eTexturePow2Restriction == emGPU_TEXTURE_POW2_RESTRICTION_ALL) || (pGpu->GetHardwareCaps()->eTexturePow2Restriction == emGPU_TEXTURE_POW2_RESTRICTION_CONDITIONAL && !bEnableConditionalNonPow2);

	if(	pow2 )
	{
		//pow2
		_w = 1;
		while(_w < width) _w <<= 1;

		_h = 1;
		while(_h < height) _h <<= 1;

		//aspect ratio
		if(pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio != 0)
		{
			while(_w/_h > pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio) _h <<= 1;
			while(_h/_w > pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio) _w <<= 1;
		}

		if( _w > pGpu->GetHardwareCaps()->dwMaxTextureWidth || _h > pGpu->GetHardwareCaps()->dwMaxTextureHeight )
		{
			//max size
			DWORD max_w = min( _w, pGpu->GetHardwareCaps()->dwMaxTextureWidth );
			DWORD max_h = min( _h, pGpu->GetHardwareCaps()->dwMaxTextureHeight );

			//pow2
			_w = 1;
			while((_w << 1) <= max_w) _w <<= 1;

			_h = 1;
			while((_h << 1) <= max_h) _h <<= 1;

			//aspect ratio
			if(pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio != 0)
			{
				while( _w/_h > pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio ) _w >>= 1;
				while( _h/_w > pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio ) _h >>= 1;
			}
		}
	}
	else
	{
		_w = width;
		_h = height;

		//aspect ratio
		if( pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio != 0)
		{
			const DWORD& k = pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio;
			if(_w > _h * k) _h = (_w + (k - _w % k) % k) / k;
			if(_h > _w * k) _w = (_h + (k - _h % k) % k) / k;
		}

		if( _w > pGpu->GetHardwareCaps()->dwMaxTextureWidth || _h > pGpu->GetHardwareCaps()->dwMaxTextureHeight )
		{
			//max size
			_w = min( _w, pGpu->GetHardwareCaps()->dwMaxTextureWidth );
			_h = min( _h, pGpu->GetHardwareCaps()->dwMaxTextureHeight );

			//aspect ratio
			if( pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio != 0)
			{
				const DWORD& k = pGpu->GetHardwareCaps()->dwMaxTextureAspectRatio;
				if(_w > _h * k) _w = _h * k;
				if(_h > _w * k) _h = _w * k;
			}
		}
	}

	//mipmap
	DWORD mip = 0;
	while( mip < dwMipmapLevel && _w >= 2 && _h >= 2 )
	{
		_w >>= 1;
		_h >>= 1;
		mip++;
	}

	//dxtn
	if( bDxtn )
	{
		if(	pow2 )
		{
			_w = max(_w, 4);
			_h = max(_h, 4);
		}
		else
		{
			_w += (4 - _w % 4) % 4;
			_h += (4 - _h % 4) % 4;
			if( _w > pGpu->GetHardwareCaps()->dwMaxTextureWidth || _h > pGpu->GetHardwareCaps()->dwMaxTextureHeight )
			{
				_w -= 4;
				_h -= 4;
			}
		}
	}

	out->x = (INT)_w;
	out->y = (INT)_h;
}

INT GpuPPSortCmpFunc( const void* arg1, const void* arg2 )
{
	KGpuPresentationParameters* pp1 = (KGpuPresentationParameters*)arg1;
	KGpuPresentationParameters* pp2 = (KGpuPresentationParameters*)arg2;

	//windowed
	if( pp1->bWindowed == g_bInputWindowed && pp2->bWindowed != g_bInputWindowed )
		return -1;
	else if( pp1->bWindowed != g_bInputWindowed && pp2->bWindowed == g_bInputWindowed )
		return 1;

	//size
	INT size_diff1 = abs((INT)pp1->sDisplayMode.Width - (INT)g_dwInputWidth) + abs((INT)pp1->sDisplayMode.Height - (INT)g_dwInputHeight);
	INT size_diff2 = abs((INT)pp2->sDisplayMode.Width - (INT)g_dwInputWidth) + abs((INT)pp2->sDisplayMode.Height - (INT)g_dwInputHeight);
	if( size_diff1 < size_diff2 )
		return -1;
	else if( size_diff1 > size_diff2 )
		return 1;

	//dialog box
	if( g_bDisableFullscreenIme )
	{
		if( !pp1->bGdiDialogboxSupport && pp2->bGdiDialogboxSupport )
			return -1;
		else if( pp1->bGdiDialogboxSupport && !pp2->bGdiDialogboxSupport )
			return 1;
	}
	else
	{
		if( pp1->bGdiDialogboxSupport && !pp2->bGdiDialogboxSupport )
			return -1;
		else if( !pp1->bGdiDialogboxSupport && pp2->bGdiDialogboxSupport )
			return 1;		
	}

	//disp fmt
	DWORD disp_fmt_order1 = 0;
	while( pp1->sDisplayMode.Format != g_vecDisplayFmtPriority[disp_fmt_order1] )
		disp_fmt_order1++;

	DWORD disp_fmt_order2 = 0;
	while( pp2->sDisplayMode.Format != g_vecDisplayFmtPriority[disp_fmt_order2] )
		disp_fmt_order2++;

	if( disp_fmt_order1 < disp_fmt_order2 )
		return -1;
	else if( disp_fmt_order1 > disp_fmt_order2 )
		return 1;

	//bkbuf fmt
	DWORD bkbuf_fmt_order1 = 0;
	while( pp1->eBackbufferFormat != g_vecBkbufFmtPriority[bkbuf_fmt_order1] )
		bkbuf_fmt_order1++;

	DWORD bkbuf_fmt_order2 = 0;
	while( pp2->eBackbufferFormat != g_vecBkbufFmtPriority[bkbuf_fmt_order2] )
		bkbuf_fmt_order2++;

	if( bkbuf_fmt_order1 < bkbuf_fmt_order2 )
		return -1;
	else if( bkbuf_fmt_order1 > bkbuf_fmt_order2 )
		return 1;

	return 0;
}

BOOL ChooseBestGpuPP(IKGpu* _gpu, BOOL windowed, DWORD width, DWORD height, BOOL prefer_32bits_color, BOOL disable_fullscreen_ime, KGpuPresentationParameters* pp)
{
	//get all valid pps
	LPD3DBUFFER ppbuf;
	DWORD ppcount = _gpu->BuildValidPresentationParameterCombos( &ppbuf );
	if( !ppcount )
		return false;

	//set sorting params
	g_bDisableFullscreenIme = disable_fullscreen_ime;
	g_bInputWindowed = windowed;
	g_dwInputWidth = width;
	g_dwInputHeight = height;

	//display format
	g_vecDisplayFmtPriority.resize(0);
	if( prefer_32bits_color )
	{
		g_vecDisplayFmtPriority.push_back(D3DFMT_X8R8G8B8);
		g_vecDisplayFmtPriority.push_back(D3DFMT_R5G6B5);
		g_vecDisplayFmtPriority.push_back(D3DFMT_X1R5G5B5);
		g_vecDisplayFmtPriority.push_back(D3DFMT_A2R10G10B10);
	}
	else
	{
		g_vecDisplayFmtPriority.push_back(D3DFMT_R5G6B5);
		g_vecDisplayFmtPriority.push_back(D3DFMT_X1R5G5B5);
		g_vecDisplayFmtPriority.push_back(D3DFMT_X8R8G8B8);
		g_vecDisplayFmtPriority.push_back(D3DFMT_A2R10G10B10);
	}

	//backbuffer format
	g_vecBkbufFmtPriority.resize(0);
	if( prefer_32bits_color )
	{
		g_vecBkbufFmtPriority.push_back(D3DFMT_X8R8G8B8);
		g_vecBkbufFmtPriority.push_back(D3DFMT_A8R8G8B8);
		g_vecBkbufFmtPriority.push_back(D3DFMT_R5G6B5);
		g_vecBkbufFmtPriority.push_back(D3DFMT_X1R5G5B5);
		g_vecBkbufFmtPriority.push_back(D3DFMT_A1R5G5B5);
		g_vecBkbufFmtPriority.push_back(D3DFMT_A2R10G10B10);
	}
	else
	{
		g_vecBkbufFmtPriority.push_back(D3DFMT_R5G6B5);
		g_vecBkbufFmtPriority.push_back(D3DFMT_X1R5G5B5);
		g_vecBkbufFmtPriority.push_back(D3DFMT_A1R5G5B5);
		g_vecBkbufFmtPriority.push_back(D3DFMT_X8R8G8B8);
		g_vecBkbufFmtPriority.push_back(D3DFMT_A8R8G8B8);
		g_vecBkbufFmtPriority.push_back(D3DFMT_A2R10G10B10);
	}

	//sort
	qsort( ppbuf->GetBufferPointer(), ppcount, sizeof(KGpuPresentationParameters), GpuPPSortCmpFunc );

	*pp = *(KGpuPresentationParameters*)ppbuf->GetBufferPointer();
	ppbuf->Release();
	return true;
}

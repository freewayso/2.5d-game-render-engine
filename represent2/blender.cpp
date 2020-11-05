/* -------------------------------------------------------------------------
//	文件名		：	blender.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	调色版、混色算法定义
//
// -----------------------------------------------------------------------*/
#include "blender.h"

__int64 g_n64ColorMask =  0x001f001f001f001f;

void ModulatePalette(WORD* pDst, WORD* pSrc, DWORD dwCount, DWORD color, BOOL rgb565)
{
	BYTE r = (BYTE)((color >> 16) & 0xff);
	BYTE g = (BYTE)((color >> 8) & 0xff);
	BYTE b = (BYTE)(color & 0xff);
	if( rgb565 )
	{
		for( DWORD i = 0; i < dwCount; i++ )
		{
			pDst[i] = ((((pSrc[i] >> 11) * r) >> 8) << 11 ) |
				(((((pSrc[i] >> 5) & 0x3f) * g) >> 8) << 5) |
				(((pSrc[i] & 0x1f) * b) >> 8);
		}
	}
	else	//x555
	{
		for( DWORD i = 0; i < dwCount; i++ )
		{
			pDst[i] = (((((pSrc[i] >> 10) & 0x1f) * r) >> 8) << 10 ) |
				(((((pSrc[i] >> 5) & 0x1f) * g) >> 8) << 5) |
				(((pSrc[i] & 0x1f) * b) >> 8);
		}
	}
}

int	GetColorDt(int& nColor)
{
	int ndt = 0;
	if (nColor < 0)
	{
		ndt = nColor;
		nColor = 0;
	}
	if (nColor > 255)
	{
		ndt = nColor - 255;
		nColor = 255;
	}
	return ndt;
}

void GenerateExchangeColorPalette(WORD* pDst, COLORCOUNT* pSrc, DWORD dwCount, DWORD dwColor, BOOL bRGB565)
{
	BYTE btRed = (BYTE)(dwColor & 0xff);
	BYTE btGreen = (BYTE)((dwColor >> 8) & 0xff);
	BYTE btBlue = (BYTE)((dwColor >> 16) & 0xff);
	BYTE bSourceCy = (btRed * 76 + btGreen * 150 + btBlue * 29) >> 8;

	for( DWORD i = 0; i < dwCount; i++ )
	{
		int nDesCy = pSrc[i].b1; 

		//  使用hsl颜色空间
		FLOAT H, S, L ;    

		L = (FLOAT)nDesCy / 255 ;

		int temp_R,temp_G,temp_B;    //合成RGB（中间量，未做亮度补偿）

		FLOAT var_R = (FLOAT)btRed /255 ;                      //RGB values = 0 ~ 255
		FLOAT var_G = (FLOAT)btGreen /255;
		FLOAT var_B = (FLOAT)btBlue /255;

		//FLOAT btL = 0.3 * var_R + 0.59 * var_G + 0.11 * var_B ;

		FLOAT var_Min = min( var_R, min (var_G, var_B ) );    // RGB最小值
		FLOAT var_Max = max( var_R, max (var_G, var_B ) );    // RGB最大值
		FLOAT del_Max = var_Max - var_Min  ;         //Delta RGB value

		FLOAT btL = ( var_Max + var_Min ) / 2 ;

		// L = (double)nDesCy;  //混合亮度计算

		if ( del_Max == 0 )                     //This is a gray, no chroma...
		{
			H = 0 ;                               //HSL results = 0 ÷ 1
			S = 0 ;
		}
		else                                    //Chromatic data...
		{
			if ( btL < 0.5 ) 
				S = (FLOAT)del_Max / ( var_Max + var_Min ) ;
			else           
				S = (FLOAT)del_Max / ( 2 - var_Max - var_Min ) ;

			FLOAT del_R = ( ( ( var_Max - var_R ) / 6 ) + ( del_Max / 2 ) ) / del_Max ;
			FLOAT del_G = ( ( ( var_Max - var_G ) / 6 ) + ( del_Max / 2 ) ) / del_Max ;
			FLOAT del_B = ( ( ( var_Max - var_B ) / 6 ) + ( del_Max / 2 ) ) / del_Max ;

			if  ( var_R == var_Max ) 
				H = del_B - del_G ;
			else if ( var_G == var_Max ) 
				H = ( 1.0f / 3 ) + del_R - del_B ;
			else if ( var_B == var_Max ) 
				H = ( 2.0f / 3 ) + del_G - del_R ;

			if ( H < 0 ) H += 1;
			if ( H > 1 ) H -= 1;
		}

		//反计算

		FLOAT var_1,var_2,delta;

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

			FLOAT t1,t2,t3;

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

		COLORCOUNT c;
		c.b1  = (BYTE)(temp_R);
		c.b2  = (BYTE)(temp_G);
		c.b3  = (BYTE)(temp_B);

		if( bRGB565 )
		{
			pDst[i] = ((c.b1 >> 3) << 11) | ((c.b2 >> 2) << 5) | (c.b3 >> 3);
		}
		else	//x555
		{
			pDst[i] = ((c.b1 >> 3) << 10) | ((c.b2 >> 3) << 5) | (c.b3 >> 3);
		}
	}
}

//color functions
WORD Color32To16( DWORD dwColor32, BOOL bRGB565 )
{
	if( bRGB565 )
	{
		return  (WORD)
			(((dwColor32 & 0xf80000) >> 8) |
			((dwColor32 & 0x00fc00) >> 5) |
			((dwColor32 & 0x0000f8) >> 3));
	}
	else
	{
		return  (WORD)
			(((dwColor32 & 0xf80000) >> 9) |
			((dwColor32 & 0x00f800) >> 6) |
			((dwColor32 & 0x0000f8) >> 3));
	}
}

void ScreenBlend1Pixel(
					   void* pDest,
					   void* pColor,
					   void* pAlpha )
{
	__asm
	{
		xor eax,eax
			mov ax, [ word ptr pDest ]
		movd mm0,eax
			mov ax,[word ptr pColor ]
		movd mm1,eax
			mov ax,[word ptr pColor ]
		movd mm7, eax
			movq mm6,g_n64ColorMask


			////r///////
			movq mm2,mm0
			psrlw mm2,11
			movq mm3, mm1
			psrlw mm3, 11
			pmullw mm3, mm7
			psrlw  mm3, 8
			movq   mm4, mm3
			paddw  mm3,mm2
			pmullw mm4,mm2
			paddw mm4,mm6
			psrlw mm4,5
			psubw mm3,mm4

			////g///////

			///G /////////
			movq  mm2,mm0
			psllw mm2,5
			psrlw mm2,11
			movq  mm4,mm1
			psllw mm4,5
			psrlw mm4,11
			pmullw mm4, mm7
			psrlw mm4,8
			movq mm5,mm4
			paddw mm4,mm2
			pmullw mm5,mm2
			paddw  mm5,mm6 
			psrlw mm5,5
			psubw mm4,mm5


			//////b///////
			pand mm0, mm6
			pand mm1,mm6
			pmullw mm1,mm7
			psrlw mm1,8
			movq mm5,mm1
			paddw mm1,mm0
			pmullw mm5,mm0
			paddw  mm5,mm6 
			psrlw mm5,5
			psubw mm1,mm5

			psllw mm3,11
			psllw mm4,6
			por  mm1,mm3
			por  mm1,mm4

			movd eax,mm1
			mov  [ word ptr pDest ], ax


			emms



	}
}

void ScreenBlend2Pixel(
					   void* pDest,
					   void* pColor,
					   void* pAlpha )
{
	__asm
	{
		movd mm0,[dword ptr pDest]  //MM0 目标颜色
		movd mm1,[ dword ptr pColor ]//mm1 源颜色
		movd mm7,[dword ptr pAlpha ] //mm7 alpha
		movq mm6,g_n64ColorMask

			////r///////
			movq mm2,mm0
			psrlw mm2,11
			movq mm3, mm1
			psrlw mm3, 11
			pmullw mm3, mm7
			psrlw  mm3, 8
			movq   mm4, mm3
			paddw  mm3,mm2
			pmullw mm4,mm2
			paddw mm4,mm6
			psrlw mm4,5
			psubw mm3,mm4

			////g///////

			///G /////////
			movq  mm2,mm0
			psllw mm2,5
			psrlw mm2,11
			movq  mm4,mm1
			psllw mm4,5
			psrlw mm4,11
			pmullw mm4, mm7
			psrlw mm4,8
			movq mm5,mm4
			paddw mm4,mm2
			pmullw mm5,mm2
			paddw  mm5,mm6 
			psrlw mm5,5
			psubw mm4,mm5


			//////b///////
			pand mm0, mm6
			pand mm1,mm6
			pmullw mm1,mm7
			psrlw mm1,8
			movq mm5,mm1
			paddw mm1,mm0
			pmullw mm5,mm0
			paddw  mm5,mm6 
			psrlw mm5,5
			psubw mm1,mm5


			psllw mm3,11
			psllw mm4,6
			por  mm1,mm3
			por  mm1,mm4

			movd [dword ptr pDest],mm1 

			emms
	}
}

void ScreenBlend4Pixel( 
					   void*  pDest ,
					   void*  pColor,
					   void*  pAlpha )
{
	__asm
	{
		mov eax ,dword ptr pDest
			movq mm0, [eax]    //mm0 目标颜色
		mov ebx ,dword ptr pColor //mm1 保存源颜色
			movq mm1,[ebx]
		mov ebx, dword ptr pAlpha
			movq mm7 , [ebx]   //mm7
		movq mm6, g_n64ColorMask

			/////r//////////
			movq mm2,mm0
			psrlw mm2, 11
			movq mm3,mm1
			psrlw mm3,11
			pmullw mm3,mm7
			psrlw mm3,8
			movq mm4, mm3
			paddw mm3, mm2
			pmullw mm4,mm2
			paddw  mm4,mm6 
			psrlw mm4,5
			psubw mm3,mm4   //mm3 R
			/////////////////////////

			///G /////////
			movq  mm2,mm0
			psllw mm2,5
			psrlw mm2,11
			movq  mm4,mm1
			psllw mm4,5
			psrlw mm4,11
			pmullw mm4, mm7
			psrlw mm4,8
			movq mm5,mm4
			paddw mm4,mm2
			pmullw mm5,mm2
			paddw  mm5,mm6 
			psrlw mm5,5
			psubw mm4,mm5

			//////b///////
			pand mm0, mm6
			pand mm1,mm6
			pmullw mm1,mm7
			psrlw mm1,8
			movq mm5,mm1
			paddw mm1,mm0
			pmullw mm5,mm0
			paddw  mm5,mm6 
			psrlw mm5,5
			psubw mm1,mm5 
			// mm3 r,mm4 g,mm1 b

			psllw mm3,11
			psllw mm4,6
			por  mm1,mm3
			por  mm1,mm4
			movq [eax] ,mm1
			emms

	}
}

WORD AlphaBlend(WORD pDst, WORD wSrc, BYTE byAlpha, BOOL bRGB565)
{
	WORD wSrcR, wSrcG, wSrcB;
	if( !bRGB565 )
	{
		wSrcR = (wSrc & 0x7c00) >> 10;
		wSrcG = (wSrc & 0x03e0) >> 5;
		wSrcB = (wSrc & 0x001f);
	}
	else
	{
		wSrcR = (wSrc & 0xf800) >> 11;
		wSrcG = (wSrc & 0x07c0) >> 6;
		wSrcB = (wSrc & 0x001f);
	}

	WORD wDstR, wDstG, wDstB;
	if( !bRGB565 )
	{
		wDstR = (pDst & 0x7c00) >> 10;
		wDstG = (pDst & 0x03e0) >> 5;
		wDstB = (pDst & 0x001f);
	}
	else
	{
		wDstR = (pDst & 0xf800) >> 11;
		wDstG = (pDst & 0x07c0) >> 6;
		wDstB = (pDst & 0x001f);
	}

	WORD wDstAMask = 0;

	WORD wAnsR, wAnsG, wAnsB;
	wAnsR = ((wSrcR * byAlpha + wDstR * (255 - byAlpha)) / 255) & 0x1f;
	wAnsG = ((wSrcG * byAlpha + wDstG * (255 - byAlpha)) / 255) & 0x1f;
	wAnsB = ((wSrcB * byAlpha + wDstB * (255 - byAlpha)) / 255) & 0x1f;

	return (!bRGB565) ? (wDstAMask | (wAnsR << 10) | (wAnsG << 5) | wAnsB) : (wDstAMask | (wAnsR << 11) | (wAnsG << 6) | wAnsB);
}

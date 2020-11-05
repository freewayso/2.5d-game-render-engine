/* -------------------------------------------------------------------------
//	文件名		：	kernel_text.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_text的定义,文本的定义
//
// -----------------------------------------------------------------------*/
#include "kernel_text.h"
#include "igpu.h"
#include "sprite.h"
#include "media_center.h"
#include "kernel_center.h"
#include "kernel_picture.h"
#include "kernel_line.h"

//buf
vector<KSymbolDrawable>	g_vecSymbolDrawable;
vector<BOOL>				g_vecIsIcon;
vector<DWORD>				g_vecCharIndex;
vector<DWORD>				g_vecIconIndex;
KFont*						g_pCurFont;

//gpu objects
LPD3DIB						g_pTextMultiquadIndexBuffer = 0;
DWORD						g_dwCharBodySrs;
DWORD						g_dwCharBorderSrs;
DWORD						g_dwCharBorderSrsEx;
DWORD						g_dwCharBodySrsAntialiased;
BOOL						g_bUseColor2;

//init
KTextConfig g_sTextConfig;

//batch
vector<KLineCmd> g_vecUnderLineCmd;

//flush m_pwszText
KSymbolDrawable*	g_pIconSortVecBase;
BOOL	g_bScissorTestEnable = false;
RECT	g_sScissorRect;

BOOL InitTextKernel(void* arg)
{
	KTextKernelInitParam* param = (KTextKernelInitParam*)arg;
	if( !param->pMultiquadIb )
		return false;
	g_pTextMultiquadIndexBuffer = param->pMultiquadIb;
	g_pTextMultiquadIndexBuffer->AddRef();
	g_sTextConfig.bWordWrap = param->bWordWrap;
	g_sTextConfig.bTextJustification = param->bTextJustification;

	//static states - body
	KStaticRenderStates sCharBodySrsDef;

	sCharBodySrsDef.dwLighting				= false;
	sCharBodySrsDef.dwCullMode				= D3DCULL_NONE;
	sCharBodySrsDef.dwAlphaTestEnable		= true;
	sCharBodySrsDef.dwAlphaRef				= 0xff;
	sCharBodySrsDef.dwAlphaFunc			= D3DCMP_EQUAL;
	sCharBodySrsDef.dwZEnable				= D3DZB_FALSE;
	sCharBodySrsDef.dwZFunc				= D3DCMP_LESSEQUAL;
	sCharBodySrsDef.dwZWriteEnable		= true;
	sCharBodySrsDef.dwAlphaBlendEnable	= false;
	sCharBodySrsDef.dwBlendOp				= D3DBLENDOP_ADD;
	sCharBodySrsDef.dwSrcBlend				= D3DBLEND_SRCALPHA;
	sCharBodySrsDef.dwDestBlend			= D3DBLEND_INVSRCALPHA;
	sCharBodySrsDef.dwColorOp				= D3DTOP_SELECTARG2;
	sCharBodySrsDef.dwColorArg1			= D3DTA_TEXTURE;
	sCharBodySrsDef.dwColorArg2			= D3DTA_DIFFUSE;
	sCharBodySrsDef.dwAlphaOp				= D3DTOP_SELECTARG1;
	sCharBodySrsDef.dwAlphaArg1			= D3DTA_TEXTURE;
	sCharBodySrsDef.dwAlphaArg2			= D3DTA_DIFFUSE;
	sCharBodySrsDef.dwAddressU				= D3DTADDRESS_CLAMP;
	sCharBodySrsDef.dwAddressV				= D3DTADDRESS_CLAMP;
	sCharBodySrsDef.dwMagFilter			= D3DTEXF_POINT;
	sCharBodySrsDef.dwMinFilter			= D3DTEXF_POINT;

	g_dwCharBodySrs = Gpu()->DefineStaticRenderStates(&sCharBodySrsDef);

	//static states - body antialiased
	KStaticRenderStates sCharBodySrsDefAntialiased;

	sCharBodySrsDefAntialiased.dwLighting				= false;
	sCharBodySrsDefAntialiased.dwCullMode				= D3DCULL_NONE;
	sCharBodySrsDefAntialiased.dwAlphaTestEnable		= true;
	sCharBodySrsDefAntialiased.dwAlphaRef				= 0;
	sCharBodySrsDefAntialiased.dwAlphaFunc			= D3DCMP_GREATER;
	sCharBodySrsDefAntialiased.dwZEnable				= D3DZB_FALSE;
	sCharBodySrsDefAntialiased.dwZFunc				= D3DCMP_LESSEQUAL;
	sCharBodySrsDefAntialiased.dwZWriteEnable		= true;
	sCharBodySrsDefAntialiased.dwAlphaBlendEnable	= true;
	sCharBodySrsDefAntialiased.dwBlendOp				= D3DBLENDOP_ADD;
	sCharBodySrsDefAntialiased.dwSrcBlend				= D3DBLEND_SRCALPHA;
	sCharBodySrsDefAntialiased.dwDestBlend			= D3DBLEND_INVSRCALPHA;
	sCharBodySrsDefAntialiased.dwColorOp				= D3DTOP_SELECTARG2;
	sCharBodySrsDefAntialiased.dwColorArg1			= D3DTA_TEXTURE;
	sCharBodySrsDefAntialiased.dwColorArg2			= D3DTA_DIFFUSE;
	sCharBodySrsDefAntialiased.dwAlphaOp				= D3DTOP_SELECTARG1;
	sCharBodySrsDefAntialiased.dwAlphaArg1			= D3DTA_TEXTURE;
	sCharBodySrsDefAntialiased.dwAlphaArg2			= D3DTA_DIFFUSE;
	sCharBodySrsDefAntialiased.dwAddressU				= D3DTADDRESS_CLAMP;
	sCharBodySrsDefAntialiased.dwAddressV				= D3DTADDRESS_CLAMP;
	sCharBodySrsDefAntialiased.dwMagFilter			= D3DTEXF_POINT;
	sCharBodySrsDefAntialiased.dwMinFilter			= D3DTEXF_POINT;

	g_dwCharBodySrsAntialiased = Gpu()->DefineStaticRenderStates(&sCharBodySrsDefAntialiased);

	//static states - border
	KStaticRenderStates sCharBorderSrsDef;

	sCharBorderSrsDef.dwLighting				= false;
	sCharBorderSrsDef.dwCullMode				= D3DCULL_NONE;
	sCharBorderSrsDef.dwAlphaTestEnable		= true;
	sCharBorderSrsDef.dwAlphaRef				= 0;
	sCharBorderSrsDef.dwAlphaFunc				= D3DCMP_EQUAL;
	sCharBorderSrsDef.dwZEnable				= D3DZB_FALSE;
	sCharBorderSrsDef.dwZFunc					= D3DCMP_LESSEQUAL;
	sCharBorderSrsDef.dwZWriteEnable			= true;
	sCharBorderSrsDef.dwAlphaBlendEnable		= false;
	sCharBorderSrsDef.dwBlendOp				= D3DBLENDOP_ADD;
	sCharBorderSrsDef.dwSrcBlend				= D3DBLEND_SRCALPHA;
	sCharBorderSrsDef.dwDestBlend				= D3DBLEND_INVSRCALPHA;
	sCharBorderSrsDef.dwColorOp				= D3DTOP_SELECTARG2;
	sCharBorderSrsDef.dwColorArg1				= D3DTA_TEXTURE;
	sCharBorderSrsDef.dwColorArg2				= D3DTA_SPECULAR;
	sCharBorderSrsDef.dwAlphaOp				= D3DTOP_SELECTARG1;
	sCharBorderSrsDef.dwAlphaArg1				= D3DTA_TEXTURE;
	sCharBorderSrsDef.dwAlphaArg2				= D3DTA_DIFFUSE;
	sCharBorderSrsDef.dwAddressU				= D3DTADDRESS_CLAMP;
	sCharBorderSrsDef.dwAddressV				= D3DTADDRESS_CLAMP;
	sCharBorderSrsDef.dwMagFilter				= D3DTEXF_POINT;
	sCharBorderSrsDef.dwMinFilter				= D3DTEXF_POINT;

	g_dwCharBorderSrs = Gpu()->DefineStaticRenderStates(&sCharBorderSrsDef);

	////static states - border ex
	KStaticRenderStates sCharBorderSrsDefEx;

	sCharBorderSrsDefEx.dwLighting				= false;
	sCharBorderSrsDefEx.dwCullMode			= D3DCULL_NONE;
	sCharBorderSrsDefEx.dwAlphaTestEnable	= true;
	sCharBorderSrsDefEx.dwAlphaRef			= 0;
	sCharBorderSrsDefEx.dwAlphaFunc			= D3DCMP_EQUAL;
	sCharBorderSrsDefEx.dwZEnable				= D3DZB_FALSE;
	sCharBorderSrsDefEx.dwZFunc				= D3DCMP_LESSEQUAL;
	sCharBorderSrsDefEx.dwZWriteEnable		= true;
	sCharBorderSrsDefEx.dwAlphaBlendEnable	= false;
	sCharBorderSrsDefEx.dwBlendOp				= D3DBLENDOP_ADD;
	sCharBorderSrsDefEx.dwSrcBlend			= D3DBLEND_SRCALPHA;
	sCharBorderSrsDefEx.dwDestBlend			= D3DBLEND_INVSRCALPHA;
	sCharBorderSrsDefEx.dwColorOp				= D3DTOP_SELECTARG2;
	sCharBorderSrsDefEx.dwColorArg1			= D3DTA_TEXTURE;
	sCharBorderSrsDefEx.dwColorArg2			= D3DTA_DIFFUSE;
	sCharBorderSrsDefEx.dwAlphaOp				= D3DTOP_SELECTARG1;
	sCharBorderSrsDefEx.dwAlphaArg1			= D3DTA_TEXTURE;
	sCharBorderSrsDefEx.dwAlphaArg2			= D3DTA_DIFFUSE;
	sCharBorderSrsDefEx.dwAddressU			= D3DTADDRESS_CLAMP;
	sCharBorderSrsDefEx.dwAddressV			= D3DTADDRESS_CLAMP;
	sCharBorderSrsDefEx.dwMagFilter			= D3DTEXF_POINT;
	sCharBorderSrsDefEx.dwMinFilter			= D3DTEXF_POINT;

	g_dwCharBorderSrsEx = Gpu()->DefineStaticRenderStates(&sCharBorderSrsDefEx);

	g_bUseColor2 = true;		//try to use color2 first
	g_pCurFont = 0;
	return true;
}

void ShutdownTextKernel()
{
	SAFE_RELEASE(g_pTextMultiquadIndexBuffer);
	g_vecSymbolDrawable.clear();
	g_vecIsIcon.clear();
	g_vecCharIndex.clear();
	g_vecIconIndex.clear();
	g_pCurFont = 0;
}

void BatchTextCmd(void* pCmd, void* pArg)		//semi-batch in fact
{
	KTextCmd* in = (KTextCmd*)pCmd;

	KFont* font = 0;
	if( !QueryProduct(in->dwFontHandle, false, (void**)&font) || !font )
		return;

	if( font != g_pCurFont )
	{
		FlushTextBuf();
		g_pCurFont = font;
	}

	static vector<DWORD>	vecColorStack;
	static vector<DWORD>	vecBorderColorStack;

	KPOINT					pen = in->cPos;				//position of the pen
	pen.y -= in->dwStartLine * in->nLineInterSpace;		//patch y
	DWORD					dwLineIndex = 0;				//current line index
	DWORD					b, e;						//'b' point to the first CHAR in current line, 'e' point to the one next to the last CHAR in current line
	b = e = (DWORD)g_vecSymbolDrawable.size();

	//save the start index for topcenter option
	DWORD					start_char_index = (DWORD)g_vecCharIndex.size();
	DWORD					start_icon_index = (DWORD)g_vecIconIndex.size();

	BOOL					newline = true;
	BOOL					underline = false;

	//push the initial color/border-color
	vecColorStack.push_back(in->dwColor);
	vecBorderColorStack.push_back(in->dwBorderColor);

	BOOL bCheckLineBreaking = TRUE;
	BOOL bSupportWholeWordWrap = JXTextRender::IsWholeWordWrap();
	INT nCurLineSpaceCount = 0;	//统计一下当前行的空格个数

	DWORD cursor = 0;
	while(cursor < in->dwBytes)
	{
		switch(in->pwchText[cursor])
		{
		case 0:
			cursor = in->dwBytes;
			break;
		case 0x02:
			if(in->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(cursor + 3 < in->dwBytes)
				{
					vecColorStack.push_back(D3DCOLOR_ARGB(0xff, in->pwchText[cursor+1], in->pwchText[cursor+2], in->pwchText[cursor+3]));
					cursor += 4;
				}
				else
				{
					//parsing done
					cursor = in->dwBytes;
					break;
				}
				break;
			}
		case 0x03:
			if(in->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(vecColorStack.size() > 1) vecColorStack.pop_back(); 
				cursor++;
				break;
			}
		case 0x04:
			if(in->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(cursor + 3 < in->dwBytes)
				{
					vecBorderColorStack.push_back(D3DCOLOR_ARGB(0xff, in->pwchText[cursor+1], in->pwchText[cursor+2], in->pwchText[cursor+3]));
					cursor += 4;
				}
				else
				{
					//parsing done
					cursor = in->dwBytes;
					break;
				}
				break;
			}
		case 0x05:
			if(in->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(vecBorderColorStack.size() > 1) vecBorderColorStack.pop_back();
				cursor++;
				break;
			}
		case 0x10:
			if(in->dwOptions & emTEXT_CMD_UNDERLINE_TAG)
			{
				underline = !underline;
				cursor++;
				break;
			}
		case 0x0A:	//newline
			{
				b = e;
				pen.x = in->cPos.x;
				pen.y += in->nLineInterSpace;
				newline = true;
				if(++dwLineIndex == in->dwEndLine)
				{
					//parsing done
					cursor = in->dwBytes;
					break;
				}
				cursor++;
			}
			break;
		default:
			{
				//get code, check whether it is an icon(inline picture)
				WORD	code; 
				BOOL	icon;
				BYTE	ctrl_id = 0;
				if( (in->dwOptions & emTEXT_CMD_INLINE_PICTURE) && JXTextRender::GetCtrlHandleW(in->pwchText[cursor]) )	//icon
				{
					if(cursor + 1 < in->dwBytes) 
					{
						ctrl_id = (BYTE)in->pwchText[cursor];
						code = *((WORD*)(in->pwchText + cursor + 1));
						icon = true;
						cursor += 2;
					}
					else
					{
						//parsing done
						cursor = in->dwBytes;
						break;
					}
				}
				//	else if( _IsDBCSLeadingByte(in->m_pwszText[cursor]) )	//DOUBLE byte character
				//	{
				//		code = in->m_pwszText[cursor]|(in->m_pwszText[cursor+1]<<8);
				//		icon = false;
				//		cursor += 2;
				//	}
				else if(in->pwchText[cursor] >= 0x20)	//unicode character (including white-space)
				{					
					code = in->pwchText[cursor];
					icon = false;
					cursor++;

					//统计当前行的空格个数
					if (code == 0x20)
						nCurLineSpaceCount++;
				}
				else	//non-printable CHAR
				{
					cursor++;
					break;
				}

				if( g_sTextConfig.bWordWrap && !icon && code == 0x20 && !newline && b==e )	//skip leading blank of broken line
					break;

				//get size
				KPOINT size;
				if(icon)
				{
					if( !SUCCEEDED(JXTextRender::GetCtrlHandleW(ctrl_id)->GetSize(code, in->nCharInterSpace, size.x, size.y)) )	//skip invalid icon
						break;

					//apply scaling
					size.x = (int)(size.x * in->fInlinePicScaling);
					size.y = (int)(size.y * in->fInlinePicScaling);
				}
				else
				{
					size.x = g_pCurFont->GetCharWidth(code);
					size.y = g_pCurFont->GetCharHeight();
				}

				//put clipping code here

				//write the char virtually
				e++;

				KSymbolDrawable elem;
				elem.bUnderLine = underline;
				elem.wCode = code;
				elem.byCtrlId = ctrl_id;
				elem.cPos.x = pen.x;
				elem.cPos.y = icon?(pen.y - ((int)size.y - (int)g_pCurFont->GetCharHeight())/2):pen.y;
				elem.cSize = size;
				elem.dwColor = vecColorStack.back();
				elem.dwBorderColor = vecBorderColorStack.back();

				g_vecSymbolDrawable.push_back(elem);
				g_vecIsIcon.push_back(icon);

				//add to the index buffer temporary
				if(dwLineIndex >= in->dwStartLine && dwLineIndex < in->dwEndLine)
				{
					if(icon)
					{
						g_vecIconIndex.push_back(e-1);
					}
					else if (code != 0x20)
					{
						g_vecCharIndex.push_back(e-1);
					}
				}

				//move pen
				if(!icon)
				{
					//	if( _IsDBCSDoubleByteChar(code) )
					//		pen.x += in->m_nCharInterSpace;
					//	else
					//		pen.x += in->m_nCharInterSpace/2;
					pen.x += JXTextRender::GetTextWidth((const WCHAR*)&code, 1, in->nCharInterSpace);
				}
				else
				{
					int w = in->nCharInterSpace/2;
					while(w< size.x)
					{
						w += in->nCharInterSpace/2;
					}
					pen.x += w;
				}

				if (bSupportWholeWordWrap)
				{
					//要不等于空格，要不到最后了
					bCheckLineBreaking = (code == 0x20 || icon == TRUE) || (nCurLineSpaceCount == 0 ) || (cursor == in->dwBytes);
				}

				//check for line breaking
				while(pen.x - in->cPos.x > in->nLineWidthLimit)
				{
					//find the break point
					DWORD e1 = 0;
					DWORD e_t = 0;

					if( !g_sTextConfig.bWordWrap )
					{
						e1 = e - 1;		//e1 will be next of the tail CHAR of the broken line						

						if(e1 <= b) 
							e1 = b + 1;

						e_t = e1;
					}
					else
					{
						DWORD i;
						for( i = e-1; i > b; i-- )
						{
							//	if( !(!is_icon_vec[i] && !_IsDBCSDoubleByteChar(g_vecSymbolDrawable[i].code) && g_vecSymbolDrawable[i].code!=0x20 && !is_icon_vec[i-1] && !_IsDBCSDoubleByteChar(g_vecSymbolDrawable[i-1].code) && g_vecSymbolDrawable[i-1].code!=0x20) )
							if( !(!g_vecIsIcon[i] && g_vecSymbolDrawable[i].wCode!=0x20 && !g_vecIsIcon[i-1] && g_vecSymbolDrawable[i-1].wCode!=0x20) )
							{
								e1 = i;
								break;
							}
						}
						if( i==b )
						{
							e1 = e - 1;
							if(e1 <= b) 
								e1 = b + 1;
						}
					}

					//start m_pwszText justification here
					if( !bSupportWholeWordWrap || nCurLineSpaceCount == 0)
						goto after_text_justification;

					DWORD b0 = 0;
					DWORD e0 = 0;
					{
						DWORD i;
						for( i = b; i < e1; i++ )
						{
							if( g_vecIsIcon[i] || g_vecSymbolDrawable[i].wCode!=0x20 )
							{
								b0 = i;
								break;
							}
						}
						if( i >= e1 )
							goto after_text_justification;
					}
					for(int i = e1; i >= (int)b; i-- )
					{
						if( g_vecIsIcon[i] || g_vecSymbolDrawable[i].wCode!=0x20 )
						{
							e0 = i;
							break;
						}
					}

					//modify by wdb	
					for (int i = e0 - 1; i >= (int)b; i--)
					{
						if (g_vecIsIcon[i] || g_vecSymbolDrawable[i].wCode == 0x20)
						{
							if (i > 0) //到头，表示只有1个词
							{
								e_t = i + 1;
							}
							ASSERT(e_t >= 2);
							break;
						}
					}

					int break_count = 0;
					for( DWORD i = b0+1; i <= e0; i++ )
					{
						if (!g_vecIsIcon[i] && g_vecSymbolDrawable[i-1].wCode==0x20)
							break_count++;
						//	if (is_icon_vec[i-1] || is_icon_vec[i] || _IsDBCSDoubleByteChar(g_vecSymbolDrawable[i-1].code) || _IsDBCSDoubleByteChar(g_vecSymbolDrawable[i].code))
						if (g_vecIsIcon[i-1] || g_vecIsIcon[i])
							break_count++;
					}
					if(break_count==0)
						goto after_text_justification;
					int ext_space = in->nLineWidthLimit + in->cPos.x - (g_vecSymbolDrawable[e0].cPos.x + g_vecSymbolDrawable[e0].cSize.x);
					int n = ext_space/break_count;
					int t = ext_space%break_count;

					//modify by wdb, 和text.dll判断手法保持一致
					//	if (n >= -1)//判断留在本行
					if (((in->nLineWidthLimit - (pen.x - in->cPos.x)) / break_count) >= -1 && in->nFontId > 12)
					{
						e_t = e1 + 1;
					}
					else	   //放在下一行
					{
						e0 = e_t - 2;
						ext_space = in->nLineWidthLimit + in->cPos.x - (g_vecSymbolDrawable[e0].cPos.x + g_vecSymbolDrawable[e0].cSize.x);
						n = ext_space/break_count;
						t = ext_space%break_count;						
					}

					int sum_break_count = 0;
					int sum_offset = 0;
					for (DWORD i = b0+1; i <= e0; i++)
					{
						int delta_break_count = 0;
						int delta_offset = 0;
						if (!g_vecIsIcon[i] && g_vecSymbolDrawable[i-1].wCode==0x20)
							delta_break_count++;
						//	if (is_icon_vec[i-1] || is_icon_vec[i] || _IsDBCSDoubleByteChar(g_vecSymbolDrawable[i-1].code) || _IsDBCSDoubleByteChar(g_vecSymbolDrawable[i].code) )
						if (g_vecIsIcon[i-1] || g_vecIsIcon[i])
							delta_break_count++;

						for (int k = sum_break_count; k < sum_break_count + delta_break_count; k++)
						{
							if (t>0 && k % (break_count/t) == 0 && k < t*(break_count/t))
								delta_offset += (n+1);
							else 
								delta_offset += n;
						}
						sum_break_count += delta_break_count;
						sum_offset += delta_offset;
						g_vecSymbolDrawable[i].cPos.x += sum_offset;
					}
					//end m_pwszText justification
after_text_justification:

					//break line
					if( !g_sTextConfig.bWordWrap )
						//	b = e1;
						b = e_t;
					else
					{
						DWORD i;
						for( i = e1; i < e; i++ )
						{
							if( g_vecIsIcon[i] || g_vecSymbolDrawable[i].wCode!=0x20 )
							{
								b = i;
								break;
							}
						}
						if( i==e )
							b = e;
					}

					//move to next line
					if(b < e)
					{
						int offset = g_vecSymbolDrawable[b].cPos.x - in->cPos.x;
						for(DWORD i = b; i < e; i++)
						{
							g_vecSymbolDrawable[i].cPos.x -= offset;
							g_vecSymbolDrawable[i].cPos.y += in->nLineInterSpace;
						}
						pen.x -= offset;
					}
					else
					{
						pen.x = in->cPos.x;
					}
					pen.y += in->nLineInterSpace;

					//inc line_index
					dwLineIndex++;

					newline = false;

					nCurLineSpaceCount = 0;
					//check the line index
					if(dwLineIndex == in->dwStartLine)	//enter the scope
					{
						for(DWORD i = b; i < e; i++)
						{
							if(g_vecIsIcon[i])
							{
								g_vecIconIndex.push_back(i);
							}
							else if(g_vecSymbolDrawable[i].wCode != 0x20)
							{
								g_vecCharIndex.push_back(i);
							}
						}
					}
					else if(dwLineIndex == in->dwEndLine)	//exit the scope
					{
						for(DWORD i = b; i < e; i++)
						{
							if(g_vecIsIcon[i])
							{
								g_vecIconIndex.pop_back();
							}
							else if(g_vecSymbolDrawable[i].wCode != 0x20)
							{
								g_vecCharIndex.pop_back();
							}
						}

						//parsing done
						cursor = in->dwBytes;
						break;
					}
				}
			}
		}
	}

	//clear color stacks
	vecColorStack.clear();
	vecBorderColorStack.clear();

	//underline
	if(in->dwOptions & emTEXT_CMD_UNDERLINE_TAG)
	{
		for( DWORD i = start_char_index; i < g_vecCharIndex.size(); i++ )
		{
			DWORD char_index = g_vecCharIndex[i];
			if( g_vecSymbolDrawable[char_index].bUnderLine )
			{
				g_vecUnderLineCmd.resize( g_vecUnderLineCmd.size() + 1 );
				KLineCmd* cmd = &g_vecUnderLineCmd[g_vecUnderLineCmd.size()-1];
				cmd->sPos1.x = (FLOAT)g_vecSymbolDrawable[char_index].cPos.x;
				cmd->sPos1.y = (FLOAT)g_vecSymbolDrawable[char_index].cPos.y + g_vecSymbolDrawable[char_index].cSize.y;
				cmd->sPos2.x = (FLOAT)g_vecSymbolDrawable[char_index].cPos.x + g_vecSymbolDrawable[char_index].cSize.x;
				cmd->sPos2.y = cmd->sPos1.y;
				cmd->dwColor1 = cmd->dwColor2 = g_vecSymbolDrawable[char_index].dwColor;
			}
		}
	}
}

//flush m_pwszText
void FlushTextCharBuf()
{
	static vector<D3DXVECTOR2> vecCharUV1;
	static vector<D3DXVECTOR2> vecCharUV2;

	static vector<DWORD>	vecRealIndex;
	if( g_bScissorTestEnable )
	{
		for( DWORD i = 0; i < g_vecCharIndex.size(); i++ )
		{
			KSymbolDrawable& symbol = g_vecSymbolDrawable[ g_vecCharIndex[i] ];
			if( Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
			{
				//intersect
				if( min( symbol.cPos.x + symbol.cSize.x, g_sScissorRect.right ) > max( symbol.cPos.x, g_sScissorRect.left ) &&
					min( symbol.cPos.y + symbol.cSize.y, g_sScissorRect.bottom ) > max( symbol.cPos.y, g_sScissorRect.top ) )
				{
					vecRealIndex.push_back( g_vecCharIndex[i] );
				}
			}
			else
			{
				//contained in
				if( symbol.cPos.x >= g_sScissorRect.left && symbol.cPos.y >= g_sScissorRect.top &&
					symbol.cPos.x + symbol.cSize.x <= g_sScissorRect.right && symbol.cPos.y + symbol.cSize.y <= g_sScissorRect.bottom )
				{
					vecRealIndex.push_back( g_vecCharIndex[i] );
				}
			}
		}

		if( vecRealIndex.size() != g_vecCharIndex.size() )
		{
			g_vecCharIndex.resize( vecRealIndex.size() );
			for( DWORD i = 0; i < vecRealIndex.size(); i++ )
			{
				g_vecCharIndex[i] = vecRealIndex[i];
			}
		}

		vecRealIndex.resize(0);
	}

	if(!g_vecCharIndex.empty())
	{
		vecCharUV1.resize(g_vecCharIndex.size());
		vecCharUV2.resize(g_vecCharIndex.size());

		DWORD dwCharStart = 0;
		DWORD dwCharCount = 0;

		//push back a dummy entry
		g_vecCharIndex.push_back(-1);

		for(DWORD i = 0; i < g_vecCharIndex.size(); i++)
		{
			DWORD dwCharIndex = g_vecCharIndex[i];
			if(dwCharIndex == -1 || !g_pCurFont->WriteChar(g_vecSymbolDrawable[dwCharIndex].wCode, &g_vecSymbolDrawable[dwCharIndex].cSize, &vecCharUV1[i], &vecCharUV2[i]))	//It's impossible for the first round to fail
			{
				//flush
				LPD3DTEXTURE font_texture = g_pCurFont->Submit();
				if(font_texture)
				{
					if( g_pCurFont->GetAntialiased() )
					{
						DWORD base_vertex_index;
						LPD3DVB	vertex_buffer;
						KVertex1* v;
						for( int ix = -(int)g_pCurFont->GetBorderWidth(); ix <= (int)g_pCurFont->GetBorderWidth(); ix++ )
						{
							for( int iy = -(int)g_pCurFont->GetBorderWidth(); iy <= (int)g_pCurFont->GetBorderWidth(); iy++ )
							{
								if( ix == 0 && iy == 0 )
									continue;

								v = (KVertex1*)Gpu()->LockDynamicVertexBuffer(dwCharCount * 4, sizeof(KVertex1), &base_vertex_index, &vertex_buffer);
								if(v) 
								{
									//lock into dynamic vertex buffer
									for(DWORD base = 0, c = dwCharStart; c < dwCharStart + dwCharCount; c++)
									{
										KSymbolDrawable& symbol = g_vecSymbolDrawable[g_vecCharIndex[c]];
										D3DXVECTOR2 pos[2];
										pos[0] = D3DXVECTOR2(symbol.cPos.x - 0.5f, symbol.cPos.y - 0.5f);
										pos[1] = D3DXVECTOR2(symbol.cPos.x + symbol.cSize.x - 0.5f, symbol.cPos.y + symbol.cSize.y - 0.5f);
										pos[0].x += ix;
										pos[0].y += iy;
										pos[1].x += ix;
										pos[1].y += iy;

										//top-left
										v[base].sPos = D3DXVECTOR4(pos[0].x, pos[0].y, 0, 1);
										v[base].dwColor = symbol.dwBorderColor;
										v[base++].sUV = vecCharUV1[c];

										//top-right
										v[base].sPos = D3DXVECTOR4(pos[1].x, pos[0].y, 0, 1);
										v[base].dwColor = symbol.dwBorderColor;
										v[base++].sUV = D3DXVECTOR2(vecCharUV2[c].x, vecCharUV1[c].y);

										//bottom-left
										v[base].sPos = D3DXVECTOR4(pos[0].x, pos[1].y, 0, 1);
										v[base].dwColor = symbol.dwBorderColor;
										v[base++].sUV = D3DXVECTOR2(vecCharUV1[c].x, vecCharUV2[c].y);

										//bottom-right
										v[base].sPos = D3DXVECTOR4(pos[1].x, pos[1].y, 0, 1);
										v[base].dwColor = symbol.dwBorderColor;
										v[base++].sUV = vecCharUV2[c];
									}
									Gpu()->UnlockDynamicVertexBuffer();

									//shared dynamic states
									//stream source
									Gpu()->SetStreamSource(vertex_buffer, sizeof(KVertex1));
									//indices
									Gpu()->SetIndices(g_pTextMultiquadIndexBuffer);
									//fvf/decl
									Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
									//texture
									Gpu()->SetTexture(font_texture);
									//static states for char body
									Gpu()->SetStaticRenderStates(g_dwCharBodySrsAntialiased);
									//draw char body
									Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, base_vertex_index, 0, dwCharCount * 4, 0, dwCharCount * 2);
								}
							}
						}

						v = (KVertex1*)Gpu()->LockDynamicVertexBuffer(dwCharCount * 4, sizeof(KVertex1), &base_vertex_index, &vertex_buffer);
						if(v) 
						{
							//lock into dynamic vertex buffer
							for(DWORD base = 0, c = dwCharStart; c < dwCharStart + dwCharCount; c++)
							{
								KSymbolDrawable& symbol = g_vecSymbolDrawable[g_vecCharIndex[c]];
								D3DXVECTOR2 pos[2];
								pos[0] = D3DXVECTOR2(symbol.cPos.x - 0.5f, symbol.cPos.y - 0.5f);
								pos[1] = D3DXVECTOR2(symbol.cPos.x + symbol.cSize.x - 0.5f, symbol.cPos.y + symbol.cSize.y - 0.5f);

								//top-left
								v[base].sPos = D3DXVECTOR4(pos[0].x, pos[0].y, 0, 1);
								v[base].dwColor = symbol.dwColor;
								v[base++].sUV = vecCharUV1[c];

								//top-right
								v[base].sPos = D3DXVECTOR4(pos[1].x, pos[0].y, 0, 1);
								v[base].dwColor = symbol.dwColor;
								v[base++].sUV = D3DXVECTOR2(vecCharUV2[c].x, vecCharUV1[c].y);

								//bottom-left
								v[base].sPos = D3DXVECTOR4(pos[0].x, pos[1].y, 0, 1);
								v[base].dwColor = symbol.dwColor;
								v[base++].sUV = D3DXVECTOR2(vecCharUV1[c].x, vecCharUV2[c].y);

								//bottom-right
								v[base].sPos = D3DXVECTOR4(pos[1].x, pos[1].y, 0, 1);
								v[base].dwColor = symbol.dwColor;
								v[base++].sUV = vecCharUV2[c];
							}
							Gpu()->UnlockDynamicVertexBuffer();

							//shared dynamic states
							//stream source
							Gpu()->SetStreamSource(vertex_buffer, sizeof(KVertex1));
							//indices
							Gpu()->SetIndices(g_pTextMultiquadIndexBuffer);
							//fvf/decl
							Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
							//texture
							Gpu()->SetTexture(font_texture);
							//static states for char body
							Gpu()->SetStaticRenderStates(g_dwCharBodySrsAntialiased);
							//draw char body
							Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, base_vertex_index, 0, dwCharCount * 4, 0, dwCharCount * 2);
						}
					}
					else
					{
						if( g_bUseColor2 )
						{
							DWORD base_vertex_index;
							LPD3DVB	vertex_buffer;
							KVertex2* v = (KVertex2*)Gpu()->LockDynamicVertexBuffer(dwCharCount * 4, sizeof(KVertex2), &base_vertex_index, &vertex_buffer);
							if(v) 
							{
								//lock into dynamic vertex buffer
								for(DWORD base = 0, c = dwCharStart; c < dwCharStart + dwCharCount; c++)
								{
									KSymbolDrawable& symbol = g_vecSymbolDrawable[g_vecCharIndex[c]];
									D3DXVECTOR2 pos[2];
									pos[0] = D3DXVECTOR2(symbol.cPos.x - 0.5f, symbol.cPos.y - 0.5f);
									pos[1] = D3DXVECTOR2(symbol.cPos.x + symbol.cSize.x - 0.5f, symbol.cPos.y + symbol.cSize.y - 0.5f);

									//top-left
									v[base].sPos = D3DXVECTOR4(pos[0].x, pos[0].y, 0, 1);
									v[base].dwColor1 = symbol.dwColor;
									v[base].dwColor2 = symbol.dwBorderColor;
									v[base++].sUV = vecCharUV1[c];

									//top-right
									v[base].sPos = D3DXVECTOR4(pos[1].x, pos[0].y, 0, 1);
									v[base].dwColor1 = symbol.dwColor;
									v[base].dwColor2 = symbol.dwBorderColor;
									v[base++].sUV = D3DXVECTOR2(vecCharUV2[c].x, vecCharUV1[c].y);

									//bottom-left
									v[base].sPos = D3DXVECTOR4(pos[0].x, pos[1].y, 0, 1);
									v[base].dwColor1 = symbol.dwColor;
									v[base].dwColor2 = symbol.dwBorderColor;
									v[base++].sUV = D3DXVECTOR2(vecCharUV1[c].x, vecCharUV2[c].y);

									//bottom-right
									v[base].sPos = D3DXVECTOR4(pos[1].x, pos[1].y, 0, 1);
									v[base].dwColor1 = symbol.dwColor;
									v[base].dwColor2 = symbol.dwBorderColor;
									v[base++].sUV = vecCharUV2[c];
								}
								Gpu()->UnlockDynamicVertexBuffer();

								//shared dynamic states
								//stream source
								Gpu()->SetStreamSource(vertex_buffer, sizeof(KVertex2));
								//indices
								Gpu()->SetIndices(g_pTextMultiquadIndexBuffer);
								//fvf/decl
								Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1);
								//texture
								Gpu()->SetTexture(font_texture);
								//static states for char body
								Gpu()->SetStaticRenderStates(g_dwCharBodySrs);
								//draw char body
								Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, base_vertex_index, 0, dwCharCount * 4, 0, dwCharCount * 2);

								//static states for CHAR border
								Gpu()->SetStaticRenderStates(g_dwCharBorderSrs);

								static BOOL bFirstTime = true;
								if( bFirstTime )
								{
									DWORD passes = 0;
									BOOL ok = Gpu()->ValidateDevice(&passes);
									if( !ok || (passes > 1) )
									{
										g_bUseColor2 = false;
									}
									bFirstTime = false;
								}
								else
								{
									//draw CHAR border
									Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, base_vertex_index, 0, dwCharCount * 4, 0, dwCharCount * 2);
								}
							}
						}
						else
						{
							DWORD dwBaseVertexIndex;
							LPD3DVB	pVertexBuffer;
							KVertex1* v = (KVertex1*)Gpu()->LockDynamicVertexBuffer(dwCharCount * 4, sizeof(KVertex1), &dwBaseVertexIndex, &pVertexBuffer);
							if(v) 
							{
								//lock into dynamic vertex buffer
								for(DWORD base = 0, c = dwCharStart; c < dwCharStart + dwCharCount; c++)
								{
									KSymbolDrawable& symbol = g_vecSymbolDrawable[g_vecCharIndex[c]];
									D3DXVECTOR2 pos[2];
									pos[0] = D3DXVECTOR2(symbol.cPos.x - 0.5f, symbol.cPos.y - 0.5f);
									pos[1] = D3DXVECTOR2(symbol.cPos.x + symbol.cSize.x - 0.5f, symbol.cPos.y + symbol.cSize.y - 0.5f);

									//top-left
									v[base].sPos = D3DXVECTOR4(pos[0].x, pos[0].y, 0, 1);
									v[base].dwColor = symbol.dwColor;
									v[base++].sUV = vecCharUV1[c];

									//top-right
									v[base].sPos = D3DXVECTOR4(pos[1].x, pos[0].y, 0, 1);
									v[base].dwColor = symbol.dwColor;
									v[base++].sUV = D3DXVECTOR2(vecCharUV2[c].x, vecCharUV1[c].y);

									//bottom-left
									v[base].sPos = D3DXVECTOR4(pos[0].x, pos[1].y, 0, 1);
									v[base].dwColor = symbol.dwColor;
									v[base++].sUV = D3DXVECTOR2(vecCharUV1[c].x, vecCharUV2[c].y);

									//bottom-right
									v[base].sPos = D3DXVECTOR4(pos[1].x, pos[1].y, 0, 1);
									v[base].dwColor = symbol.dwColor;
									v[base++].sUV = vecCharUV2[c];
								}
								Gpu()->UnlockDynamicVertexBuffer();

								//shared dynamic states
								//stream source
								Gpu()->SetStreamSource(pVertexBuffer, sizeof(KVertex1));
								//indices
								Gpu()->SetIndices(g_pTextMultiquadIndexBuffer);
								//fvf/decl
								Gpu()->SetFvf(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
								//texture
								Gpu()->SetTexture(font_texture);
								//static states for char body
								Gpu()->SetStaticRenderStates(g_dwCharBodySrs);
								//draw char body
								Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, dwBaseVertexIndex, 0, dwCharCount * 4, 0, dwCharCount * 2);
							}

							v = (KVertex1*)Gpu()->LockDynamicVertexBuffer(dwCharCount * 4, sizeof(KVertex1), &dwBaseVertexIndex, &pVertexBuffer);
							if(v) 
							{
								//lock into dynamic vertex buffer
								for(DWORD base = 0, c = dwCharStart; c < dwCharStart + dwCharCount; c++)
								{
									KSymbolDrawable& symbol = g_vecSymbolDrawable[g_vecCharIndex[c]];
									D3DXVECTOR2 pos[2];
									pos[0] = D3DXVECTOR2(symbol.cPos.x - 0.5f, symbol.cPos.y - 0.5f);
									pos[1] = D3DXVECTOR2(symbol.cPos.x + symbol.cSize.x - 0.5f, symbol.cPos.y + symbol.cSize.y - 0.5f);

									//top-left
									v[base].sPos = D3DXVECTOR4(pos[0].x, pos[0].y, 0, 1);
									v[base].dwColor = symbol.dwBorderColor;
									v[base++].sUV = vecCharUV1[c];

									//top-right
									v[base].sPos = D3DXVECTOR4(pos[1].x, pos[0].y, 0, 1);
									v[base].dwColor = symbol.dwBorderColor;
									v[base++].sUV = D3DXVECTOR2(vecCharUV2[c].x, vecCharUV1[c].y);

									//bottom-left
									v[base].sPos = D3DXVECTOR4(pos[0].x, pos[1].y, 0, 1);
									v[base].dwColor = symbol.dwBorderColor;
									v[base++].sUV = D3DXVECTOR2(vecCharUV1[c].x, vecCharUV2[c].y);

									//bottom-right
									v[base].sPos = D3DXVECTOR4(pos[1].x, pos[1].y, 0, 1);
									v[base].dwColor = symbol.dwBorderColor;
									v[base++].sUV = vecCharUV2[c];
								}
								Gpu()->UnlockDynamicVertexBuffer();
								Gpu()->SetStreamSource(pVertexBuffer, sizeof(KVertex1));
								Gpu()->SetStaticRenderStates(g_dwCharBorderSrsEx);
								//draw char border
								Gpu()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, dwBaseVertexIndex, 0, dwCharCount * 4, 0, dwCharCount * 2);
							}
						}
					}
				}

				if(dwCharIndex != -1)
				{
					//batch
					g_pCurFont->WriteChar(g_vecSymbolDrawable[dwCharIndex].wCode, &g_vecSymbolDrawable[dwCharIndex].cSize, &vecCharUV1[i], &vecCharUV2[i]);
					dwCharStart += dwCharCount;
					dwCharCount = 1;
				}
			}
			else
			{
				//batch
				dwCharCount++;
			}
		}

		vecCharUV1.resize(0);
		vecCharUV2.resize(0);

		//clear index buffer
		g_vecCharIndex.clear();
	}
}

void FlushTextIconBuf()
{
	static vector<DWORD>	vecRealIndex;

	if( g_bScissorTestEnable )
	{
		for( DWORD i = 0; i < g_vecIconIndex.size(); i++ )
		{
			KSymbolDrawable& symbol = g_vecSymbolDrawable[ g_vecIconIndex[i] ];
			if( Gpu()->GetHardwareCaps()->bScissorTestAvaiable )
			{
				//intersect
				if( min( symbol.cPos.x + symbol.cSize.x, g_sScissorRect.right ) > max( symbol.cPos.x, g_sScissorRect.left ) &&
					min( symbol.cPos.y + symbol.cSize.y, g_sScissorRect.bottom ) > max( symbol.cPos.y, g_sScissorRect.top ) )
				{
					vecRealIndex.push_back( g_vecIconIndex[i] );
				}
			}
			else
			{
				//contained in
				if( symbol.cPos.x >= g_sScissorRect.left && symbol.cPos.y >= g_sScissorRect.top &&
					symbol.cPos.x + symbol.cSize.x <= g_sScissorRect.right && symbol.cPos.y + symbol.cSize.y <= g_sScissorRect.bottom )
				{
					vecRealIndex.push_back( g_vecIconIndex[i] );
				}
			}
		}

		if( vecRealIndex.size() != g_vecIconIndex.size() )
		{
			g_vecIconIndex.resize( vecRealIndex.size() );
			for( DWORD i = 0; i < vecRealIndex.size(); i++ )
			{
				g_vecIconIndex[i] = vecRealIndex[i];
			}
		}

		vecRealIndex.resize(0);
	}

	if(!g_vecIconIndex.empty())
	{
		//sort
		g_pIconSortVecBase = &g_vecSymbolDrawable[0];
		qsort(&g_vecIconIndex[0], g_vecIconIndex.size(), sizeof(DWORD), IconSortCmpFunc);

		//patch to avoid recursive batching
		DWORD dwCurSubSaved = g_dwCurSub;
		g_dwCurSub = 0;

		//draw
		for(DWORD i = 0; i < g_vecIconIndex.size(); i++)
		{
			KSymbolDrawable* pIconInfo = &g_vecSymbolDrawable[g_vecIconIndex[i]];

			g_bForceDrawPic = true;
			JXTextRender::GetCtrlHandleW(pIconInfo->byCtrlId)->RepresentAction(pIconInfo->wCode, pIconInfo->cPos.x, pIconInfo->cPos.y);
			g_bForceDrawPic = false;
		}

		//patch to avoid recursive batching
		ProcessCmd(0, 0, 0);
		g_dwCurSub = dwCurSubSaved;

		//clear index buffer
		g_vecIconIndex.resize(0);
	}
}

void FlushTextBuf()
{
	if( g_pCurFont )
	{
		if( !g_vecSymbolDrawable.empty() )
		{
			FlushTextCharBuf();
			FlushTextIconBuf();

			//clear buffer
			g_vecSymbolDrawable.resize(0);
			g_vecIsIcon.resize(0);

		}
		g_pCurFont = 0;
	}
}

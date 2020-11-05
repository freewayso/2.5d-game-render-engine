/* -------------------------------------------------------------------------
//	文件名		：	kernel_text.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	文本的绘制处理
//
// -----------------------------------------------------------------------*/

#include "kernel_text.h"
#include "font_media.h"
#include "global_config.h"
#include "blender.h"
#include "drawer.h"

BOOL g_bScissorTestEnable = FALSE;
RECT g_sScissorRect;
BOOL g_bForceDrawPic = FALSE;
RECT g_sClipOverwriteRect;
BOOL g_bClipOverwrite = FALSE;

//font & text

void WriteText( BOOL bRGB565, KTextCmd* pCmd, LPDIRECTDRAWSURFACE pCanvas )
{
	//buf
	static vector<KSymbolDrawable>	s_vecSymbolDrawable;
	static vector<BOOL>				s_vecIsIcon;
	static vector<DWORD>			s_vecCharIndex;
	static vector<DWORD>			s_vecIconIndex;
	KFont*							pCurFont = 0;

	//typeset
	KTextCmd* pIn = (KTextCmd*)pCmd;

	pCurFont = 0;
	if( !QueryProduct(pIn->dwFontHandle, FALSE, (void**)&pCurFont) || !pCurFont )
		return;

	static vector<WORD>	s_vecColorStack;
	static vector<WORD>	s_vecBorderColorStack;

	KPOINT					cPen = pIn->cPos;				//position of the cPen
	cPen.y -= pIn->dwStartLine * pIn->nLineInterSpace;	//patch y
	DWORD					dwLineIndex = 0;					//current line index
	DWORD					dwBegin, dwEnd;					//'dwBegin' point to the first CHAR in current line, 'dwEnd' point to the one next to the last CHAR in current line
	dwBegin = dwEnd = (DWORD)s_vecSymbolDrawable.size();
	BOOL					bNewLine = TRUE;
	BOOL					bUnderLine = FALSE;

	//push the initial color/border-color
	s_vecColorStack.push_back( Color32To16(pIn->dwColor, bRGB565) );
	s_vecBorderColorStack.push_back( Color32To16(pIn->dwBorderColor, bRGB565) );

	BOOL bCheckLineBreaking = TRUE;
	BOOL bSupportWholeWordWrap = JXTextRender::IsWholeWordWrap();
	INT nCurLineSpaceCount = 0;	//统计一下当前行的空格个数

	DWORD dwCursor = 0;
	while(dwCursor < pIn->dwBytes)
	{
		switch(pIn->pwchText[dwCursor])
		{
		case 0:
			dwCursor = pIn->dwBytes;
			break;
		case 0x02:
			if(pIn->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(dwCursor + 3 < pIn->dwBytes)
				{
					s_vecColorStack.push_back( Color32To16(D3DCOLOR_ARGB(0xff, pIn->pwchText[dwCursor+1], pIn->pwchText[dwCursor+2], pIn->pwchText[dwCursor+3]), bRGB565) );
					dwCursor += 4;
				}
				else
				{
					//parsing done
					dwCursor = pIn->dwBytes;
					break;
				}
				break;
			}
		case 0x03:
			if(pIn->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(s_vecColorStack.size() > 1) s_vecColorStack.pop_back(); 
				dwCursor++;
				break;
			}
		case 0x04:
			if(pIn->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(dwCursor + 3 < pIn->dwBytes)
				{
					s_vecBorderColorStack.push_back( Color32To16(D3DCOLOR_ARGB(0xff, pIn->pwchText[dwCursor+1], pIn->pwchText[dwCursor+2], pIn->pwchText[dwCursor+3]), bRGB565) );
					dwCursor += 4;
				}
				else
				{
					//parsing done
					dwCursor = pIn->dwBytes;
					break;
				}
				break;
			}
		case 0x05:
			if(pIn->dwOptions & emTEXT_CMD_COLOR_TAG)
			{
				if(s_vecBorderColorStack.size() > 1) s_vecBorderColorStack.pop_back();
				dwCursor++;
				break;
			}
		case 0x10:
			if(pIn->dwOptions & emTEXT_CMD_UNDERLINE_TAG)
			{
				bUnderLine = !bUnderLine;
				dwCursor++;
				break;
			}
		case 0x0A:	//newline
			{
				dwBegin = dwEnd;
				cPen.x = pIn->cPos.x;
				cPen.y += pIn->nLineInterSpace;
				bNewLine = TRUE;
				if(++dwLineIndex == pIn->dwEndLine)
				{
					//parsing done
					dwCursor = pIn->dwBytes;
					break;
				}
				dwCursor++;
			}
			break;
		default:
			{
				//get code, check whether it is an icon(inline picture)
				WORD	wCode; 
				BOOL	bIcon;
				BYTE	byCtrlId = 0;
				if( (pIn->dwOptions & emTEXT_CMD_INLINE_PICTURE) && JXTextRender::GetCtrlHandleW(pIn->pwchText[dwCursor]) )	//icon
				{
					if(dwCursor + 1 < pIn->dwBytes) 
					{
						byCtrlId = (BYTE)pIn->pwchText[dwCursor];
						wCode = *((WORD*)(pIn->pwchText + dwCursor + 1));
						bIcon = TRUE;
						dwCursor += 2;

					}
					else
					{
						//parsing done
						dwCursor = pIn->dwBytes;
						break;
					}
				}
				//		else if( _IsDBCSLeadingByte(in->text[cursor]) )	//DOUBLE byte character
				//		{
				//			code = in->text[cursor]|(in->text[cursor+1]<<8);
				//			icon = FALSE;
				//			cursor += 2;
				//		}
				else if(pIn->pwchText[dwCursor] >= 0x20)	//single byte character (including white-space)
				{					
					wCode = pIn->pwchText[dwCursor];
					bIcon = FALSE;
					dwCursor++;

					//统计当前行的空格个数
					if (wCode == 0x20)
						nCurLineSpaceCount++;
				}
				else	//non-printable CHAR
				{
					dwCursor++;
					break;
				}

				if( g_cGlobalConfig.m_bWordWrap && !bIcon && wCode == 0x20 && !bNewLine && dwBegin==dwEnd )	//skip leading blank of broken line
					break;

				//get size
				KPOINT cSize;
				if(bIcon)
				{
					if( !SUCCEEDED(JXTextRender::GetCtrlHandleW(byCtrlId)->GetSize(wCode, pIn->nCharInterSpace, cSize.x, cSize.y)) )
						break;

					//apply scaling
					cSize.x = (INT)(cSize.x * pIn->fInlinePicScaling);
					cSize.y = (INT)(cSize.y * pIn->fInlinePicScaling);
				}
				else
				{
					cSize.x = pCurFont->GetCharWidth(wCode);
					cSize.y = pCurFont->GetCharHeight();
				}

				//put clipping code here

				//write the CHAR virtually
				dwEnd++;
				s_vecSymbolDrawable.resize(dwEnd);
				s_vecIsIcon.resize(dwEnd);

				s_vecSymbolDrawable[dwEnd-1].bUnderLine = bUnderLine;
				s_vecSymbolDrawable[dwEnd-1].wCode = wCode;
				s_vecSymbolDrawable[dwEnd-1].byCtrlId = byCtrlId;
				s_vecSymbolDrawable[dwEnd-1].cPos.x = cPen.x;
				s_vecSymbolDrawable[dwEnd-1].cPos.y = bIcon ? (cPen.y - ((INT)cSize.y - (INT)pCurFont->GetCharHeight())/2) : cPen.y;
				s_vecSymbolDrawable[dwEnd-1].cSize = cSize;
				s_vecSymbolDrawable[dwEnd-1].dwColor = s_vecColorStack.back();
				s_vecSymbolDrawable[dwEnd-1].dwBorderColor = s_vecBorderColorStack.back();
				s_vecIsIcon[dwEnd-1] = bIcon;

				//add to the index buffer temporary
				if(dwLineIndex >= pIn->dwStartLine && dwLineIndex < pIn->dwEndLine)
				{
					if(bIcon)
					{
						s_vecIconIndex.push_back(dwEnd-1);
					}
					else if (wCode != 0x20)
					{
						s_vecCharIndex.push_back(dwEnd-1);
					}
				}

				//move cPen
				if(!bIcon)
				{
					//	if( _IsDBCSDoubleByteChar( code ) )
					//		cPen.x += in->m_nCharInterSpace;
					//	else
					//		cPen.x += in->char_interspace/2;
					cPen.x += JXTextRender::GetTextWidth((const WCHAR*)&wCode, 1, pIn->nCharInterSpace);
				}
				else
				{
					INT w = pIn->nCharInterSpace/2;
					while(w< cSize.x)
					{
						w += pIn->nCharInterSpace/2;
					}
					cPen.x += w;
				}

				if (bSupportWholeWordWrap)
				{
					//要不等于空格/图，或一个空格/图都没有，要不到最后了
					bCheckLineBreaking = (wCode == 0x20 || bIcon == TRUE) || (nCurLineSpaceCount == 0) || (dwCursor == pIn->dwBytes);
				}	

				//check for line breaking
				while(cPen.x - pIn->cPos.x > pIn->nLineWidthLimit && bCheckLineBreaking)
				{
					//find the break point
					DWORD e1  = 0;
					DWORD e_t = 0;		  //支持整词换行的，断行的位置

					if( !g_cGlobalConfig.m_bWordWrap )
					{					
						e1 = dwEnd - 1;		//e1 will be next of the tail CHAR of the broken line

						//modify by wdb						
						//保证至少有1个词在第一行	????
						//	if(e1 <= dwBegin) 
						//		e1 = dwBegin + 1;

						e_t = e1;
					}
					else
					{
						DWORD i;
						for( i = dwEnd-1; i > dwBegin; i-- )
						{
							//	if( !(!is_icon_vec[i] && !_IsDBCSDoubleByteChar(symbol_drawable_vec[i].m_wCode) && symbol_drawable_vec[i].m_wCode!=0x20 && !is_icon_vec[i-1] && !_IsDBCSDoubleByteChar(symbol_drawable_vec[i-1].m_wCode) && symbol_drawable_vec[i-1].m_wCode!=0x20) )
							if( !(!s_vecIsIcon[i] && s_vecSymbolDrawable[i].wCode!=0x20 && !s_vecIsIcon[i-1] && s_vecSymbolDrawable[i-1].wCode!=0x20) )
							{
								e1 = i;
								break;
							}
						}
						if( i==dwBegin )
						{
							e1 = dwEnd - 1;
							if(e1 <= dwBegin) 
								e1 = dwBegin + 1;
						}
					}

					//start text justification here, 是否支持整词换行, 如果是，则做处理, 或者本行没有空格或图
					if( !bSupportWholeWordWrap || nCurLineSpaceCount == 0)
						goto after_text_justification;

					DWORD b0 = 0; //本行非空格开始位置，结束位置
					DWORD e0 = 0;
					{						
						DWORD i;
						for( i = dwBegin; i < e1; i++ )
						{
							if( s_vecIsIcon[i] || s_vecSymbolDrawable[i].wCode!=0x20 )
							{
								b0 = i;
								break;
							}
						}
						if( i >= e1 )
							goto after_text_justification;						
					}					

					for(INT i = e1; i >= (INT)dwBegin; i--)
					{
						if( s_vecIsIcon[i] || s_vecSymbolDrawable[i].wCode!=0x20 )
						{
							e0 = i;
							break;
						}
					}	

					//modify by wdb	
					for (INT i = e0 - 1; i >= (INT)dwBegin; i--)
					{
						if (s_vecIsIcon[i] || s_vecSymbolDrawable[i].wCode == 0x20)
						{
							if (i > 0) //到头，表示只有1个词
							{
								e_t = i + 1;
							}
							ASSERT(e_t >= 2);
							break;
						}
					}

					INT nBreakCount = 0;
					for( DWORD i = b0+1; i <= e0; i++ )
					{
						if (!s_vecIsIcon[i] && s_vecSymbolDrawable[i-1].wCode==0x20)
							nBreakCount++;
						//	if (is_icon_vec[i-1] || is_icon_vec[i] || _IsDBCSDoubleByteChar(symbol_drawable_vec[i-1].m_wCode) || _IsDBCSDoubleByteChar(symbol_drawable_vec[i].m_wCode))
						if (s_vecIsIcon[i-1] || s_vecIsIcon[i])
							nBreakCount++;
					}
					if(nBreakCount==0)
						goto after_text_justification;

					INT nExtSpace = pIn->nLineWidthLimit + pIn->cPos.x - (s_vecSymbolDrawable[e0].cPos.x + s_vecSymbolDrawable[e0].cSize.x);
					INT n = nExtSpace/nBreakCount;
					INT t = nExtSpace%nBreakCount;
					//modify by wdb, 和text.dll判断手法保持一致
					//	if (n >= -1)//判断留在本行
					if (((pIn->nLineWidthLimit - (cPen.x - pIn->cPos.x)) / nBreakCount) >= -1 && pIn->nFontId > 12)
					{
						e_t = e1 + 1;
					}
					else	   //放在下一行
					{
						e0 = e_t - 2;
						nExtSpace = pIn->nLineWidthLimit + pIn->cPos.x - (s_vecSymbolDrawable[e0].cPos.x + s_vecSymbolDrawable[e0].cSize.x);
						n = nExtSpace/nBreakCount;
						t = nExtSpace%nBreakCount;						
					}
					INT sum_break_count = 0;
					INT sum_offset = 0;
					for (DWORD i = b0+1; i <= e0; i++)
					{
						INT delta_break_count = 0;
						INT delta_offset = 0;
						if (!s_vecIsIcon[i] && s_vecSymbolDrawable[i-1].wCode==0x20)
							delta_break_count++;
						//	if (is_icon_vec[i-1] || is_icon_vec[i] || _IsDBCSDoubleByteChar(symbol_drawable_vec[i-1].m_wCode) || _IsDBCSDoubleByteChar(symbol_drawable_vec[i].m_wCode))
						if (s_vecIsIcon[i-1] || s_vecIsIcon[i])
							delta_break_count++;
						for (INT k = sum_break_count; k < sum_break_count + delta_break_count; k++)
						{
							if (t>0 && k % (nBreakCount/t) == 0 && k < t*(nBreakCount/t))
								delta_offset += (n+1);
							else 
								delta_offset += n;
						}
						sum_break_count += delta_break_count;
						sum_offset += delta_offset;
						s_vecSymbolDrawable[i].cPos.x += sum_offset;
					}
					//end text justification
after_text_justification:

					//break line
					if( !g_cGlobalConfig.m_bWordWrap )
						//	dwBegin = e1;
						dwBegin = e_t;
					else
					{
						DWORD i;
						for( i = e1; i < dwEnd; i++ )
						{
							if( s_vecIsIcon[i] || s_vecSymbolDrawable[i].wCode!=0x20 )
							{
								dwBegin = i;
								break;
							}
						}
						if( i==dwEnd )
							dwBegin = dwEnd;
					}

					//move to next line
					if(dwBegin < dwEnd)
					{
						INT offset = s_vecSymbolDrawable[dwBegin].cPos.x - pIn->cPos.x;
						for(DWORD i = dwBegin; i < dwEnd; i++)
						{
							s_vecSymbolDrawable[i].cPos.x -= offset;
							s_vecSymbolDrawable[i].cPos.y += pIn->nLineInterSpace;
						}
						cPen.x -= offset;
					}
					else
					{
						cPen.x = pIn->cPos.x;
					}
					cPen.y += pIn->nLineInterSpace;

					//inc dwLineIndex
					dwLineIndex++;

					bNewLine = FALSE;

					nCurLineSpaceCount = 0;
					//check the line index
					if(dwLineIndex == pIn->dwStartLine)	//enter the scope
					{
						for(DWORD i = dwBegin; i < dwEnd; i++)
						{
							if(s_vecIsIcon[i])
							{
								s_vecIconIndex.push_back(i);
							}
							else if(s_vecSymbolDrawable[i].wCode != 0x20)
							{
								s_vecCharIndex.push_back(i);
							}
						}
					}
					else if(dwLineIndex == pIn->dwEndLine)	//exit the scope
					{
						for(DWORD i = dwBegin; i < dwEnd; i++)
						{
							if(s_vecIsIcon[i])
							{
								s_vecIconIndex.pop_back();
							}
							else if(s_vecSymbolDrawable[i].wCode != 0x20)
							{
								s_vecCharIndex.pop_back();
							}
						}

						//parsing done
						dwCursor = pIn->dwBytes;
						break;
					}
				}
			}
		}
	}

	//clear color stacks
	s_vecColorStack.resize(0);
	s_vecBorderColorStack.resize(0);

	//flush
	if( pCurFont )
	{
		if( !s_vecSymbolDrawable.empty() )
		{
			//draw the chars first
			if( !s_vecCharIndex.empty() )
			{
				//Lock( canvas );	//游戏循环外部调用时锁定
				DDSURFACEDESC sDesc;
				sDesc.dwSize = sizeof(sDesc);
				if ( pCanvas->Lock( NULL, &sDesc, DDLOCK_WAIT, NULL ) == DD_OK )
				{
					void* real_canvas_ptr = g_bScissorTestEnable ? (BYTE*)sDesc.lpSurface + g_sScissorRect.top * sDesc.lPitch + 2*g_sScissorRect.left : sDesc.lpSurface;
					DWORD real_canvas_width = g_bScissorTestEnable ? g_sScissorRect.right - g_sScissorRect.left : sDesc.dwWidth;
					DWORD real_canvas_height = g_bScissorTestEnable ? g_sScissorRect.bottom - g_sScissorRect.top : sDesc.dwHeight;

					for( DWORD i = 0; i < s_vecCharIndex.size(); i++ )
					{
						KSymbolDrawable& sSymbol = s_vecSymbolDrawable[s_vecCharIndex[i]];
						if( pCurFont->IsSysFont() )
						{
							if( pCurFont->GetAntialiased() )
							{
								for( INT ix = -(INT)pCurFont->GetBorderWidth(); ix <= (INT)pCurFont->GetBorderWidth(); ix++ )
								{
									for( INT iy = -(INT)pCurFont->GetBorderWidth(); iy <= (INT)pCurFont->GetBorderWidth(); iy++ )
									{
										if( ix == 0 && iy == 0 )
											continue;

										DrawSysCharAntialiased(bRGB565, sSymbol.dwBorderColor, real_canvas_ptr, real_canvas_width, real_canvas_height, sDesc.lPitch, (g_bScissorTestEnable ? sSymbol.cPos.x - g_sScissorRect.left : sSymbol.cPos.x) + ix, (g_bScissorTestEnable ? sSymbol.cPos.y - g_sScissorRect.top : sSymbol.cPos.y) + iy, pCurFont->GetCharData(sSymbol.wCode), sSymbol.cSize.x, sSymbol.cSize.y);
									}
								}

								DrawSysCharAntialiased(bRGB565, sSymbol.dwColor, real_canvas_ptr, real_canvas_width, real_canvas_height, sDesc.lPitch, (g_bScissorTestEnable ? sSymbol.cPos.x - g_sScissorRect.left : sSymbol.cPos.x), (g_bScissorTestEnable ? sSymbol.cPos.y - g_sScissorRect.top : sSymbol.cPos.y), pCurFont->GetCharData(sSymbol.wCode), sSymbol.cSize.x, sSymbol.cSize.y);
							}
							else
							{
								DrawSysCharWithBorder(sSymbol.dwColor, sSymbol.dwBorderColor, real_canvas_ptr, real_canvas_width, real_canvas_height, sDesc.lPitch, g_bScissorTestEnable ? sSymbol.cPos.x - g_sScissorRect.left : sSymbol.cPos.x, g_bScissorTestEnable ? sSymbol.cPos.y - g_sScissorRect.top : sSymbol.cPos.y, pCurFont->GetCharData(sSymbol.wCode), sSymbol.cSize.x, sSymbol.cSize.y);
							}
						}
						else
						{
							DrawCharWithBorder(bRGB565 ?  0x07e0f81f : 0x03e07c1f, sSymbol.dwColor, sSymbol.dwBorderColor, real_canvas_ptr, real_canvas_width, real_canvas_height, sDesc.lPitch, g_bScissorTestEnable ? sSymbol.cPos.x - g_sScissorRect.left : sSymbol.cPos.x, g_bScissorTestEnable ? sSymbol.cPos.y - g_sScissorRect.top : sSymbol.cPos.y, pCurFont->GetCharData(sSymbol.wCode), sSymbol.cSize.x, sSymbol.cSize.y);
						}

						if( sSymbol.bUnderLine )
						{
							if( g_bScissorTestEnable )
							{
								DrawLine(real_canvas_ptr, real_canvas_width, real_canvas_height, sDesc.lPitch, sSymbol.cPos.x - g_sScissorRect.left, sSymbol.cPos.y + sSymbol.cSize.y - g_sScissorRect.top, sSymbol.cPos.x + sSymbol.cSize.x - g_sScissorRect.left, sSymbol.cPos.y + sSymbol.cSize.y - g_sScissorRect.top, sSymbol.dwColor);
							}
							else
							{
								DrawLine(sDesc.lpSurface, sDesc.dwWidth, sDesc.dwHeight, sDesc.lPitch, sSymbol.cPos.x, sSymbol.cPos.y + sSymbol.cSize.y, sSymbol.cPos.x + sSymbol.cSize.x, sSymbol.cPos.y + sSymbol.cSize.y, sSymbol.dwColor);
							}
						}
					}

					pCanvas->Unlock( NULL );
				}

				s_vecCharIndex.resize(0);
			}

			//then draw the icons
			if( !s_vecIconIndex.empty() )
			{
				for( DWORD i = 0; i < s_vecIconIndex.size(); i++ )
				{
					KSymbolDrawable& symbol = s_vecSymbolDrawable[s_vecIconIndex[i]];
					if( g_bScissorTestEnable )
					{
						RECT sSymbol;
						SetRect(&sSymbol, 0, 0, symbol.cSize.x, symbol.cSize.y);
						RECT sClip;
						SetRect(&sClip, g_sScissorRect.left - symbol.cPos.x, g_sScissorRect.top - symbol.cPos.y, g_sScissorRect.right - symbol.cPos.x, g_sScissorRect.bottom - symbol.cPos.y);
						g_sClipOverwriteRect;
						if( !RectIntersect( &sSymbol, &sClip, &g_sClipOverwriteRect ) )
							continue;
						g_bClipOverwrite = TRUE;
					}
					g_bForceDrawPic = TRUE;
					JXTextRender::GetCtrlHandleW(symbol.byCtrlId)->RepresentAction(symbol.wCode, symbol.cPos.x, symbol.cPos.y);
					g_bForceDrawPic = FALSE;
					g_bClipOverwrite = FALSE;
				}

				s_vecIconIndex.resize(0);
			}

			//clear buffer
			s_vecSymbolDrawable.resize(0);
			s_vecIsIcon.resize(0);

		}
		pCurFont = 0;
	}
}


BOOL CheckFont(LPCSTR pszTypeFace, DWORD dwCharSet, HWND hWnd)
{
	//create font
	LOGFONT sFontInfo;
	memset(&sFontInfo, 0, sizeof(sFontInfo));
	strcpy(sFontInfo.lfFaceName, pszTypeFace);
	sFontInfo.lfCharSet = (BYTE)dwCharSet;
	HFONT hfont = CreateFontIndirect(&sFontInfo);
	if(!hfont)
		return FALSE;

	//create memdc
	HDC dc = GetDC(hWnd);
	HFONT hFontOld = (HFONT)SelectObject(dc, hfont);
	char real_typeface[256];
	GetTextFace(dc, 255, real_typeface);
	SelectObject(dc, hFontOld);
	ReleaseDC(hWnd, dc);
	DeleteObject(hfont);
	return (strcmp( pszTypeFace, real_typeface ) == 0);
}

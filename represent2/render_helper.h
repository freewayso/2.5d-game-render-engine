/* -------------------------------------------------------------------------
//	文件名		：	render_helper.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-26 11:11:11
//	功能描述	：	绘制器的通用函数和宏定义
//
// -----------------------------------------------------------------------*/
#ifndef __RENDER_HELPER_H__
#define __RENDER_HELPER_H__
#include "../represent_common/common.h"

extern WORD*	g_pPal;
extern BYTE*	g_pSection;
extern INT		g_nIndex;

inline void _Jmp(INT n)
{
	while( g_nIndex + n >= *g_pSection )
	{
		BYTE length = *g_pSection++;
		if( *g_pSection++ )
			g_pSection += length;
		n -= (length - g_nIndex);
		g_nIndex = 0;
	}
	g_nIndex += n;
}

inline BYTE _Alpha()
{
	return g_pSection[1];
}

inline WORD _Color()	//ignore if _alpha() == 0
{
	return g_pPal[g_pSection[g_nIndex+2]];
}

#define KD_IGNORE_ALPHA		10
#define KD_ADD_ALPHA		245
#define KD_IGNORE_ALPHA32	5
#define KD_ADD_ALPHA32		245

#define		KD_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX		    \
{	movzx	eax, BYTE ptr[esi]	}	    \
{	movzx	ebx, BYTE ptr[esi + 1]	}	    \
{	add		esi, 2   			}


#define		KD_COPY_PIXEL_USE_EAX	/*ebx指向调色板*/	\
{	xor	    eax, eax            }	\
{	mov	    al, BYTE ptr [esi]	}	\
{	add		esi, 1 				}	\
{	mov		ax, [ebx + eax * 2]	}	\
{	add		edi, 2				}   \
{	mov		[edi - 2], ax			}

#define		KD_COPY_4PIXEL_USE_EAX	/*ebx指向调色板*/	\
{	movd    mm4, [esi]	        } 	\
{	add		esi, 4 				}	\
{   movd    eax, mm4            }   \
{   and     eax, 0xff           }   \
{	mov		ax, [ebx + eax * 2]	}	\
{	add		edi, 2				}   \
{	mov		[edi - 2], ax		}   \
{   movd    eax, mm4            }   \
{   shr     eax, 8              }   \
{   and     eax, 0xff           }   \
{	mov		ax, [ebx + eax * 2]	}	\
{	add		edi, 2				}   \
{	mov		[edi - 2], ax		}   \
{   movd    eax, mm4            }   \
{   shr     eax, 16             }   \
{   and     eax, 0xff           }   \
{	mov		ax, [ebx + eax * 2]	}	\
{	add		edi, 2				}   \
{	mov		[edi - 2], ax		}   \
{   movd    eax, mm4            }   \
{   shr     eax, 24             }   \
{   and     eax, 0xff           }   \
{	mov		ax, [ebx + eax * 2]	}	\
{	add		edi, 2				}   \
{	mov		[edi - 2], ax		}



#define		KD_MIX_2_PIXEL_COLOR_USE_EABDX									\
{	movd	mm7, ecx			}						\
{   xor     eax, eax            }                       \
{	movd    ebx, mm0    		}	/* pPalette */		\
{	mov	    al, BYTE ptr[esi]	}						\
{	inc		esi					}						\
{	mov     dx, [ebx + eax * 2]	}	/*edx = ...rgb*/	\
{	movd	ecx, mm2    		}	/* nMask32 */		\
{	mov		ax, dx				}	/*eax = ...rgb*/	\
{	shl		eax, 16				}	/*eax = rgb...*/	\
{	mov		ax, dx				}	/*eax = rgbrgb*/	\
{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
{	and		eax, ecx			}	/*eax = .g.r.b*/	\
{	mov		bx, dx				}	/*ebx = ...rgb*/	\
{	shl		ebx, 16				}	/*ebx = rgb...*/	\
{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
{   lea     edx, [ebx + ebx * 2]}                       \
{   add     eax, edx            }                       \
{	shr		eax, 2				}	/*c = (3xc1+c2)/4*/	\
{	and     eax, ecx			}	/*eax = .g.r.b*/	\
{	mov     dx, ax				}	/*edx = ...r.b*/	\
{	shr     eax, 16				}	/*eax = ....g.*/	\
{	add 	edi, 2				}						\
{	or      ax, dx				}	/*eax = ...rgb*/	\
{	movd     ecx, mm7			}                       \
{	mov		[edi - 2], ax		}

#define		KD_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX							\
{	movd	mm7, ecx    		}						\
{	xor	    eax, eax    		}						\
{	movd    ebx, mm0    		}	/* pPalette */		\
{	mov  	al, BYTE ptr[esi]	}						\
{	inc		esi					}						\
{	mov     dx, [ebx + eax * 2]	}	/*edx = ...rgb*/	\
{	movd	ecx, mm2    		}   /* nMask32 */		\
{	mov		ax, dx				}	/*eax = ...rgb*/	\
{	shl		eax, 16				}	/*eax = rgb...*/	\
{	mov		ax, dx				}	/*eax = rgbrgb*/	\
{	and		eax, ecx			}	/*eax = .g.r.b*/	\
{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
{	mov		bx, dx				}	/*ebx = ...rgb*/	\
{	shl		ebx, 16				}	/*ebx = rgb...*/	\
{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
{	movd	edx, mm3			}	/*	nAlpha    */	\
{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
{   sub     eax, ebx            }   /*eax = c1 - c2*/   \
{	imul    eax, edx			}	/*eax = (c1 - c2)*nAlpha */	\
{	shr		eax, 5				}	/*c=(c1 - c2)*nAlpha/32*/ \
{   add     eax, ebx            }   /*c=(c1 - c2)*nAlpha/32 + c2*/ \
{	and     eax, ecx			}	/*eax = .g.r.b*/	\
{	mov     dx, ax				}	/*edx = ...r.b*/	\
{	shr     eax, 16				}	/*eax = ....g.*/	\
{	add		edi, 2				}						\
{	or      ax, dx				}	/*eax = ...rgb*/	\
{	movd    ecx, mm7    		}                       \
{	mov		[edi - 2], ax		}



//读取alpha数据到ebx，读取跳点距离到eax
#define		KD_RIO_READ_ALPHA_2_EBX_RUN_LENGTH_2_EAX		\
{	xor		eax, eax			}	\
{	xor		ebx, ebx			}	\
{	mov		al,	 BYTE ptr[esi]	}	\
{	inc		esi					}	\
{	mov		bl,  BYTE ptr[esi]	}	\
{	inc		esi					}
//从调色板复制像点颜色到eax
#define		KD_RIO_COPY_PIXEL_USE_EAX	/*ebx指向调色板*/	\
{	movzx	eax, BYTE ptr[esi]	}	\
{	add		eax, eax			}	\
{	inc		esi					}	\
{	mov		ax, [ebx + eax]		}	\
{	mov		[edi], ax			}	\
{	inc		edi					}	\
{	inc		edi					}
//从调色板复制像点颜色到eax 原颜色格式565目标颜色格式555版本
#define		KD_RIO_COPY_PIXEL_USE_EAX_1555/*ebx指向调色板*/\
{	push	ecx					}	\
{	movzx	eax, BYTE ptr[esi]	}	\
{	add		eax, eax			}	\
{	inc		esi					}	\
{	mov		ax, [ebx + eax]		}	\
{	mov		cx, ax				}	\
{	shr		ax, 6				}	/*eax = ....rg*/	\
{	shl		eax, 16				}	/*eax = .rg...*/	\
{	mov		ax, 0x1f			}	/*eax = .rg..m(m=mask)*/	\
{	and		ax, cx				}	/*eax = .rg..b*/	\
{	shl		ax, 11				}	/*eax = .rgb..*/	\
{	shr		eax,11				}	/*eax = ...rgb*/	\
{	mov		[edi], ax			}	\
{	inc		edi					}	\
{	inc		edi					}	\
{	pop		ecx					}
//固定alpha值合两个颜色，运算使用了eax,ebx,edx
#define		KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX									\
{	push	ecx					}						\
{	mov     ebx, pPalette		}						\
{	movzx	eax, BYTE ptr[esi]	}						\
{	mov		ecx, dwRGBBitMask32}						\
{	add		eax, eax			}						\
{	inc		esi					}						\
{	mov     dx, [ebx + eax]		}	/*edx = ...rgb*/	\
{	mov		ax, dx				}	/*eax = ...rgb*/	\
{	shl		eax, 16				}	/*eax = rgb...*/	\
{	mov		ax, dx				}	/*eax = rgbrgb*/	\
{	and		eax, ecx			}	/*eax = .g.r.b*/	\
{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
{	mov		bx, dx				}	/*ebx = ...rgb*/	\
{	shl		ebx, 16				}	/*ebx = rgb...*/	\
{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
{	add		eax, ebx			}						\
{	add		ebx, ebx			}						\
{	add		eax, ebx			}						\
{	shr		eax, 2				}	/*c = (3xc1+c2)/4*/	\
{	and     eax, ecx			}	/*eax = .g.r.b*/	\
{	mov     dx, ax				}	/*edx = ...r.b*/	\
{	shr     eax, 16				}	/*eax = ....g.*/	\
{	or      ax, dx				}	/*eax = ...rgb*/	\
{	mov		[edi], ax			}						\
{	inc		edi					}						\
{	inc		edi					}						\
{	pop		ecx					}
//固定alpha值合两个颜色，运算使用了eax,ebx,edx，源颜色格式565 目标颜色格式555版本
#define		KD_RIO_MIX_2_PIXEL_COLOR_USE_EABDX_1555							\
{	push	ecx					}						\
{	mov     ebx, pPalette		}						\
{	movzx	eax, BYTE ptr[esi]	}						\
{	add		eax, eax			}						\
{	inc		esi					}						\
{	mov     dx, [ebx + eax]		}	/*edx = ...rgb*/	\
{	mov		ax, dx				}	/*565->555*/		\
{	shr		dx, 6				}	/*edx = ....rg*/	\
{	shl		edx, 16				}	/*edx = .rg...*/	\
{	mov		dx, 0x1f			}	/*edx = .rg..m (m=mask)*/\
{	and		dx, ax				}	/*edx = .rg..b*/	\
{	shl		dx, 11				}	/*edx = .rgb..*/	\
{	shl		edx, 5				}	/*edx = rgb...*/	\
{	mov		eax, edx			}   /*eax = rgb...*/	\
{	shr		eax, 16				}	/*eax = ...rgb*/	\
{	or		eax, edx			}	/*eax = rgbrgb*/	\
{	mov		ecx, 0x03e07c1f		}	/*555 mask*/		\
{	and		eax, ecx			}	/*eax = .g.r.b*/	\
{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
{	mov		bx, dx				}	/*ebx = ...rgb*/	\
{	shl		ebx, 16				}	/*ebx = rgb...*/	\
{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
{	add		eax, ebx			}						\
{	add		ebx, ebx			}						\
{	add		eax, ebx			}						\
{	shr		eax, 2				}	/*c = (3xc1+c2)/4*/	\
{	and     eax, ecx			}	/*eax = .g.r.b*/	\
{	mov     dx, ax				}	/*edx = ...r.b*/	\
{	shr     eax, 16				}	/*eax = ....g.*/	\
{	or      ax, dx				}	/*eax = ...rgb*/	\
{	mov		[edi], ax			}						\
{	inc		edi					}						\
{	inc		edi					}						\
{	pop		ecx					}
//alpha混合两个颜色，运算使用了eax,ebx,edx
#define		KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX							\
{	push	ecx					}						\
{	mov     ebx, pPalette		}						\
{	movzx	eax, BYTE ptr[esi]	}						\
{	mov		ecx, dwRGBBitMask32}						\
{	add		eax, eax			}						\
{	inc		esi					}						\
{	mov     dx, [ebx + eax]		}	/*edx = ...rgb*/	\
{	mov		ax, dx				}	/*eax = ...rgb*/	\
{	shl		eax, 16				}	/*eax = rgb...*/	\
{	mov		ax, dx				}	/*eax = rgbrgb*/	\
{	and		eax, ecx			}	/*eax = .g.r.b*/	\
{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
{	mov		bx, dx				}	/*ebx = ...rgb*/	\
{	shl		ebx, 16				}	/*ebx = rgb...*/	\
{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
{	mov		edx, nAlpha			}						\
{	imul	eax, edx			}	/*eax = c1*nAlpha*/	\
{	neg		edx					}						\
{	add		edx, 32				}						\
{	imul	ebx, edx			} /*ebx=c2*(32-nAlpha)*/\
{	add		eax, ebx			}						\
{	shr		eax, 5				}	/*c=(c1*nAlpha+c2*(32-nAlpha))/32*/ \
{	and     eax, ecx			}	/*eax = .g.r.b*/	\
{	mov     dx, ax				}	/*edx = ...r.b*/	\
{	shr     eax, 16				}	/*eax = ....g.*/	\
{	or      ax, dx				}	/*eax = ...rgb*/	\
{	mov		[edi], ax			}						\
{	inc		edi					}						\
{	inc		edi					}						\
{	pop		ecx					}
//alpha混合两个颜色，运算使用了eax,ebx,edx，源颜色格式565 目标颜色格式555版本
#define		KD_RIO_MIX_2_PIXEL_COLOR_ALPHA_USE_EABDX_1555						\
{	push	ecx					}						\
{	mov     ebx, pPalette		}						\
{	movzx	eax, BYTE ptr[esi]	}						\
{	add		eax, eax			}						\
{	inc		esi					}						\
{	mov     dx, [ebx + eax]		}	/*edx = ...rgb*/	\
{	mov		ax, dx				}	/*565->555*/		\
{	shr		dx, 6				}	/*edx = ....rg*/	\
{	shl		edx, 16				}	/*edx = .rg...*/	\
{	mov		dx, 0x1f			}	/*edx = .rg..m (m=mask)*/\
{	and		dx, ax				}	/*edx = .rg..b*/	\
{	shl		dx, 11				}	/*edx = .rgb..*/	\
{	shl		edx, 5				}	/*edx = rgb...*/	\
{	mov		eax, edx			}   /*eax = rgb...*/	\
{	shr		eax, 16				}	/*eax = ...rgb*/	\
{	or		eax, edx			}	/*eax = rgbrgb*/	\
{	mov		ecx, 0x03e07c1f		}	/*555 mask*/		\
{	and		eax, ecx			}	/*eax = .g.r.b*/	\
{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
{	mov		bx, dx				}	/*ebx = ...rgb*/	\
{	shl		ebx, 16				}	/*ebx = rgb...*/	\
{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
{	mov		edx, nAlpha			}						\
{	imul	eax, edx			}	/*eax = c1*nAlpha*/	\
{	neg		edx					}						\
{	add		edx, 32				}						\
{	imul	ebx, edx			} /*ebx=c2*(32-nAlpha)*/\
{	add		eax, ebx			}						\
{	shr		eax, 5				}	/*c=(c1*nAlpha+c2*(32-nAlpha))/32*/ \
{	and     eax, ecx			}	/*eax = .g.r.b*/	\
{	mov     dx, ax				}	/*edx = ...r.b*/	\
{	shr     eax, 16				}	/*eax = ....g.*/	\
{	or      ax, dx				}	/*eax = ...rgb*/	\
{	mov		cx, ax				}						\
{	shr		ax, 6				}	/*eax = ....rg*/	\
{	shl		eax, 16				}	/*eax = .rg...*/	\
{	mov		ax, 0x1f			}	/*eax = .rg..m (m=mask)*/\
{	and		ax, cx				}	/*eax = .rg..b*/	\
{	shl		ax, 11				}	/*eax = .rgb..*/	\
{	shr		eax,11				}	/*eax = ...rgb*/	\
{	mov		[edi], ax			}						\
{	inc		edi					}						\
{	inc		edi					}						\
{	pop		ecx					}


#endif
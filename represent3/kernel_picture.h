/* -------------------------------------------------------------------------
//	文件名		：	kernel_picture.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-23 11:11:11
//	功能描述	：	kernel_picture的定义,图片的定义
//
// -----------------------------------------------------------------------*/
#ifndef __KERNEL_PICTURE_H__
#define __KERNEL_PICTURE_H__
#include "../represent_common/common.h"

//kernel - pic
extern D3DXVECTOR2	g_sPicPostScaling;
extern BOOL	g_bForceDrawPic;

//cmd
enum KE_RENDER_STYLE
{
	emRENDER_STYLE_ALPHA_MODULATE_ALPHA_BLENDING = 1 << 0,
	emRENDER_STYLE_ALPHA_COLOR_MODULATE_ALPHA_BLENDING = 1 << 3,
	emRENDER_STYLE_ALPHA_BLENDING = 1 << 6,
	emRENDER_STYLE_COPY = 1 << 1,
	emRENDER_STYLE_SCREEN = 3,
};

enum KE_REF_METHOD
{
	emREF_METHOD_TOP_LEFT		= 0,
	emREF_METHOD_CENTER			= 1,
	emREF_METHOD_FRAME_TOP_LEFT	= 2,
};

enum KE_PIC_TYPE
{
	emPIC_TYPE_JPG_OR_LOCKABLE,
	emPIC_TYPE_SPR,
	emPIC_TYPE_RT,
};

struct KPicCmd
{
	D3DXVECTOR3		sPos1;
	D3DXVECTOR3		sPos2;
	DWORD			dwColor;
	BYTE			byRenderStyle;	//one of render_style_t
	BYTE			byRefMethod;	//one of ref_method_t
	DWORD			dwPicType;		//one of pic_type_t
	char			szFileName[128];
	char			szSubstrituteFileName[128];
	DWORD			dwHandle1;
	DWORD			dwHandle2;
	WORD			wFrameIndex;
	DWORD			dwExchangeColor;
};

struct KPicCmdPart : public KPicCmd
{
	KPOINT			cTopLeft;
	KPOINT			cBottomRight;
};

struct KPicCmdPartFour : public KPicCmdPart
{
	D3DXVECTOR3			sPos3;
	D3DXVECTOR3			sPos4;
};

//drawable
enum KE_ALPHA_BLENDING_MODE
{
	emKE_ALPHA_BLENDING_MODE_ALPHA,
	emKE_ALPHA_BLENDING_MODE_COPY,
	emKE_ALPHA_BLENDING_MODE_SCREEN,
};

struct KPicDrawable
{
	D3DXVECTOR2				asPos[2];
	D3DXVECTOR2				asUV[2];
	DWORD					dwColor;
	LPD3DTEXTURE			pTexture;
	KE_ALPHA_BLENDING_MODE	eAlphaBlendingMode;
};

//buf
extern vector<KPicDrawable>		g_vecPicDrawable;

//gpu objects
extern LPD3DIB								g_pPicMultiquadIndexBuffer;
extern DWORD								g_dwPicSrsAlpha;
extern DWORD								g_dwPicSrsCopy;
extern DWORD								g_dwPicSrsScreen;

struct KPicKernelInitParam
{
	LPD3DIB	pMultiquadIb;
	BOOL	bLinearFilter;
};

BOOL InitPicKernel(void* arg);

void ShutdownPicKernel();

void CorrectPicHandle(KPicCmd* in);

struct KPRIMITIVE_INDEX
{
	UINT	uImage;		// 图像ID
	INT		nFrame;		// 图像帧序号
	INT		nRenderStyle;
};

struct KPRIMITIVE_INDEX_LIST
{
	KPRIMITIVE_INDEX		sIndex;	// 图元索引数据
	KPRIMITIVE_INDEX_LIST*	pNext;
};

struct KPicCmdArg
{
	BOOL					bPart;
	KPRIMITIVE_INDEX_LIST*	pStandByList;
};

BOOL SnapshotPic(KPicCmd* pIn, KPicCmdArg* pArg, KPicDrawable* pOut);

//batch
void BatchPicCmd(void* pCmd, void* pArg);

//flush
void FlushPicBuf();

#endif
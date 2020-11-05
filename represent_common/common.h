/* -------------------------------------------------------------------------
//	�ļ���		��	common.h
//	������		��	fenghewen
//	����ʱ��	��	2009-11-18 11:11:00
//	��������	��	����ȫ�����Ͷ��塢����ͷ�ļ������ӵĿ⡢���õĺ�ͽṹ�Ķ���
// -----------------------------------------------------------------------*/
#pragma once
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#ifdef UNICODE
#undef UNICODE
#endif
//header & lib
#pragma comment (lib, "winmm.lib")		//for timeGetTime()
#pragma comment (lib, "gdiplus.lib")	//for jpeg encoder
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "ddraw.lib")
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "jpglib.lib")

#include <windows.h>
#include <gdiplus.h>
#include <process.h>		//for multi-threading
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <hash_map>
#include <string>
#include <cmath>
#include <algorithm>
//#include "ksdef.h"
#include "directx9/ddraw.h"
#include "directx9/d3dx9.h"
#include "client_core/ijxreplay.h"	// Replay ...Add by Lucien
#include "text/jxtext_linklib.h"
#include "text/jxtext.h"

using namespace Gdiplus;
using namespace std;
using namespace stdext;		//for hashmap

#undef min
#undef max
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

//typedefs
typedef LPD3DXBUFFER			LPD3DBUFFER;
typedef LPDIRECT3D9				LPD3D;
typedef LPDIRECT3DDEVICE9		LPD3DDEVICE;
typedef LPDIRECT3DVERTEXBUFFER9	LPD3DVB;
typedef LPDIRECT3DINDEXBUFFER9	LPD3DIB;
typedef LPDIRECT3DTEXTURE9		LPD3DTEXTURE;
typedef LPDIRECT3DSURFACE9		LPD3DSURFACE;

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef struct {BYTE b1, b2, b3;} COLORCOUNT;
typedef unsigned long	DWORD;
typedef class KPoint
{
public:
	INT x, y; 
	KPoint()							{ x = y = 0; } 
	KPoint(INT nX, INT nY)				{ x = nX; y = nY; }
	KPoint operator + (const KPoint& rPoint)		{ return KPoint(x + rPoint.x, y + rPoint.y); }
	KPoint operator - (const KPoint& rPoint)		{ return KPoint(x - rPoint.x, y - rPoint.y); }
	bool operator == (const KPoint& rPoint)	{ return (x == rPoint.x && y == rPoint.y); }
	bool operator != (const KPoint& rPoint)	{ return (x != rPoint.x || y != rPoint.y); }
} KPOINT;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

//macros & constants
#define BOOL_bool(b)			((b) != 0)
#define SAFE_DELETE(p)			{ if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)			{ if(p) { (p)->Release(); (p)=NULL; } }
#define SAFE_FREE(p)			{ if(p) { free(p);        (p)=NULL; } }

const KPOINT g_PointZero = KPOINT(0, 0);

struct KRepresentConfig
{
	DWORD	dwLoaderThreadCount;	// ��ԴLoader�߳�������
	DWORD	dwMediaTimeLimit;		// ��Դ����ʱ�� �� in seconds ��
	DWORD	dwMediaSpaceLimit;		// ��Դ�����ڴ����� �� in megabytes ��
	BOOL	bShowFps;				// �Ƿ���ʾFPS
	DWORD	dwLanguage;				// ��������
	BOOL	bUseStackForLoader;		// �Ƿ�Ϊ��ԴLoader���û���յ
	DWORD	dwPerFrameTaskCountLimit;	// ÿ֡�������ļ�����������ý��ĸ�����

	DWORD	dwAlphaLevel;			// Alpha�ȼ�

	BOOL	bVerticalSync;			// �Ƿ�����ֱͬ��
	BOOL	bMultithreadedD3d;		// �Ƿ������̵߳�D3D
	DWORD	dwMipmap;				// ϸ�ڼ���
	DWORD	dwTextureFormat;		//Texture��ʽ �� 0 - 32bit, 1 - 16bit, else - compressed ��
	DWORD	dwMaxBatchedQuadCount;		// ������εĲ�ѯ����
	BOOL	bEnableConditionalNonPow2;	// �Ƿ�����Դsize��8�ı�������
};

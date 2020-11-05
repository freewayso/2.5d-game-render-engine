/* -------------------------------------------------------------------------
//	文件名		：	media_center.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	媒体类工具函数
//
// -----------------------------------------------------------------------*/

#ifndef __MEDIA_CENTER_H__
#define __MEDIA_CENTER_H__
#include "../represent_common/common.h"
#include "../represent_common/vlist.h"
#include "../represent_common/media.h"

// media center
extern DWORD g_dwMediaBytes;
extern CRITICAL_SECTION g_sMediaBytesCS;
extern DWORD g_dwLoaderThreadCount;

typedef BOOL		( *InitMediaCreator ) (void* pArg);
typedef void		( *ShutdownMediaCreator ) ();
typedef KMedia*	( *CreateMedia ) (LPCSTR pszIdentifier, void* pArg);

void	InitMediaCenter(DWORD dwLoaderThreadCount, BOOL bProduceByLoader);
void	ShutdownMediaCenter();
DWORD	InstallMediaType(InitMediaCreator pfnInit, void* pInitArg, CreateMedia pfnCreate, ShutdownMediaCreator pfnShutdown);
DWORD	FindOrCreateMedia(DWORD dwType, LPCSTR pszIdentifier, void* pArg, BYTE byNoCollect = 0);
DWORD	FindMedia(DWORD dwType, LPCSTR pszIdentifier);
DWORD	CheckHandle(DWORD dwHandle);
BOOL	QueryProduct(DWORD dwHandle, BOOL bWait, void** ppProduct);
void	PurgeProcessingQueues();
void	DiscardVideoObjects();
void	CollectGarbage(DWORD dwTimeout, DWORD dwBytesLimit);	//in milliseconds
void	PlusMediaBytes(DWORD dwBytes);
void	MinusMediaBytes(DWORD dwBytes);

struct KDummyMedia : public KMedia
{
	void Load() {}
	void Process() {}
	void Produce() {}
	void PostProcess() {}
	void DiscardVideoObjects() {}
	void Clear() {}
	void Destroy() {delete this;}
	void* Product() {return 0;}
};

BOOL InitDummyMediaCreator(void* pArg);
void ShutdownDummyMediaCreator();
KMedia* CreateDummyMedia(LPCSTR pszIdentifier, void* pArg);

//casing
struct KMediaCasing
{
	KMediaCasing()
	{
		m_pMedia = NULL;
		m_dwType = 0;
		m_dwPriorityTag = 0;
		m_dwLastQueryTime= 0;
		m_byNoCollect = 0;
	}

	KMedia*			m_pMedia;
	DWORD			m_dwType;
	DWORD			m_dwPriorityTag;
	DWORD			m_dwLastQueryTime;
	BYTE			m_byNoCollect;
};

//catalog
typedef hash_map<string, DWORD>	CATALOG;

//type
struct KMediaType
{
	CreateMedia				m_pfnCreate;
	ShutdownMediaCreator	m_pfnShutdown;
	CATALOG					m_hmapCatalog;
};

extern vector<KMediaCasing>		g_vecMediaCasing;
extern vector<KMediaType>		g_vecMediaType;
extern KVList<DWORD>			g_vlstMediaPriorityList;

//media type consts
extern DWORD g_dwMediaTypeSpr;
extern DWORD g_dwMediaTypeJpg;
extern DWORD g_dwMediaTypeBitmap;
extern DWORD g_dwMediaTypeFont;

#endif
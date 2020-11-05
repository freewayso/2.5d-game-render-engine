/* -------------------------------------------------------------------------
//	文件名		：	media_center.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	媒体类工具函数
//
// -----------------------------------------------------------------------*/
#include "../represent_common/timer.h"
#include "media_center.h"
#include "global_config.h"

DWORD g_dwMediaBytes = 0;
CRITICAL_SECTION g_sMediaBytesCS;
DWORD g_dwLoaderThreadCount;

vector<KMediaCasing>	g_vecMediaCasing;
vector<KMediaType>		g_vecMediaType;
KVList<DWORD>			g_vlstMediaPriorityList;

//media types
DWORD g_dwMediaTypeSpr;
DWORD g_dwMediaTypeJpg;
DWORD g_dwMediaTypeRt;
DWORD g_dwMediaTypeLockable;
DWORD g_dwMediaTypeFont;

void InitMediaCenter(DWORD dwLoaderThreadCount, BOOL bProduceByLoader)
{
	g_dwMediaBytes = 0;
	g_dwLoaderThreadCount = dwLoaderThreadCount;
	if( g_dwLoaderThreadCount > 0 )
		InitializeCriticalSection( &g_sMediaBytesCS );

	//init media base system
	KMedia::Init(dwLoaderThreadCount, bProduceByLoader);
	KMedia::SetTaskLimit(g_cGlobalConfig.m_dwPerFrameTaskCountLimit);
	InstallMediaType(InitDummyMediaCreator, 0, CreateDummyMedia, ShutdownDummyMediaCreator);	//install dummy media type
	FindOrCreateMedia(0, "", 0);		//handle 0 is reserved for invalid handle
}	

void ShutdownMediaCenter()
{
	for( DWORD i = 0; i < g_vecMediaCasing.size(); i++ )
	{
		g_vecMediaCasing[i].m_pMedia->RequestDestroyed(TRUE);
	}
	g_vecMediaCasing.clear();

	for( DWORD i = 0; i < g_vecMediaType.size(); i++ )
	{
		g_vecMediaType[i].m_pfnShutdown();
	}
	g_vecMediaType.clear();

	g_vlstMediaPriorityList.Clear();

	//shutdown media base system
	KMedia::Shutdown();

	if( g_dwLoaderThreadCount > 0 )
		DeleteCriticalSection( &g_sMediaBytesCS );
}

DWORD InstallMediaType(InitMediaCreator pfnInit, void* pInitArg, CreateMedia pfnCreate, ShutdownMediaCreator pfnShutdown)
{
	if( !pfnInit(pInitArg) )	//init fail, return dummy type
		return 0;

	DWORD dwOldSize = (DWORD)g_vecMediaType.size();
	g_vecMediaType.resize(dwOldSize + 1);
	g_vecMediaType[dwOldSize].m_pfnCreate = pfnCreate;
	g_vecMediaType[dwOldSize].m_pfnShutdown = pfnShutdown;
	return dwOldSize;
}

DWORD FindOrCreateMedia(DWORD dwType, LPCSTR pszIdentifier, void* pArg, BYTE byNoCollect/* = 0*/)
{
	CATALOG::iterator p = g_vecMediaType[dwType].m_hmapCatalog.find(pszIdentifier);
	if( p != g_vecMediaType[dwType].m_hmapCatalog.end() )
		return p->second;

	//if (g_bLowVidMem)
	//	return 0; // 显存不足时不再创建

	DWORD dwHandle = (DWORD)g_vecMediaCasing.size();
	KMediaCasing elem;
	elem.m_pMedia = g_vecMediaType[dwType].m_pfnCreate(pszIdentifier, pArg);
	elem.m_dwType = dwType;
	elem.m_dwPriorityTag = 0;
	elem.m_dwLastQueryTime = 0;
	elem.m_byNoCollect = byNoCollect;
	g_vecMediaCasing.push_back(elem);
	g_vecMediaType[dwType].m_hmapCatalog[pszIdentifier] = dwHandle;
	return dwHandle;
}

DWORD FindMedia(DWORD dwType, LPCSTR pszIdentifier)
{
	CATALOG::iterator p = g_vecMediaType[dwType].m_hmapCatalog.find(pszIdentifier);
	if( p != g_vecMediaType[dwType].m_hmapCatalog.end() )
		return p->second;
	return 0;
}

DWORD CheckHandle(DWORD dwHandle)
{
	return (dwHandle < g_vecMediaCasing.size()) ? g_vecMediaCasing[dwHandle].m_dwType : 0;
}

BOOL QueryProduct(DWORD dwHandle, BOOL bWait, void** ppProduct)
{
	if (g_vecMediaCasing[dwHandle].m_byNoCollect == 0)
	{
		g_vecMediaCasing[dwHandle].m_dwLastQueryTime = NowTime();
		if( !g_vecMediaCasing[dwHandle].m_dwPriorityTag )
		{
			g_vlstMediaPriorityList.PushFront(dwHandle);
			g_vecMediaCasing[dwHandle].m_dwPriorityTag = g_vlstMediaPriorityList.Begin();
		}
		g_vlstMediaPriorityList.Move(g_vecMediaCasing[dwHandle].m_dwPriorityTag, g_vlstMediaPriorityList.Begin());
	}
	
	return g_vecMediaCasing[dwHandle].m_pMedia->RequestProduct(bWait, ppProduct);
}

DWORD AppendMedia(KMedia* pMedia)
{
	DWORD old_size = (DWORD)g_vecMediaCasing.size();
	KMediaCasing elem;
	elem.m_pMedia = pMedia;
	elem.m_dwPriorityTag = 0;
	elem.m_dwLastQueryTime = 0;
	g_vecMediaCasing.push_back(elem);
	return old_size;
}

void QueryProductWithoutReturn(DWORD dwHandle)
{
	if (g_vecMediaCasing[dwHandle].m_byNoCollect == 0)
	{
		g_vecMediaCasing[dwHandle].m_dwLastQueryTime = NowTime();
		if( !g_vecMediaCasing[dwHandle].m_dwPriorityTag )
		{
			g_vlstMediaPriorityList.PushFront(dwHandle);
			g_vecMediaCasing[dwHandle].m_dwPriorityTag = g_vlstMediaPriorityList.Begin();
		}
		g_vlstMediaPriorityList.Move(g_vecMediaCasing[dwHandle].m_dwPriorityTag, g_vlstMediaPriorityList.Begin());
	}
}

void PurgeProcessingQueues()
{
	for( DWORD i = 0; i < g_vecMediaCasing.size(); i++ )
	{
		g_vecMediaCasing[i].m_pMedia->RequestNonprocessing(TRUE);
	}
}

void DiscardVideoObjects()
{
	for( DWORD i = 0; i < g_vecMediaCasing.size(); i++ )
	{
		g_vecMediaCasing[i].m_pMedia->DiscardVideoObjects();
	}
}

 //TODO: temporary code
void CollectGarbage(DWORD dwTimeout, DWORD dwBytesLimit)
{
	static DWORD sdwLastCollectUseTime = 0;
	if (sdwLastCollectUseTime >= _KD_CLEANUP_TOLERANCE)
	{
		sdwLastCollectUseTime >>= 1;
		return;
	}
	DWORD iterator = g_vlstMediaPriorityList.RBegin();
	DWORD dwStart = timeGetTime();

	while(	(iterator != g_vlstMediaPriorityList.REnd()) && 
			((NowTime() - g_vecMediaCasing[g_vlstMediaPriorityList.Value(iterator)].m_dwLastQueryTime > dwTimeout) 
			|| (g_dwMediaBytes > dwBytesLimit)) )
	{
		DWORD cur_iterator = iterator;
		iterator = g_vlstMediaPriorityList.Pre(iterator);

		DWORD handle = g_vlstMediaPriorityList.Value(cur_iterator);

		if( g_vecMediaCasing[handle].m_pMedia->RequestCleared(false) )
		{
			g_vlstMediaPriorityList.Erase(cur_iterator);
			g_vecMediaCasing[handle].m_dwPriorityTag = 0;
			g_vecMediaCasing[handle].m_dwLastQueryTime = 0;
		}

		DWORD dwEnd = timeGetTime();
		if (dwEnd - dwStart > _KD_CLEANUP_TOLERANCE) // test
			break;
	}

	DWORD dwEnd = timeGetTime();

	sdwLastCollectUseTime = dwEnd - dwStart;
}


// todo: 这两个函数不是很重要，但是临界区阻碍了绘制线程
VOID PlusMediaBytes(DWORD dwBytes)
{
	//if( g_dwLoaderThreadCount > 0 )
	//	EnterCriticalSection(&g_sMediaBytesCS);
	g_dwMediaBytes += dwBytes; 
	//if( g_dwLoaderThreadCount > 0 )
	//	LeaveCriticalSection(&g_sMediaBytesCS);
}

void MinusMediaBytes(DWORD dwBytes) 
{
	//if( g_dwLoaderThreadCount > 0 )
	//	EnterCriticalSection(&g_sMediaBytesCS);
	g_dwMediaBytes -= dwBytes; 
	//if( g_dwLoaderThreadCount > 0 )
	//	LeaveCriticalSection(&g_sMediaBytesCS);
}

BOOL InitDummyMediaCreator(void* pArg) 
{
	return TRUE;
}

void ShutdownDummyMediaCreator() 
{

}

KMedia* CreateDummyMedia(LPCSTR pszIdentifier, void* pArg) 
{
	return new KDummyMedia;
}
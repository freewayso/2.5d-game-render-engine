/* -------------------------------------------------------------------------
//	文件名		：	loader.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-19 11:11:11
//	功能描述	：	读取器
//
// -----------------------------------------------------------------------*/
#ifndef __LOADER_H__
#define __LOADER_H__
#include "common.h"
#include "async_queue.h"

//loader
class IKLoadTask
{
public:
	virtual VOID Load() = 0;
	virtual VOID Process() = 0;
	virtual VOID Produce() = 0;
};

class IKMediaLoader
{
public:
	virtual BOOL ProduceByLoader() = 0;
	virtual BOOL Send(IKLoadTask* pTask) = 0;
	virtual BOOL Fetch(IKLoadTask** ppDone, DWORD dwTimeout) = 0;
	virtual VOID Destroy() = 0;

	virtual VOID ResetTaskCount() = 0;
	virtual VOID SetTaskLimit(INT nTaskLimit) = 0;
};

class KMediaLoader : public IKMediaLoader
{
public:
	KMediaLoader(BOOL bProduce, DWORD dwThreadCount);
	VOID Run();
	VOID Destroy();

	BOOL Send(IKLoadTask* pTask)						
	{
		if (m_nTaskLimit == 0)
		{
			 m_queTask.Push(pTask);
			 return TRUE;
		}
		if (m_nCurTaskCount <= m_nTaskLimit)
		{
			 m_queTask.Push(pTask);
			 ++m_nCurTaskCount;
			 return TRUE;
		}
		return FALSE;
	}

	BOOL Fetch(IKLoadTask** ppDoneTask, DWORD dwTimeout)	{return m_queDone.Pop(ppDoneTask, dwTimeout);}
	BOOL ProduceByLoader()									{return m_bProduce;}

	VOID ResetTaskCount()									{m_nCurTaskCount = 0;}
	VOID SetTaskLimit(INT nTaskLimit)						{m_nTaskLimit = nTaskLimit;}

protected:
	BOOL						m_bProduce;
	KAsyncQueue<IKLoadTask*>	m_queTask;
	KAsyncQueue<IKLoadTask*>	m_queDone;
	CRITICAL_SECTION			m_sWaitingRoom;
	CRITICAL_SECTION			m_sCounter_cs;
	DWORD 						m_dwThreadCount;
	DWORD 						m_dwActiveThreadCount;

	INT							m_nCurTaskCount;
	INT							m_nTaskLimit;
};

VOID LoaderMain( VOID* arg );
IKMediaLoader* CreateLoader(BOOL bProduce, DWORD dwThreadCount);

#endif
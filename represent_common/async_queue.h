/* -------------------------------------------------------------------------
//	文件名		：	async_queue.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-19 11:11:11
//	功能描述	：	异步队列
//
// -----------------------------------------------------------------------*/
#ifndef __ASYNC_QUEUE_H__
#define __ASYNC_QUEUE_H__
#include "common.h"
//async queue
static BOOL s_bUseStackForLoader = FALSE;

template <typename TYPE>
class KAsyncQueue
{
public:
	KAsyncQueue();
	~KAsyncQueue();
	void Push(TYPE tObj);
	bool Pop(TYPE* pObj, DWORD dwTimeout);

private:
	stack<TYPE>			m_stkObjStack;
	queue<TYPE>			m_queObjQueue;
	CRITICAL_SECTION	m_sCs;
	HANDLE				m_hObjComingEvent;

};


//implementation
template <typename TYPE>
KAsyncQueue<TYPE>::KAsyncQueue()
{
	InitializeCriticalSection(&m_sCs);
	m_hObjComingEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

template <typename TYPE>
KAsyncQueue<TYPE>::~KAsyncQueue()
{
	DeleteCriticalSection(&m_sCs);
	CloseHandle(m_hObjComingEvent);
}

template <typename TYPE>
void KAsyncQueue<TYPE>::Push(TYPE tObj)
{
	EnterCriticalSection( &m_sCs );
	if( s_bUseStackForLoader )
		m_stkObjStack.push( tObj );
	else
		m_queObjQueue.push( tObj );
	SetEvent( m_hObjComingEvent );
	LeaveCriticalSection( &m_sCs );
}

template <typename TYPE>
bool KAsyncQueue<TYPE>::Pop(TYPE* pObj, DWORD dwTimeout)
{
	if( WAIT_TIMEOUT == WaitForSingleObject( m_hObjComingEvent, dwTimeout ) )
		return FALSE;
	EnterCriticalSection(&m_sCs);
	if( s_bUseStackForLoader )
	{
		if (!m_stkObjStack.empty())
		{
			*pObj = m_stkObjStack.top();
			m_stkObjStack.pop();
		}
		else
		{
			*pObj = NULL;
		}
		if( m_stkObjStack.empty() )
			ResetEvent( m_hObjComingEvent );
	}
	else
	{
		if (!m_queObjQueue.empty())
		{
			*pObj = m_queObjQueue.front();
			m_queObjQueue.pop();
		}
		else
		{
			*pObj = NULL;
		}
		if( m_queObjQueue.empty() )
			ResetEvent( m_hObjComingEvent );
	}
	LeaveCriticalSection(&m_sCs);
	return true;
}

#endif /* __ASYNC_QUEUE_H__ */
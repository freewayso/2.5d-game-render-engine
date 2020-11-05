/* -------------------------------------------------------------------------
//	文件名		：	loader.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-19 11:11:11
//	功能描述	：	读取器
//
// -----------------------------------------------------------------------*/
#include "loader.h"

KMediaLoader::KMediaLoader(BOOL bProduce, DWORD dwThreadCount)
{
	m_bProduce = bProduce;
	m_dwActiveThreadCount = m_dwThreadCount = dwThreadCount;
	m_nCurTaskCount = 0;
	m_nTaskLimit = 0;
	if( dwThreadCount > 1 )
	{
		InitializeCriticalSection( &m_sWaitingRoom );
		InitializeCriticalSection( &m_sCounter_cs );
	}
}

VOID KMediaLoader::Destroy()
{
	for( DWORD i = 0; i < m_dwThreadCount; i++ )
	{
		m_queTask.Push( 0 );
	}
}

VOID KMediaLoader::Run()
{
	while(true)
	{
		if( m_dwThreadCount > 1 )
			EnterCriticalSection( &m_sWaitingRoom );

		IKLoadTask* pTask = NULL;
		m_queTask.Pop( &pTask, INFINITE );

		if( m_dwThreadCount > 1 )
			LeaveCriticalSection( &m_sWaitingRoom );

		if(!pTask)
			break;

		pTask->Load();
		pTask->Process();
		if( m_bProduce )
			pTask->Produce();

		m_queDone.Push( pTask );
	}

	DWORD nAtc_See;
	if( m_dwThreadCount > 1 )
		EnterCriticalSection( &m_sCounter_cs );
	nAtc_See = --m_dwActiveThreadCount;
	if( m_dwThreadCount > 1 )
		LeaveCriticalSection( &m_sCounter_cs );

	if( !nAtc_See )
	{
		if( m_dwThreadCount > 1 )
		{
			DeleteCriticalSection( &m_sWaitingRoom );
			DeleteCriticalSection( &m_sCounter_cs );
		}
		delete this;
	}
}

void LoaderMain( void* arg )
{
	((KMediaLoader*)arg)->Run();
}

IKMediaLoader* CreateLoader(BOOL bProduce, DWORD m_dwThreadCount)
{
	//at least one thread
	if( m_dwThreadCount == 0 )
		m_dwThreadCount = 1;

	KMediaLoader* pLoader = new KMediaLoader(bProduce, m_dwThreadCount);
	for( DWORD i = 0; i < m_dwThreadCount; i++ )
	{
		_beginthread(LoaderMain, 0, pLoader);
	}
	return pLoader;
}

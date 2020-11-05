/* -------------------------------------------------------------------------
//	文件名		：	media.cpp
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	读取器
//
// -----------------------------------------------------------------------*/
#include "common.h"
#include "media.h"

//implementation
IKMediaLoader* KMedia::m_pLoader = 0;

VOID KMedia::Init(DWORD dwLoaderThreadCount, BOOL bProduceByLoader)
{
	m_pLoader = dwLoaderThreadCount ? CreateLoader( bProduceByLoader, dwLoaderThreadCount ) : 0;
}

VOID KMedia::Shutdown()
{
	if( m_pLoader )
	{
		m_pLoader->Destroy();
		m_pLoader = 0;
	}
}

BOOL KMedia::Send()
{
	BOOL bRes = m_pLoader->Send(this);
	if (bRes == TRUE)
	{
		m_dwTag = eTagProcessing;
	}
	return bRes;
}

BOOL KMedia::Receive(BOOL bWait)
{
	IKLoadTask* pReceived = NULL;
	if(bWait)
	{	
		do
		{
			m_pLoader->Fetch(&pReceived, INFINITE);
			if (pReceived)
			{
				((KMedia*)pReceived)->m_dwTag = eTagReceived;
			}
		}while(pReceived != this);

		return TRUE;
	}
	else
	{
		while(m_pLoader->Fetch(&pReceived, 0))
		{
			((KMedia*)pReceived)->m_dwTag = eTagReceived;
		}

		if( m_dwTag != eTagReceived )
			return FALSE;

		return TRUE;
	}
}

VOID KMedia::PostReceived()
{
	if( !m_pLoader->ProduceByLoader() )
		Produce();
	PostProcess();
	m_dwTag = eTagProduced;
}

VOID KMedia::ProduceDirectly()
{
	Load();
	Process();
	Produce();
	PostProcess();
	m_dwTag = eTagProduced;
}

VOID KMedia::Clean()
{
	Clear();
	m_dwTag = eTagRaw;
}

BOOL KMedia::RequestProduct(BOOL bWait, VOID** ppOut)
{
	switch(m_dwTag)
	{
	case eTagRaw:
		if( !m_pLoader || bWait )
		{
			ProduceDirectly();
			*ppOut = Product();
			return TRUE;
		}
		Send();
		return FALSE;
	case eTagProcessing:
		if(!Receive(bWait))
			return FALSE;
	case eTagReceived:
		PostReceived();
	case eTagProduced:
		*ppOut = Product();
		return TRUE;
	}
	return FALSE;
}

BOOL KMedia::RequestNonprocessing(BOOL bWait)
{
	if(m_dwTag != eTagProcessing)
		return TRUE;

	return Receive(bWait);
}

BOOL KMedia::RequestCleared(BOOL bWait)
{
	if(!RequestNonprocessing(bWait))
		return FALSE;

	Clean();
	return TRUE;
}

BOOL KMedia::RequestDestroyed(BOOL bWait)
{
	if(!RequestNonprocessing(bWait))
		return FALSE;

	Destroy();
	return TRUE;
}

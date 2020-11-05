/* -------------------------------------------------------------------------
//	文件名		：	media.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-20 11:11:11
//	功能描述	：	媒体基类
//
// -----------------------------------------------------------------------*/
#ifndef __MEDIA_H__
#define __MEDIA_H__
#include "loader.h"
//media base
class KMedia : public IKLoadTask
{
private:
	enum
	{
		eTagRaw,
		eTagProcessing,
		eTagReceived,
		eTagProduced,
	};

	static IKMediaLoader*	m_pLoader;
	DWORD					m_dwTag;

	BOOL			Send();
	BOOL			Receive(BOOL bWait);
	VOID			PostReceived();
	VOID			ProduceDirectly();
	VOID			Clean();

protected:
	KMedia() {m_dwTag = eTagRaw;}
	virtual void	Load() = 0;
	virtual void	Process() = 0;
	virtual void	Produce() = 0;
	virtual void	PostProcess() = 0;
	virtual void	Clear() = 0;
	virtual void	Destroy() = 0;
	virtual void*	Product() = 0;

public:
	static VOID		Init(DWORD dwLoaderThreadCount, BOOL bProduceByLoader);
	static VOID		Shutdown();
	virtual VOID	DiscardVideoObjects() = 0;
	BOOL			RequestProduct(BOOL bWait, VOID** ppOut);
	BOOL			RequestNonprocessing(BOOL bWait);
	BOOL			RequestCleared(BOOL bWait);
	BOOL			RequestDestroyed(BOOL bWait);
	static VOID		ResetTaskCount() {m_pLoader->ResetTaskCount();}
	static VOID		SetTaskLimit(INT nTaskLimit) {m_pLoader->SetTaskLimit(nTaskLimit);}
};

#endif

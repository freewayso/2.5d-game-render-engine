/* -------------------------------------------------------------------------
//	文件名		：	vlist.h
//	创建者		：	fenghewen
//	创建时间	：	2009-11-18 11:11:00
//	功能描述	：	基于STL的vector自定制的list
//					主要用于保存各种元素的Id，避免频繁地调用new/delete
// -----------------------------------------------------------------------*/
#ifndef __VLIST_H__
#define __VLIST_H__
#include "common.h"

//vector based list ( to keep a persistent id for each element and to avoid frequently new/delete operations while reordering )
template <typename TYPE>
class KVList
{
private:
	struct KEntry
	{
		DWORD		dwPre, dwNext;			
		TYPE		tData;
	};

	vector<KEntry>			m_vecEntry;			//the first entry is reserved to connect the first and last element of the list together
	vector<DWORD>			m_vecFreeIndex;

public:
	KVList()				{Clear();}

	DWORD	Begin()				{return m_vecEntry[0].dwNext;}
	DWORD	End()				{return 0;}
	DWORD	RBegin()			{return m_vecEntry[0].dwPre;}
	DWORD	REnd()				{return 0;}
	DWORD	Pre(DWORD dwWhere)	{return m_vecEntry[dwWhere].dwPre;}
	DWORD	Next(DWORD dwWhere)	{return m_vecEntry[dwWhere].dwNext;}
	TYPE&	Value(DWORD dwWhere)	{return m_vecEntry[dwWhere].tData;}

	TYPE&	Front()				{return m_vecEntry[Begin()].tData;}
	TYPE&	Back()				{return m_vecEntry[RBegin()].tData;}

	VOID	PushFront(const TYPE& rtVal)	{return Insert(Begin(), rtVal);}
	VOID	PushBack(const TYPE& rtVal)	{return Insert(End(), rtVal);}
	VOID	PopFront()			{return Erase(Begin());}
	VOID	PopBack()			{return Erase(RBegin());}

	BOOL	Empty()				{return (Begin() == End());}
	size_t	Size()				{return (m_vecEntry.size() - 1 - m_vecFreeIndex.size());}

	VOID	Insert(DWORD dwWhere, const TYPE& rtVal);
	VOID	Erase(DWORD dwWhere);
	VOID	Move(DWORD dwFrom, DWORD dwTo);
	VOID	Clear();
};

//implementation
template <typename TYPE>
VOID KVList<TYPE>::Insert(DWORD dwWhere, const TYPE& rtVal)
{
	//find an empty entry
	DWORD entry;
	if( !m_vecFreeIndex.empty() )
	{
		entry = m_vecFreeIndex.back();
		m_vecFreeIndex.pop_back();
	}
	else 
	{
		entry = (DWORD)m_vecEntry.size();

		KEntry elem;
		m_vecEntry.push_back(elem);
	}

	//insert
	m_vecEntry[entry].tData = rtVal;
	m_vecEntry[entry].dwPre = m_vecEntry[dwWhere].dwPre;
	m_vecEntry[entry].dwNext = dwWhere;
	m_vecEntry[m_vecEntry[dwWhere].dwPre].dwNext = entry;
	m_vecEntry[dwWhere].dwPre = entry;
}

template <typename TYPE>
VOID KVList<TYPE>::Erase(DWORD dwWhere)
{
	m_vecFreeIndex.push_back(dwWhere);
	m_vecEntry[m_vecEntry[dwWhere].dwPre].dwNext = m_vecEntry[dwWhere].dwNext;
	m_vecEntry[m_vecEntry[dwWhere].dwNext].dwPre = m_vecEntry[dwWhere].dwPre;
}

template <typename TYPE>
VOID KVList<TYPE>::Move(DWORD dwFrom, DWORD dwTo)
{
	if( dwFrom != dwTo )
	{
		if (m_vecEntry[m_vecEntry[dwFrom].dwPre].dwNext != dwFrom || 
			m_vecEntry[m_vecEntry[dwFrom].dwNext].dwPre != dwFrom )
		{
			ASSERT(FALSE);
			return;
		}
		//erase
		m_vecEntry[m_vecEntry[dwFrom].dwPre].dwNext = m_vecEntry[dwFrom].dwNext;
		m_vecEntry[m_vecEntry[dwFrom].dwNext].dwPre = m_vecEntry[dwFrom].dwPre;

		//insert
		m_vecEntry[dwFrom].dwPre = m_vecEntry[dwTo].dwPre;
		m_vecEntry[dwFrom].dwNext = dwTo;
		m_vecEntry[m_vecEntry[dwTo].dwPre].dwNext = dwFrom;
		m_vecEntry[dwTo].dwPre = dwFrom;
	}
}

template <typename TYPE>
VOID KVList<TYPE>::Clear()				
{
	m_vecEntry.clear();

	KEntry elem;
	m_vecEntry.push_back(elem);
	m_vecEntry[0].dwPre = m_vecEntry[0].dwNext = 0;
	m_vecFreeIndex.clear();
}

#endif
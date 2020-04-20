#pragma once

namespace RmtImgui
{

template<typename TType>
inline void SafeDelete(TType*& pNewData) 
{ 
	if(pNewData != nullptr)
	{
		delete pNewData;
		pNewData = nullptr;
	} 
}

template<typename TType>
inline void SafeDelArray(TType*& pNewArrayData) 
{ 
	if(pNewArrayData != nullptr)
	{
		delete[] pNewArrayData;
		pNewArrayData = nullptr;
	} 
}

template <typename TType>
typename TType* ExchangePtr<TType>::Release()
{
	return mpData.exchange(nullptr);
}

//-----------------------------------------------------------------------------
// Take ownership of the provided data.
// If there's a previous unclaimed pointer to some data, release it
//-----------------------------------------------------------------------------
template <typename TType>
void ExchangePtr<TType>::Assign(TType*& pNewData)
{
	TType* pPreviousData = mpData.exchange(pNewData);
	SafeDelete( pPreviousData );
	pNewData = nullptr;
}

template <typename TType>
void ExchangePtr<TType>::Free()
{
	TType* pNull(nullptr);
	Assign(pNull);
}

template <typename TType>
ExchangePtr<TType>::~ExchangePtr()	
{ 
	Free();
}

template <typename TType, size_t TCount>
void Ringbuffer<TType,TCount>::AddData(const TType* pData, size_t& count)
{
	size_t i(0);
	for(; i<count && mPosLast-mPosCur < TCount; ++i)
	{
		mBuffer[(mPosLast + i) % TCount] = pData[i];
		++mPosLast;
	}
	count = i;
}

template <typename TType, size_t TCount>
void Ringbuffer<TType,TCount>::ReadData(TType* pData, size_t& count)
{
	size_t i(0);
	for(; i<count && mPosLast != mPosCur; ++i)
	{
		pData[i] = mBuffer[mPosCur % TCount];
		++mPosCur;
	}
	count = i;
}


	
} //namespace RmtImgui

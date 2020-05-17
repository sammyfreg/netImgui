#pragma once

#include <assert.h>

namespace NetImgui { namespace Internal
{

template <typename TType> 
TType* netImguiNew(size_t placementSize)
{
	return new( ImGui::MemAlloc(placementSize != static_cast<size_t>(-1) ? placementSize : sizeof(TType)) ) TType();
}

template <typename TType> 
void netImguiDelete(TType* pData)
{
	if( pData )
	{
		pData->~TType();
		ImGui::MemFree(pData);
	}
}

template <typename TType> 
void netImguiDeleteSafe(TType*& pData)
{
	netImguiDelete(pData);
	pData = nullptr;
}

//=============================================================================
//=============================================================================
template <typename TType>
TType* ExchangePtr<TType>::Release()
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
	netImguiDelete( mpData.exchange(pNewData) );
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

//=============================================================================
//
//=============================================================================
template <typename TType>
bool OffsetPointer<TType>::IsOffset()const
{
	return mOffset < 128*1024; //Not certain, but should do for now
}

template <typename TType>
bool OffsetPointer<TType>::IsPointer()const
{
	return !IsOffset();
}

template <typename TType>
TType* OffsetPointer<TType>::ToPointer()
{
	assert(IsOffset());
	mPointer = reinterpret_cast<TType*>( reinterpret_cast<uint64_t>(&mPointer) + mOffset );
	return mPointer;
}

template <typename TType>
uint64_t OffsetPointer<TType>::ToOffset()
{
	assert(IsPointer());
	mOffset = reinterpret_cast<uint64_t>(mPointer) - reinterpret_cast<uint64_t>(&mPointer);
	return mOffset;
}

template <typename TType>
TType* OffsetPointer<TType>::operator->()
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const TType* OffsetPointer<TType>::operator->()const
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
TType* OffsetPointer<TType>::Get()
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
const TType* OffsetPointer<TType>::Get()const
{
	assert(IsPointer());
	return mPointer;
}

template <typename TType>
TType& OffsetPointer<TType>::operator[](size_t index)
{
	assert(IsPointer());
	return mPointer[index];
}

template <typename TType>
const TType& OffsetPointer<TType>::operator[](size_t index)const
{
	assert(IsPointer());
	return mPointer[index];
}

//=============================================================================
//=============================================================================
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

#if 0
template <class T, class U>
constexpr bool operator == (const stdAllocator<T>&, const stdAllocator<U>&) noexcept
{
	return true;
}

template <class T, class U>
constexpr bool operator != (const stdAllocator<T>&, const stdAllocator<U>&) noexcept
{
	return false;
}
#endif

}} //namespace NetImgui::Internal

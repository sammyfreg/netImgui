#pragma once

#include <stdint.h>
#include <atomic>

#define MAYBE_UNUSED(_VAR_)		(void)_VAR_
#define ARRAY_COUNT(_ARRAY_)	(sizeof(_ARRAY_)/sizeof(_ARRAY_[0]))

namespace RmtImgui
{

template<typename TType>
inline void SafeDelete(TType*& pNewData);

template<typename TType>
inline void SafeDelArray(TType*& pNewArrayData);

//=============================================================================
// Class to exchange a pointer between two threads, safely
//=============================================================================
template <typename TType>
class ExchangePtr
{
public:
						~ExchangePtr();	
	inline TType*		Release();
	inline void			Assign(TType*& pNewData);
	inline void			Free();
private:
	std::atomic<TType*> mpData = nullptr;
};

template <typename TType, size_t TCount>
class Ringbuffer
{
public:
	void					AddData(const TType* pData, size_t& count);
	void					ReadData(TType* pData, size_t& count);
private:
	TType					mBuffer[TCount];
	std::atomic_uint64_t	mPosCur		= 0;
	std::atomic_uint64_t	mPosLast	= 0;
};

} //namespace RmtImgui

#include "RemoteImgui_Shared.inl"
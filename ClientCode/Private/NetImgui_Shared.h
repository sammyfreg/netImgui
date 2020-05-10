#pragma once

#include <atomic>
#include "../NetImGui_Api.h"
#define MAYBE_UNUSED(_VAR_)		(void)_VAR_
#define ARRAY_COUNT(_ARRAY_)	(sizeof(_ARRAY_)/sizeof(_ARRAY_[0]))

namespace NetImgui { namespace Internal
{

//=============================================================================
// All allocations made by netImgui goes through here. 
// Relies in ImGui allocator
//=============================================================================
template <typename TType> TType*	netImguiNew(size_t placementSize=(size_t)-1);
template <typename TType> void		netImguiDelete(TType* pData);
template <typename TType> void		netImguiDeleteSafe(TType*& pData);

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

//=============================================================================
// Make data serialization easier
//=============================================================================
template <typename TType>
struct OffsetPointer
{
	union
	{
		uint64_t	mOffset;
		TType*		mPointer;
	};	
	
	bool			IsPointer()const;
	bool			IsOffset()const;

	TType*			ToPointer();
	uint64_t		ToOffset();
	TType*			operator->();
	const TType*	operator->()const;
	TType&			operator[](size_t index);
	const TType&	operator[](size_t index)const;

	TType*			Get();
	const TType*	Get()const;
};

//=============================================================================
//=============================================================================
//SF re purpose this to a threadsafe consume/append buffer?
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

//=============================================================================
// Std custom allocator
// SF TODO get rid of this and use imgui vector?
//=============================================================================
template <class T>
struct stdAllocator 
{
  typedef T value_type;
						stdAllocator() noexcept {}
  template <class U>	stdAllocator (const stdAllocator<U>&) noexcept {}
  T*					allocate (std::size_t n) { return reinterpret_cast<T*>(IM_ALLOC(sizeof(T)*n)); }
  void					deallocate (T* p, std::size_t n) { MAYBE_UNUSED(n); IM_FREE(p); }
};

}} //namespace NetImgui::Internal

using namespace NetImgui::Internal;

#include "NetImGui_Shared.inl"
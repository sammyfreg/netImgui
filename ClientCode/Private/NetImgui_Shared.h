#pragma once

//=================================================================================================
// Include NetImgui_Api.h with almost no warning suppression.
// this is to make sure library user does not need to suppress any
#if defined(__clang__)
	#pragma clang diagnostic push

#elif defined(_MSC_VER) && !defined(__clang__)
	#pragma warning	(push)
	#pragma warning (disable: 5031)		// warning C5031: #pragma warning(pop): likely mismatch, popping warning state pushed in different file
	#pragma warning (disable: 4464)		// warning C4464: relative include path contains '..'
	#pragma warning (disable: 4514)		// unreferenced inline function has been removed
#endif
#include "../NetImgui_Api.h"
#include "NetImgui_WarningReenable.h"

//=================================================================================================
// Disable a few warnings that the NetImgui client code generates in -wall
#include "NetImgui_WarningDisable.h"	

//=================================================================================================
// Disable a few more warning caused by system includes
#if defined(__clang__)
	#pragma clang diagnostic push

#elif defined(_MSC_VER) && !defined(__clang__)
	#pragma warning	(push)
	#pragma warning (disable: 4820)		// warning C4820 : xxx : yyy bytes padding added after data member zzz
	#pragma warning (disable: 4710)		// warning C4710: 'xxx': function not inlined
#endif
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include "NetImgui_WarningReenable.h"
//=================================================================================================


#define MAYBE_UNUSED(_VAR_)		(void)_VAR_
#define ARRAY_COUNT(_ARRAY_)	(sizeof(_ARRAY_)/sizeof(_ARRAY_[0]))

namespace NetImgui { namespace Internal
{

constexpr uint64_t	u64Invalid	= static_cast<uint64_t>(-1);
constexpr size_t	sizeInvalid	= static_cast<size_t>(-1);

//=============================================================================
// All allocations made by netImgui goes through here. 
// Relies in ImGui allocator
//=============================================================================
template <typename TType> TType*	netImguiNew(size_t placementSize=static_cast<size_t>(-1));
template <typename TType> void		netImguiDelete(TType* pData);
template <typename TType> void		netImguiDeleteSafe(TType*& pData);

//=============================================================================
// Class to exchange a pointer between two threads, safely
//=============================================================================
template <typename TType>
class ExchangePtr
{
public:
						ExchangePtr():mpData(nullptr){}
						~ExchangePtr();	
	inline TType*		Release();
	inline void			Assign(TType*& pNewData);
	inline void			Free();
private:						
	std::atomic<TType*> mpData;

// Prevents warning about implicitly delete functions
private:
	ExchangePtr(const ExchangePtr&){}
	ExchangePtr(const ExchangePtr&&){}
	ExchangePtr operator=(const ExchangePtr&){}
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
							Ringbuffer():mPosCur(0),mPosLast(0){}
	void					AddData(const TType* pData, size_t& count);
	void					ReadData(TType* pData, size_t& count);
private:
	TType					mBuffer[TCount];
	std::atomic_uint64_t	mPosCur;
	std::atomic_uint64_t	mPosLast;

// Prevents warning about implicitly delete functions
private:
	Ringbuffer(const Ringbuffer&){}
	Ringbuffer(const Ringbuffer&&){}
	Ringbuffer operator=(const Ringbuffer&){}
};

}} //namespace NetImgui::Internal

#include "NetImgui_Shared.inl"
#include "NetImgui_WarningReenable.h"

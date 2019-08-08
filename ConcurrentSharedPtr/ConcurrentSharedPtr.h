#pragma once

// Copyright(c) 2019 Flovin Michaelsen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <atomic>
#include <functional>
#include <stdint.h>
#include "AtomicOWord.h"

namespace csp {

	class MoveSafe;
	class MoveFast;

	typedef MoveFast MoveDefault;
	typedef std::allocator<uint8_t> DefaultAllocator;

	template <class T>
	class DefaultDeleter;
	template <class T, class Allocator>
	class ControlBlock;
}

template <class T, class MoveType = csp::MoveDefault, class Allocator = csp::DefaultAllocator>
class ConcurrentSharedPtr;

template <class T, class ...Args>
inline ConcurrentSharedPtr<T, csp::MoveDefault> MakeConcurrentShared(Args&&...);
template <class T, class MoveType, class ...Args>
inline ConcurrentSharedPtr<T, MoveType, csp::DefaultAllocator> MakeConcurrentShared(Args&&...);
template <class T, class MoveType, class Allocator, class ...Args>
inline ConcurrentSharedPtr<T, MoveType, Allocator> MakeConcurrentShared(Allocator&, Args&&...);

#pragma warning(push)
#pragma warning(disable : 4201)

template <class T, class MoveType, class Allocator>
class ConcurrentSharedPtr
{
public:
	typedef std::size_t size_type;

	static constexpr std::size_t BlockAllocSize = sizeof(csp::ControlBlock<T, Allocator>) + alignof(T) + sizeof(T);

	inline ConcurrentSharedPtr();
	inline ConcurrentSharedPtr(const std::nullptr_t);
	inline ~ConcurrentSharedPtr();

	inline explicit ConcurrentSharedPtr(T* const aObject);
	template <class Deleter>
	inline explicit ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter);
	template <class Deleter>
	inline explicit ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter, const Allocator& aAllocator);

	/*	Concurrency SAFE	*/

	// May be used from any thread at any time
	//--------------------------------------------------------------------------------------------------//
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, MoveType, Allocator>& aOther);
	inline ConcurrentSharedPtr<T, MoveType, Allocator>& operator=(ConcurrentSharedPtr<T, MoveType, Allocator>& aOther);

	inline void SafeClaim(T* const aObject);
	template <class Deleter>
	inline void SafeClaim(T* const aObject, Deleter&& aDeleter);
	template <class Deleter>
	inline void SafeClaim(T* const aObject, Deleter&& aDeleter, const Allocator& aAllocator);

	inline void SafeReset();
	inline void SafeMove(ConcurrentSharedPtr<T, MoveType, Allocator>&& aFrom);

	inline const bool CompareAndSwap(const T* &aExpected, ConcurrentSharedPtr<T, MoveType, Allocator>& aDesired);

	//--------------------------------------------------------------------------------------------------//


	/*	Concurrency SAFE	*/

	// Safe versions of move constructor / operator. Disabled by default. May be enabled
	// by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::MoveSafe>::value>* = nullptr>
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::MoveSafe>::value>* = nullptr>
	inline ConcurrentSharedPtr<T, MoveType, Allocator>& operator=(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	//--------------------------------------------------------------------------------------------------------------//


	/*	Concurrency SAFE	*/

	// Safe to use, however, may yield fleeting results if this object is reassigned 
	// during use
	//--------------------------------------------------------------------------------------//
	inline operator bool() const;

	inline const bool operator==(const ConcurrentSharedPtr<T, MoveType, Allocator>& aOther) const;
	inline const bool operator!=(const ConcurrentSharedPtr<T, MoveType, Allocator>& aOther) const;
	//--------------------------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// Default versions of move constructor / operator. Assumes FROM object is unused
	// by other threads. May be turned safe by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::MoveFast>::value>* = nullptr>
	inline ConcurrentSharedPtr<T, MoveType, Allocator>(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::MoveFast>::value>* = nullptr>
	inline ConcurrentSharedPtr<T, MoveType, Allocator>& operator=(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	//--------------------------------------------------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// May be used for faster performance when TO object is unused by other 
	// threads
	//----------------------------------------------------------------------//
	inline void PrivateAssign(ConcurrentSharedPtr<T, MoveType, Allocator>& aOther);
	inline void PrivateMove(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	//----------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// These methods may be used for faster performance when TO and FROM objects are
	// not in use by other threads
	//----------------------------------------------------------------------//
	inline void UnsafeSwap(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	inline void UnsafeAssign(ConcurrentSharedPtr<T, MoveType, Allocator>& aOther);
	inline void UnsafeMove(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther);
	inline void UnsafeReset();
	inline void UnsafeClaim(T* const aObject);
	template <class Deleter>
	inline void UnsafeClaim(T* const aObject, Deleter&& aDeleter);
	template <class Deleter>
	inline void UnsafeClaim(T* const aObject, Deleter&& aDeleter, const Allocator& aAllocator);
	//----------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// These methods may be used safely so long as no other 
	// thread is reassigning or otherwise altering the state of 
	// this object
	//--------------------------------------------------------------//
	inline const size_type UseCount() const;


	inline T* const operator->();
	inline T& operator*();

	inline const T* const operator->() const;
	inline const T& operator*() const;

	inline const T& operator[](const size_type aIndex) const;
	inline T& operator[](const size_type aIndex);
	//--------------------------------------------------------------//

	/*  Concurrency SAFE  */

	// These adresses are stored locally in this object and are 
	// safe to fetch. Accessing them is safe so long as no other 
	// thread is reassigning or otherwise altering the state of 
	// this object
	//--------------------------------------------------------------//
	const csp::ControlBlock<T, Allocator>* const ControlBlock() const;
	const T* const Object() const;

	csp::ControlBlock<T, Allocator>* const ControlBlock();
	T* const Object();

	inline explicit operator T*();
	inline explicit operator const T*() const;

	inline explicit operator csp::ControlBlock<T, Allocator>*();
	inline explicit operator const csp::ControlBlock<T, Allocator>*() const;
	//--------------------------------------------------------------//

private:

	inline const OWord SafeCopy();
	inline const OWord UnsafeCopy();

	inline void UnsafeStore(const OWord& aFrom);
	inline void SafeStore(const OWord& aFrom);

	inline const OWord UnsafeSteal();
	inline const OWord SafeSteal();

	inline const OWord SafeExchange(const OWord& aTo, const bool aDecrementPrevious);

	template<class Deleter>
	inline const OWord CreateControlBlock(T* const aObject, Deleter&& aDeleter, const Allocator& aAllocator);
	inline const bool IncrementAndTrySwap(OWord& aExpected, const OWord& aDesired);
	inline const bool CASInternal(OWord& aExpected, const OWord& aDesired, const bool aDecrementPrevious);

	constexpr csp::ControlBlock<T, Allocator>* const ToControlBlock(const OWord& aFrom);
	constexpr T* const ToObject(const OWord& aFrom);

	constexpr const csp::ControlBlock<T, Allocator>* const ToControlBlock(const OWord& aFrom) const;
	constexpr const T* const ToObject(const OWord& aFrom) const;

	enum STORAGE_QWORD : uint8_t
	{
		STORAGE_QWORD_CONTROLBLOCKPTR = 0,
		STORAGE_QWORD_OBJECTPTR = 1
	};
	enum STORAGE_WORD : uint8_t
	{
		STORAGE_WORD_COPYREQUEST = 3,
		STORAGE_WORD_REASSIGNINDEX = 7
	};

	template <class T, class MoveType, class Allocator, class ...Args>
	friend ConcurrentSharedPtr<T, MoveType, Allocator> MakeConcurrentShared<T, MoveType, Allocator>(Allocator&, Args&&...);

	static const uint64_t PtrMask = std::numeric_limits<uint64_t>::max() >> 16;

	AtomicOWord myStorage;
};
// Default constructor
template <class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr()
	: myStorage()
{
	static_assert(std::is_same<MoveType, csp::MoveSafe>() | std::is_same<MoveType, csp::MoveFast>(), "Only csp::MoveFast and csp::MoveSafe valid arguments for MoveType");
	static_assert(std::is_same<Allocator::value_type, uint8_t>(), "Value type for allocator must be uint8_t");
}
// Nullptr constructor
template<class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(const std::nullptr_t)
	: ConcurrentSharedPtr()
{
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, MoveType, Allocator> & aOther)
	: ConcurrentSharedPtr<T, MoveType, Allocator>()
{
	PrivateAssign(aOther);
}
// Concurrency UNSAFE
// Default version of move constructor. Assumes FROM object is unused by other threads.
// May be turned safe by template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::MoveFast>::value>*>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, MoveType, Allocator> && aOther)
	: ConcurrentSharedPtr<U, MoveType>()
{
	UnsafeMove(std::move(aOther));
}
// Concurrency SAFE
// Safe version of move constructor. Disabled by default. May be enabled by 
// template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::MoveSafe>::value>*>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, MoveType, Allocator> && aOther)
	: ConcurrentSharedPtr<T, MoveType, Allocator>()
{
	PrivateMove(std::move(aOther));
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(T * aObject)
	: ConcurrentSharedPtr<T, MoveType, Allocator>()
{
	UnsafeClaim(aObject);
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template <class T, class MoveType, class Allocator>
template<class Deleter>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter)
	: ConcurrentSharedPtr<T, MoveType, Allocator>()
{
	UnsafeClaim(aObject, std::forward<Deleter&&>(aDeleter));
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template<class T, class MoveType, class Allocator>
template<class Deleter>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::ConcurrentSharedPtr(T * const aObject, Deleter && aDeleter, const Allocator & aAllocator)
	: ConcurrentSharedPtr<T, MoveType, Allocator>()
{
	UnsafeClaim(aObject, std::forward<Deleter&&>(aDeleter), aAllocator);
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template <class T, class MoveType, class Allocator>
template<class Deleter>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::SafeClaim(T * const aObject, Deleter && aDeleter)
{
	const Allocator alloc;
	SafeClaim(aObject, std::forward<Deleter&&>(aDeleter), alloc);
}
template<class T, class MoveType, class Allocator>
template<class Deleter>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::SafeClaim(T * const aObject, Deleter && aDeleter, const Allocator & aAllocator)
{
	SafeStore(CreateControlBlock(aObject, std::forward<Deleter&&>(aDeleter), aAllocator));
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::SafeClaim(T * const aObject)
{
	SafeClaim(aObject, csp::DefaultDeleter<T>());
}
// Destructor
template <class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::~ConcurrentSharedPtr()
{
	UnsafeReset();
}
// Concurrency UNSAFE
// Default version of move operator. Assumes FROM object is unused by other threads.
// May be turned safe by template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::MoveFast>::value>*>
inline ConcurrentSharedPtr<T, MoveType, Allocator> & ConcurrentSharedPtr<T, MoveType, Allocator>::operator=(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther)
{
	SafeStore(aOther.UnsafeSteal());

	return *this;
}
// Concurrency SAFE
// Safe version of move operator. Disabled by default. May be enabled by 
// template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::MoveSafe>::value>*>
inline ConcurrentSharedPtr<T, MoveType, Allocator> & ConcurrentSharedPtr<T, MoveType, Allocator>::operator=(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther)
{
	SafeStore(aOther.SafeSteal());

	return *this;
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>& ConcurrentSharedPtr<T, MoveType, Allocator>::operator=(ConcurrentSharedPtr<T, MoveType, Allocator> & aOther)
{
	const bool sameobj(&aOther == this);
	const bool sameref(aOther == *this);

	if (sameobj | sameref) {
		return *this;
	}

	const OWord toStore(aOther.SafeCopy());

	SafeStore(toStore);

	return *this;
}
// Concurrency UNSAFE
// May be used for faster performance when TO object is unused by other 
// threads
template<class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::PrivateAssign(ConcurrentSharedPtr<T, MoveType, Allocator>& aOther)
{
	UnsafeStore(aOther.SafeCopy());
}
// Concurrency UNSAFE
// May be used for faster performance when TO object is unused by other 
// threads
template<class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::PrivateMove(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther)
{
	UnsafeStore(aOther.SafeSteal());
}
// Concurrency UNSAFE
// May be used for faster performance when TO and FROM objects are unused
// by other threads
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeSwap(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther)
{
	const OWord otherStorage(aOther.myStorage.MyVal());
	aOther.myStorage.MyVal() = myStorage.MyVal();
	myStorage.MyVal() = otherStorage;
}
// Concurrency UNSAFE
// May be used for faster performance when TO and FROM objects are unused
// by other threads
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeAssign(ConcurrentSharedPtr<T, MoveType, Allocator>& aOther)
{
	UnsafeStore(aOther.UnsafeCopy());
}
// Concurrency UNSAFE
// May be used for faster performance when TO and FROM objects are unused
// by other threads
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeMove(ConcurrentSharedPtr<T, MoveType, Allocator>&& aOther)
{
	UnsafeStore(aOther.UnsafeSteal());
}
// Concurrency UNSAFE
// May be used for faster performance when this object is unused by other
// threads
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeReset()
{
	UnsafeStore(OWord());
}
// Concurrency UNSAFE
// May be used for faster performance when this object is unused by other
// threads
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeClaim(T * const aObject)
{
	UnsafeClaim(aObject, csp::DefaultDeleter<T>());
}
// Concurrency UNSAFE. The Deleter callable has signature void(T* arg)
// May be used for faster performance when this object is unused by other
// threads
template <class T, class MoveType, class Allocator>
template <class Deleter>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeClaim(T * const aObject, Deleter && aDeleter)
{
	const Allocator alloc;
	UnsafeClaim(aObject, std::forward<Deleter&&>(aDeleter), alloc);
}
// Concurrency UNSAFE. The Deleter callable has signature void(T* arg)
// May be used for faster performance when this object is unused by other
// threads
template<class T, class MoveType, class Allocator>
template<class Deleter>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeClaim(T * const aObject, Deleter && aDeleter, const Allocator & aAllocator)
{
	UnsafeStore(CreateControlBlock(aObject, std::forward<Deleter&&>(aDeleter), aAllocator));
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::SafeReset()
{
	SafeStore(OWord());
}
// Concurrency SAFE
template<class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::SafeMove(ConcurrentSharedPtr<T, MoveType, Allocator>&& aFrom)
{
	SafeStore(aFrom.SafeSteal());
}
// Concurrency SAFE
template<class T, class MoveType, class Allocator>
inline const bool ConcurrentSharedPtr<T, MoveType, Allocator>::CompareAndSwap(const T *& aExpected, ConcurrentSharedPtr<T, MoveType, Allocator>& aDesired)
{
	const OWord desired(aDesired.SafeCopy());
	OWord expected(myStorage.Load());

	csp::ControlBlock<T, Allocator>* const otherStorage(ToControlBlock(desired));

	const T* const object(ToObject(expected));

	if (object == aExpected &&
		CASInternal(expected, desired, true)) {
		return true;
	}

	aExpected = object;

	if (otherStorage)
		--(*otherStorage);

	return false;
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline const typename ConcurrentSharedPtr<T, MoveType, Allocator>::size_type ConcurrentSharedPtr<T, MoveType, Allocator>::UseCount() const
{
	if (!operator bool()) {
		return 0;
	}

	return ControlBlock()->RefCount();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::operator bool() const
{
	return static_cast<bool>(Object());
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline T * const ConcurrentSharedPtr<T, MoveType, Allocator>::operator->()
{
	return Object();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline T & ConcurrentSharedPtr<T, MoveType, Allocator>::operator*()
{
	return *(Object());
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline const T * const ConcurrentSharedPtr<T, MoveType, Allocator>::operator->() const
{
	return Object();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline const T & ConcurrentSharedPtr<T, MoveType, Allocator>::operator*() const
{
	return *(Object());
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline const bool ConcurrentSharedPtr<T, MoveType, Allocator>::operator==(const ConcurrentSharedPtr<T, MoveType, Allocator>& aOther) const
{
	return operator->() == aOther.operator->();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline const bool ConcurrentSharedPtr<T, MoveType, Allocator>::operator!=(const ConcurrentSharedPtr<T, MoveType, Allocator>& aOther) const
{
	return !operator==(aOther);
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline const bool operator==(std::nullptr_t /*aNullptr*/, const ConcurrentSharedPtr<T, MoveType, Allocator>& aConcurrentSharedPtr)
{
	return !aConcurrentSharedPtr;
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline const bool operator!=(std::nullptr_t /*aNullptr*/, const ConcurrentSharedPtr<T, MoveType, Allocator>& aConcurrentSharedPtr)
{
	return aConcurrentSharedPtr.operator bool();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline const bool operator==(const ConcurrentSharedPtr<T, MoveType, Allocator>& aConcurrentSharedPtr, std::nullptr_t /*aNullptr*/)
{
	return !aConcurrentSharedPtr.operator bool();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline const bool operator!=(const ConcurrentSharedPtr<T, MoveType, Allocator>& aConcurrentSharedPtr, std::nullptr_t /*aNullptr*/)
{
	return aConcurrentSharedPtr.operator bool();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline const T & ConcurrentSharedPtr<T, MoveType, Allocator>::operator[](const size_type aIndex) const
{
	return (Object())[aIndex];
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline T & ConcurrentSharedPtr<T, MoveType, Allocator>::operator[](const size_type aIndex)
{
	return (Object())[aIndex];
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template <class T, class MoveType, class Allocator>
inline const csp::ControlBlock<T, Allocator>* const ConcurrentSharedPtr<T, MoveType, Allocator>::ControlBlock() const
{
	return ToControlBlock(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template <class T, class MoveType, class Allocator>
inline const T * const ConcurrentSharedPtr<T, MoveType, Allocator>::Object() const
{
	return ToObject(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline csp::ControlBlock<T, Allocator>* const ConcurrentSharedPtr<T, MoveType, Allocator>::ControlBlock()
{
	return ToControlBlock(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline T * const ConcurrentSharedPtr<T, MoveType, Allocator>::Object()
{
	return ToObject(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::operator T*()
{
	return Object();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::operator const T*() const
{
	return Object();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::operator const csp::ControlBlock<T, Allocator>*() const
{
	return ControlBlock();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline ConcurrentSharedPtr<T, MoveType, Allocator>::operator csp::ControlBlock<T, Allocator>*()
{
	return ControlBlock();
}
// ------------------------------------------------------------------------------------

// -------------------------Internals--------------------------------------------------

// ------------------------------------------------------------------------------------
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::SafeStore(const OWord& aFrom)
{
	SafeExchange(aFrom, true);
}
template<class T, class MoveType, class Allocator>
inline const OWord ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeSteal()
{
	const OWord toSteal(myStorage.MyVal());
	myStorage.MyVal() = OWord();

	return toSteal;
}
template<class T, class MoveType, class Allocator>
inline const OWord ConcurrentSharedPtr<T, MoveType, Allocator>::SafeSteal()
{
	return SafeExchange(OWord(), false);
}
template <class T, class MoveType, class Allocator>
inline const bool ConcurrentSharedPtr<T, MoveType, Allocator>::IncrementAndTrySwap(OWord & aExpected, const OWord & aDesired)
{
	const uint16_t initialReassignIndex(aExpected.myWords[STORAGE_WORD_REASSIGNINDEX]);

	csp::ControlBlock<T, Allocator>* const controlBlock(ToControlBlock(aExpected));

	OWord desired(aDesired);
	desired.myWords[STORAGE_WORD_REASSIGNINDEX] = initialReassignIndex + 1;
	desired.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	bool returnValue(false);
	do {
		const uint16_t copyRequests(aExpected.myWords[STORAGE_WORD_COPYREQUEST]);

		if (controlBlock)
			(*controlBlock) += copyRequests;

		if (!myStorage.CompareAndSwap(aExpected, desired)) {
			if (controlBlock)
				(*controlBlock) -= copyRequests;
		}
		else {
			returnValue = !static_cast<bool>(desired.myWords[STORAGE_WORD_COPYREQUEST]);
		}

	} while (
		aExpected.myWords[STORAGE_WORD_COPYREQUEST] &&
		aExpected.myWords[STORAGE_WORD_REASSIGNINDEX] == initialReassignIndex);

	return returnValue;
}
template<class T, class MoveType, class Allocator>
inline const bool ConcurrentSharedPtr<T, MoveType, Allocator>::CASInternal(OWord & aExpected, const OWord & aDesired, const bool aDecrementPrevious)
{
	bool success(false);

	csp::ControlBlock<T, Allocator>* controlBlock(ToControlBlock(aExpected));

	OWord desired(aDesired);
	desired.myWords[STORAGE_WORD_REASSIGNINDEX] = aExpected.myWords[STORAGE_WORD_REASSIGNINDEX] + 1;
	desired.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	if (aExpected.myWords[STORAGE_WORD_COPYREQUEST]) {
		aExpected = myStorage.FetchAddToWord(1, STORAGE_WORD_COPYREQUEST);
		aExpected.myWords[STORAGE_WORD_COPYREQUEST] += 1;

		desired.myWords[STORAGE_WORD_REASSIGNINDEX] = aExpected.myWords[STORAGE_WORD_REASSIGNINDEX] + 1;

		controlBlock = ToControlBlock(aExpected);
		success = IncrementAndTrySwap(aExpected, desired);

		if (controlBlock) {
			(*controlBlock) -= 1 + static_cast<size_type>(aDecrementPrevious & success);
		}
	}
	else {
		success = myStorage.CompareAndSwap(aExpected, desired);

		if (static_cast<bool>(controlBlock) & aDecrementPrevious & success) {
			--(*controlBlock);
		}
	}
	return success;
}
template<class T, class MoveType, class Allocator>
constexpr inline csp::ControlBlock<T, Allocator>* const ConcurrentSharedPtr<T, MoveType, Allocator>::ToControlBlock(const OWord & aFrom)
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<csp::ControlBlock<T, Allocator>* const>(aFrom.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] & ptrMask);
}
template<class T, class MoveType, class Allocator>
constexpr inline T* const ConcurrentSharedPtr<T, MoveType, Allocator>::ToObject(const OWord & aFrom)
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<T* const>(aFrom.myQWords[STORAGE_QWORD_OBJECTPTR] & ptrMask);
}
template<class T, class MoveType, class Allocator>
constexpr inline const csp::ControlBlock<T, Allocator>* const ConcurrentSharedPtr<T, MoveType, Allocator>::ToControlBlock(const OWord & aFrom) const
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<const csp::ControlBlock<T, Allocator>* const>(aFrom.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] & ptrMask);
}
template<class T, class MoveType, class Allocator>
constexpr inline const T * const ConcurrentSharedPtr<T, MoveType, Allocator>::ToObject(const OWord & aFrom) const
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<const T* const>(aFrom.myQWords[STORAGE_QWORD_OBJECTPTR] & ptrMask);
}
template <class T, class MoveType, class Allocator>
inline const OWord ConcurrentSharedPtr<T, MoveType, Allocator>::SafeCopy()
{
	OWord initial(myStorage.FetchAddToWord(1, STORAGE_WORD_COPYREQUEST));
	initial.myWords[STORAGE_WORD_COPYREQUEST] += 1;

	if (ToControlBlock(initial)) {
		OWord expected(initial);
		IncrementAndTrySwap(expected, expected);
	}

	return initial;
}
template <class T, class MoveType, class Allocator>
inline const OWord ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeCopy()
{
	if (operator bool()) {
		++(*ControlBlock());
	}
	return myStorage.MyVal();
}
template <class T, class MoveType, class Allocator>
inline void ConcurrentSharedPtr<T, MoveType, Allocator>::UnsafeStore(const OWord & aFrom)
{
	if (operator bool()) {
		--(*ControlBlock());
	}
	myStorage.MyVal().myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] = aFrom.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR];
	myStorage.MyVal().myQWords[STORAGE_QWORD_OBJECTPTR] = aFrom.myQWords[STORAGE_QWORD_OBJECTPTR];
}
template<class T, class MoveType, class Allocator>
inline const OWord ConcurrentSharedPtr<T, MoveType, Allocator>::SafeExchange(const OWord & aTo, const bool aDecrementPrevious)
{
	OWord expected(myStorage.MyVal());
	while (!CASInternal(expected, aTo, aDecrementPrevious));
	return expected;
}
template <class T, class MoveType, class Allocator>
template<class Deleter>
inline const OWord ConcurrentSharedPtr<T, MoveType, Allocator>::CreateControlBlock(T* const aObject, Deleter&& aDeleter, const Allocator& aAllocator)
{
	csp::ControlBlock<T, Allocator>* controlBlock(nullptr);

	const std::size_t blockSize(sizeof(csp::ControlBlock<T, Allocator>));

	void* block(nullptr);

	Allocator allocator(aAllocator);

	try {
		block = allocator.allocate(blockSize);
		controlBlock = new (block) csp::ControlBlock<T, Allocator>(blockSize, aObject, std::forward<Deleter&&>(aDeleter), allocator);
	}
	catch (...) {
		allocator.deallocate(static_cast<uint8_t*>(block), blockSize);
		aDeleter(aObject);
		throw;
	}

	T* const object(controlBlock->Object());

	OWord returnValue;
	returnValue.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] = reinterpret_cast<uint64_t>(controlBlock);
	returnValue.myQWords[STORAGE_QWORD_OBJECTPTR] = reinterpret_cast<uint64_t>(object);

	return returnValue;
}
namespace csp {
	template <class T, class Allocator>
	class csp::ControlBlock
	{
	public:
		typedef typename ConcurrentSharedPtr<T>::size_type size_type;

		ControlBlock(const std::size_t aBlockSize, T* const aObject, Allocator& aAllocator);
		template <class Deleter>
		ControlBlock(const std::size_t aBlockSize, T* const aObject, Deleter&& aDeleter, Allocator& aAllocator);

		T* const Object();
		const T* const Object() const;

		const size_type RefCount() const;

		const size_type operator--();
		const size_type operator++();
		const size_type operator-=(const size_type aDecrement);
		const size_type operator+=(const size_type aIncrement);

	private:
		friend class ConcurrentSharedPtr<T>;

		void Destroy();

		std::atomic<size_type> myRefCount;
		std::function<void(T*)> myDeleter;
		T* const myPtr;
		const std::size_t myBlockSize;
		Allocator myAllocator;
	};
	template<class T, class Allocator>
	inline ControlBlock<T, Allocator>::ControlBlock(const std::size_t aBlockSize, T* const aObject, Allocator& aAllocator)
		: myRefCount(1)
		, myDeleter([](T* aObject) { aObject->~T(); })
		, myPtr(aObject)
		, myBlockSize(aBlockSize)
		, myAllocator(aAllocator)
	{
	}
	template <class T, class Allocator>
	template<class Deleter>
	inline ControlBlock<T, Allocator>::ControlBlock(const std::size_t aBlockSize, T* const aObject, Deleter&& aDeleter, Allocator& aAllocator)
		: myRefCount(1)
		, myDeleter(std::forward<Deleter&&>(aDeleter))
		, myPtr(aObject)
		, myBlockSize(aBlockSize)
		, myAllocator(aAllocator)
	{
	}
	template <class T, class Allocator>
	inline const typename ControlBlock<T, Allocator>::size_type ControlBlock<T, Allocator>::operator--()
	{
		const size_type refCount(myRefCount.fetch_sub(1, std::memory_order_acq_rel) - 1);
		if (!refCount) {
			Destroy();
		}
		return refCount;
	}
	template <class T, class Allocator>
	inline const typename ControlBlock<T, Allocator>::size_type ControlBlock<T, Allocator>::operator++()
	{
		return myRefCount.fetch_add(1, std::memory_order_acq_rel) + 1;
	}
	template <class T, class Allocator>
	inline const typename ControlBlock<T, Allocator>::size_type ControlBlock<T, Allocator>::operator-=(const size_type aDecrement)
	{
		const size_type refCount(myRefCount.fetch_sub(aDecrement, std::memory_order_acq_rel) - aDecrement);
		if (!refCount) {
			Destroy();
		}
		return refCount;
	}
	template <class T, class Allocator>
	inline const typename ControlBlock<T, Allocator>::size_type ControlBlock<T, Allocator>::operator+=(const size_type aIncrement)
	{
		return myRefCount.fetch_add(aIncrement, std::memory_order_acq_rel) + aIncrement;
	}
	template <class T, class Allocator>
	inline T* const ControlBlock<T, Allocator>::Object()
	{
		return myPtr;
	}
	template<class T, class Allocator>
	inline const T * const ControlBlock<T, Allocator>::Object() const
	{
		return myPtr;
	}
	template <class T, class Allocator>
	inline const typename ControlBlock<T, Allocator>::size_type ControlBlock<T, Allocator>::RefCount() const
	{
		return myRefCount.load(std::memory_order_acquire);
	}
	template <class T, class Allocator>
	inline void ControlBlock<T, Allocator>::Destroy()
	{
		myDeleter(Object());
		(*this).~ControlBlock<T, Allocator>();
		myAllocator.deallocate(reinterpret_cast<uint8_t*>(this), myBlockSize);
	}
	template <class T>
	class DefaultDeleter
	{
	public:
		void operator()(T* const aObject);
	};
	template<class T>
	inline void DefaultDeleter<T>::operator()(T * const aObject)
	{
		delete aObject;
	}
}
// Constructs a ConcurrentSharedPtr object using the default MoveType and allocator
template<class T, class ...Args>
inline ConcurrentSharedPtr<T, csp::MoveDefault> MakeConcurrentShared(Args&& ...aArgs)
{
	csp::DefaultAllocator alloc;
	return MakeConcurrentShared<T, csp::MoveDefault>(alloc, std::forward<Args&&>(aArgs)...);
}
// Constructs a ConcurrentSharedPtr object using explicit MoveType and default allocator
template<class T, class MoveType, class ...Args>
inline ConcurrentSharedPtr<T, MoveType> MakeConcurrentShared(Args&& ...aArgs)
{
	csp::DefaultAllocator alloc;
	return MakeConcurrentShared<T, MoveType>(alloc, std::forward<Args&&>(aArgs)...);
}
// Constructs a ConcurrentSharedPtr object using explicit MoveType and explicit allocator
template<class T, class MoveType, class Allocator, class ...Args>
inline ConcurrentSharedPtr<T, MoveType, Allocator> MakeConcurrentShared(Allocator& aAllocator, Args&& ...aArgs)
{
	ConcurrentSharedPtr<T, MoveType, Allocator> returnValue;

	const std::size_t controlBlockSize(sizeof(csp::ControlBlock<T, Allocator>));
	const std::size_t objectSize(sizeof(T));
	const std::size_t alignment(alignof(T));
	const std::size_t blockSize(controlBlockSize + objectSize + alignment);

	uint8_t* const block(aAllocator.allocate(blockSize));

	const std::size_t controlBlockOffset(0);
	const std::size_t controlBlockEndAddr(reinterpret_cast<std::size_t>(block + controlBlockSize));
	const std::size_t alignmentRemainder(controlBlockEndAddr % alignment);
	const std::size_t alignmentOffset(alignment - (alignmentRemainder ? alignmentRemainder : alignment));
	const std::size_t objectOffset(controlBlockOffset + controlBlockSize + alignmentOffset);

	csp::ControlBlock<T, Allocator> * controlBlock(nullptr);
	T* object(nullptr);
	try {
		object = new (reinterpret_cast<uint8_t*>(block) + objectOffset) T(std::forward<Args&&>(aArgs)...);
		controlBlock = new (reinterpret_cast<uint8_t*>(block) + controlBlockOffset) csp::ControlBlock<T, Allocator>(blockSize, object, aAllocator);
	}
	catch (...) {
		if (object) {
			(*object).~T();
		}
		aAllocator.deallocate(block, blockSize);
		throw;
	}

	returnValue.myStorage.MyVal().myQWords[ConcurrentSharedPtr<T, MoveType, Allocator>::STORAGE_QWORD_CONTROLBLOCKPTR] = reinterpret_cast<uint64_t>(controlBlock);
	returnValue.myStorage.MyVal().myQWords[ConcurrentSharedPtr<T, MoveType, Allocator>::STORAGE_QWORD_OBJECTPTR] = reinterpret_cast<uint64_t>(object);

	return returnValue;
};

#pragma warning(pop)
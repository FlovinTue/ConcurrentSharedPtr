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

class CSMoveSafe;
class CSMoveFast;

typedef CSMoveFast CSMoveDefault;

template <class T>
class DefaultDeleter;
template <class T, class CSMoveType = CSMoveDefault>
class ConcurrentSharedPtr;
template <class T>
class CSControlBlock;

template <class T, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveDefault> MakeConcurrentShared(Args&&...);
template <class T, class CSMoveType, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveType> MakeConcurrentShared(Args&&...);

#pragma warning(push)
#pragma warning(disable : 4201)

template <class T, class CSMoveType>
class ConcurrentSharedPtr
{
public:
	typedef std::size_t size_type;

	inline ConcurrentSharedPtr();
	inline ConcurrentSharedPtr(const std::nullptr_t);
	inline ~ConcurrentSharedPtr();

	template <class Deleter>
	inline explicit ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter);
	inline explicit ConcurrentSharedPtr(T* const aObject);

	/*	Concurrency SAFE	*/

	// May be used from any thread at any time
	//--------------------------------------------------------------------------------------------------//
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType>& aOther);
	inline ConcurrentSharedPtr<T, CSMoveType>& operator=(ConcurrentSharedPtr<T, CSMoveType>& aOther);

	inline void SafeClaim(T* const aObject);
	template <class Deleter>
	inline void SafeClaim(T* const aObject, Deleter&& aDeleter);

	inline void SafeReset();
	inline void SafeMove(ConcurrentSharedPtr<T, CSMoveType>&& aFrom);

	inline const bool CompareAndSwap(const T* &aExpected, ConcurrentSharedPtr<T, CSMoveType>& aDesired);

	//--------------------------------------------------------------------------------------------------//


	/*	Concurrency SAFE	*/

	// Safe versions of move constructor / operator. Disabled by default. May be enabled
	// by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>* = nullptr>
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>* = nullptr>
	inline ConcurrentSharedPtr<T, CSMoveType>& operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	//--------------------------------------------------------------------------------------------------------------//


	/*	Concurrency SAFE	*/

	// Safe to use, however, may yield fleeting results if this object is reassigned 
	// during use
	//--------------------------------------------------------------------------------------//
	inline operator bool() const;

	inline const bool operator==(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const;
	inline const bool operator!=(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const;
	//--------------------------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// Default versions of move constructor / operator. Assumes FROM object is unused
	// by other threads. May be turned safe by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveFast>::value>* = nullptr>
	inline ConcurrentSharedPtr<T, CSMoveType>(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveFast>::value>* = nullptr>
	inline ConcurrentSharedPtr<T, CSMoveType>& operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	//--------------------------------------------------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// May be used for faster performance when TO object is unused by other 
	// threads
	//----------------------------------------------------------------------//
	inline void PrivateAssign(ConcurrentSharedPtr<T, CSMoveType>& aOther);
	inline void PrivateMove(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	//----------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// These methods may be used for faster performance when TO and FROM objects are
	// not in use by other threads
	//----------------------------------------------------------------------//
	inline void UnsafeSwap(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	inline void UnsafeAssign(ConcurrentSharedPtr<T, CSMoveType>& aOther);
	inline void UnsafeMove(ConcurrentSharedPtr<T, CSMoveType>&& aOther);
	inline void UnsafeReset();
	inline void UnsafeClaim(T* const aObject);
	template <class Deleter>
	inline void UnsafeClaim(T* const aObject, Deleter&& aDeleter);
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
	const CSControlBlock<T>* const ControlBlock() const;
	const T* const Object() const;

	CSControlBlock<T>* const ControlBlock();
	T* const Object();

	inline explicit operator T*();
	inline explicit operator const T*() const;

	inline explicit operator CSControlBlock<T>*();
	inline explicit operator const CSControlBlock<T>*() const;
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
	inline const OWord CreateControlBlock(T* const aObject, Deleter&& aDeleter);
	inline const bool IncrementAndTrySwap(OWord& aExpected, const OWord& aDesired);
	inline const bool CASInternal(OWord& aExpected, const OWord& aDesired, const bool aDecrementPrevious);

	constexpr CSControlBlock<T>* const ToControlBlock(const OWord& aFrom);
	constexpr T* const ToObject(const OWord& aFrom);

	constexpr const CSControlBlock<T>* const ToControlBlock(const OWord& aFrom) const;
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

	template <class T, class CSMoveType, class ...Args>
	friend ConcurrentSharedPtr<T, CSMoveType> MakeConcurrentShared<T, CSMoveType, Args>(Args&&...);

	static const uint64_t PtrMask = std::numeric_limits<uint64_t>::max() >> 16;

	AtomicOWord myStorage;
};
// Default constructor
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr()
	: myStorage()
{
	static_assert(std::is_same<CSMoveType, CSMoveSafe>() | std::is_same<CSMoveType, CSMoveFast>(), "Only CSMoveFast and CSMoveSafe valid arguments for CSMoveType");
}
// Nullptr constructor
template<class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(const std::nullptr_t)
	: ConcurrentSharedPtr()
{
}
// Concurrency SAFE
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType> & aOther)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	PrivateAssign(aOther);
}
// Concurrency UNSAFE
// Default version of move constructor. Assumes FROM object is unused by other threads.
// May be turned safe by template argument
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveFast>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType> && aOther)
	: ConcurrentSharedPtr<U, CSMoveType>()
{
	UnsafeMove(std::move(aOther));
}
// Concurrency SAFE
// Safe version of move constructor. Disabled by default. May be enabled by 
// template argument
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType> && aOther)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	PrivateMove(std::move(aOther));
}
// Concurrency SAFE
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(T * aObject)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	UnsafeClaim(aObject);
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template <class T, class CSMoveType>
template<class Deleter>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	UnsafeClaim(aObject, std::forward<Deleter&&>(aDeleter));
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template <class T, class CSMoveType>
template<class Deleter>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeClaim(T * const aObject, Deleter && aDeleter)
{
	SafeStore(CreateControlBlock(aObject, std::forward<Deleter&&>(aDeleter)));
}
// Concurrency SAFE
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeClaim(T * const aObject)
{
	SafeClaim(aObject, DefaultDeleter<T>());
}
// Destructor
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::~ConcurrentSharedPtr()
{
	UnsafeReset();
}
// Concurrency UNSAFE
// Default version of move operator. Assumes FROM object is unused by other threads.
// May be turned safe by template argument
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveFast>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType> & ConcurrentSharedPtr<T, CSMoveType>::operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	SafeStore(aOther.UnsafeSteal());

	return *this;
}
// Concurrency SAFE
// Safe version of move operator. Disabled by default. May be enabled by 
// template argument
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType> & ConcurrentSharedPtr<T, CSMoveType>::operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	SafeStore(aOther.SafeSteal());

	return *this;
}
// Concurrency SAFE
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>& ConcurrentSharedPtr<T, CSMoveType>::operator=(ConcurrentSharedPtr<T, CSMoveType> & aOther)
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
template<class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::PrivateAssign(ConcurrentSharedPtr<T, CSMoveType>& aOther)
{
	UnsafeStore(aOther.SafeCopy());
}
// Concurrency UNSAFE
// May be used for faster performance when TO object is unused by other 
// threads
template<class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::PrivateMove(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	UnsafeStore(aOther.SafeSteal());
}
// Concurrency UNSAFE
// May be used for faster performance when TO and FROM objects are unused
// by other threads
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeSwap(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	const OWord otherStorage(aOther.myStorage.MyVal());
	aOther.myStorage.MyVal() = myStorage.MyVal();
	myStorage.MyVal() = otherStorage;
}
// Concurrency UNSAFE
// May be used for faster performance when TO and FROM objects are unused
// by other threads
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeAssign(ConcurrentSharedPtr<T, CSMoveType>& aOther)
{
	UnsafeStore(aOther.UnsafeCopy());
}
// Concurrency UNSAFE
// May be used for faster performance when TO and FROM objects are unused
// by other threads
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeMove(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	UnsafeStore(aOther.UnsafeSteal());
}
// Concurrency UNSAFE
// May be used for faster performance when this object is unused by other
// threads
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeReset()
{
	UnsafeStore(OWord());
}
// Concurrency UNSAFE
// May be used for faster performance when this object is unused by other
// threads
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeClaim(T * const aObject)
{
	UnsafeClaim(aObject, DefaultDeleter<T>());
}
// Concurrency UNSAFE. The Deleter callable has signature void(T* arg)
// May be used for faster performance when this object is unused by other
// threads
template <class T, class CSMoveType>
template <class Deleter>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeClaim(T * const aObject, Deleter && aDeleter)
{
	UnsafeStore(CreateControlBlock(aObject, std::forward<Deleter&&>(aDeleter)));
}
// Concurrency SAFE
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeReset()
{
	SafeStore(OWord());
}
// Concurrency SAFE
template<class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeMove(ConcurrentSharedPtr<T, CSMoveType>&& aFrom)
{
	SafeStore(aFrom.SafeSteal());
}
// Concurrency SAFE
template<class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::CompareAndSwap(const T *& aExpected, ConcurrentSharedPtr<T, CSMoveType>& aDesired)
{
	const OWord desired(aDesired.SafeCopy());
	OWord expected(myStorage.Load());

	CSControlBlock<T>* const otherStorage(ToControlBlock(desired));

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
template <class T, class CSMoveType>
inline const typename ConcurrentSharedPtr<T, CSMoveType>::size_type ConcurrentSharedPtr<T, CSMoveType>::UseCount() const
{
	if (!operator bool()) {
		return 0;
	}

	return ControlBlock()->RefCount();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::operator bool() const
{
	return static_cast<bool>(Object());
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline T * const ConcurrentSharedPtr<T, CSMoveType>::operator->()
{
	return Object();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline T & ConcurrentSharedPtr<T, CSMoveType>::operator*()
{
	return *(Object());
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline const T * const ConcurrentSharedPtr<T, CSMoveType>::operator->() const
{
	return Object();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline const T & ConcurrentSharedPtr<T, CSMoveType>::operator*() const
{
	return *(Object());
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::operator==(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const
{
	return operator->() == aOther.operator->();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::operator!=(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const
{
	return !operator==(aOther);
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class CSMoveType>
inline const bool operator==(std::nullptr_t /*aNullptr*/, const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr)
{
	return !aConcurrentSharedPtr;
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class CSMoveType>
inline const bool operator!=(std::nullptr_t /*aNullptr*/, const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr)
{
	return aConcurrentSharedPtr.operator bool();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class CSMoveType>
inline const bool operator==(const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr, std::nullptr_t /*aNullptr*/)
{
	return !aConcurrentSharedPtr.operator bool();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class CSMoveType>
inline const bool operator!=(const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr, std::nullptr_t /*aNullptr*/)
{
	return aConcurrentSharedPtr.operator bool();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline const T & ConcurrentSharedPtr<T, CSMoveType>::operator[](const size_type aIndex) const
{
	return (Object())[aIndex];
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class CSMoveType>
inline T & ConcurrentSharedPtr<T, CSMoveType>::operator[](const size_type aIndex)
{
	return (Object())[aIndex];
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template <class T, class CSMoveType>
inline const CSControlBlock<T>* const ConcurrentSharedPtr<T, CSMoveType>::ControlBlock() const
{
	return ToControlBlock(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template <class T, class CSMoveType>
inline const T * const ConcurrentSharedPtr<T, CSMoveType>::Object() const
{
	return ToObject(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class CSMoveType>
inline CSControlBlock<T>* const ConcurrentSharedPtr<T, CSMoveType>::ControlBlock()
{
	return ToControlBlock(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class CSMoveType>
inline T * const ConcurrentSharedPtr<T, CSMoveType>::Object()
{
	return ToObject(myStorage.MyVal());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::operator T*()
{
	return Object();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::operator const T*() const
{
	return Object();
}
template<class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::operator const CSControlBlock<T>*() const
{
	return ControlBlock();
}
template<class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::operator CSControlBlock<T>*()
{
	return ControlBlock();
}
// ------------------------------------------------------------------------------------

// -------------------------Internals--------------------------------------------------

// ------------------------------------------------------------------------------------
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeStore(const OWord& aFrom)
{
	SafeExchange(aFrom, true);
}
template<class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::UnsafeSteal()
{
	const OWord toSteal(myStorage.MyVal());
	myStorage.MyVal() = OWord();

	return toSteal;
}
template<class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::SafeSteal()
{
	return SafeExchange(OWord(), false);
}
template <class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::IncrementAndTrySwap(OWord & aExpected, const OWord & aDesired)
{
	const uint16_t initialReassignIndex(aExpected.myWords[STORAGE_WORD_REASSIGNINDEX]);

	CSControlBlock<T>* const controlBlock(ToControlBlock(aExpected));

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
template<class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::CASInternal(OWord & aExpected, const OWord & aDesired, const bool aDecrementPrevious)
{
	bool success(false);

	CSControlBlock<T>* controlBlock(ToControlBlock(aExpected));

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
template<class T, class CSMoveType>
constexpr inline CSControlBlock<T>* const ConcurrentSharedPtr<T, CSMoveType>::ToControlBlock(const OWord & aFrom)
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<CSControlBlock<T>* const>(aFrom.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] & ptrMask);
}
template<class T, class CSMoveType>
constexpr inline T* const ConcurrentSharedPtr<T, CSMoveType>::ToObject(const OWord & aFrom)
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<T* const>(aFrom.myQWords[STORAGE_QWORD_OBJECTPTR] & ptrMask);
}
template<class T, class CSMoveType>
constexpr inline const CSControlBlock<T>* const ConcurrentSharedPtr<T, CSMoveType>::ToControlBlock(const OWord & aFrom) const
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<const CSControlBlock<T>* const>(aFrom.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] & ptrMask);
}
template<class T, class CSMoveType>
constexpr inline const T * const ConcurrentSharedPtr<T, CSMoveType>::ToObject(const OWord & aFrom) const
{
	const uint64_t ptrMask(std::numeric_limits<uint64_t>::max() >> 16);
	return reinterpret_cast<const T* const>(aFrom.myQWords[STORAGE_QWORD_OBJECTPTR] & ptrMask);
}
template <class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::SafeCopy()
{
	OWord initial(myStorage.FetchAddToWord(1, STORAGE_WORD_COPYREQUEST));
	initial.myWords[STORAGE_WORD_COPYREQUEST] += 1;

	if (ToControlBlock(initial)) {
		OWord expected(initial);
		IncrementAndTrySwap(expected, expected);
	}

	return initial;
}
template <class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::UnsafeCopy()
{
	if (operator bool()) {
		++(*ControlBlock());
	}
	return myStorage.MyVal();
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeStore(const OWord & aFrom)
{
	if (operator bool()) {
		--(*ControlBlock());
	}
	myStorage.MyVal().myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] = aFrom.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR];
	myStorage.MyVal().myQWords[STORAGE_QWORD_OBJECTPTR] = aFrom.myQWords[STORAGE_QWORD_OBJECTPTR];
}
template<class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::SafeExchange(const OWord & aTo, const bool aDecrementPrevious)
{
	OWord expected(myStorage.MyVal());
	while (!CASInternal(expected, aTo, aDecrementPrevious));
	return expected;
}
template <class T, class CSMoveType>
template<class Deleter>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::CreateControlBlock(T* const aObject, Deleter&& aDeleter)
{
	CSControlBlock<T>* controlBlock(nullptr);
	void* block(nullptr);

	try {
		block = ::operator new(sizeof(CSControlBlock<T>));
		controlBlock = new (block) CSControlBlock<T>(aObject, std::forward<Deleter&&>(aDeleter));
	}
	catch (...) {
		::operator delete(block);
		aDeleter(aObject);
		throw;
	}

	T* const object(controlBlock->Object());

	OWord returnValue;
	returnValue.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] = reinterpret_cast<uint64_t>(controlBlock);
	returnValue.myQWords[STORAGE_QWORD_OBJECTPTR] = reinterpret_cast<uint64_t>(object);

	return returnValue;
}
template <class T>
class CSControlBlock
{
public:
	typedef typename ConcurrentSharedPtr<T>::size_type size_type;

	CSControlBlock(T* const aObject);
	template <class Deleter>
	CSControlBlock(T* const aObject, Deleter&& aDeleter);

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

	T* const myPtr;
	std::atomic<size_type> myRefCount;
	std::function<void(T*)> myDeleter;
};
template<class T>
inline CSControlBlock<T>::CSControlBlock(T* const aObject)
	: myPtr(aObject)
	, myRefCount(1)
	, myDeleter([](T* aObject) { aObject->~T(); })
{
}
template <class T>
template<class Deleter>
inline CSControlBlock<T>::CSControlBlock(T* const aObject, Deleter&& aDeleter)
	: myPtr(aObject)
	, myRefCount(1)
	, myDeleter(std::forward<Deleter&&>(aDeleter))
{
}
template <class T>
inline const typename CSControlBlock<T>::size_type CSControlBlock<T>::operator--()
{
	const size_type refCount(--myRefCount);
	if (!refCount) {
		Destroy();
	}
	return refCount;
}
template <class T>
inline const typename CSControlBlock<T>::size_type CSControlBlock<T>::operator++()
{
	return ++myRefCount;
}
template <class T>
inline const typename CSControlBlock<T>::size_type CSControlBlock<T>::operator-=(const size_type aDecrement)
{
	const size_type refCount(myRefCount -= aDecrement);
	if (!refCount) {
		Destroy();
	}
	return refCount;
}
template <class T>
inline const typename CSControlBlock<T>::size_type CSControlBlock<T>::operator+=(const size_type aIncrement)
{
	return myRefCount += aIncrement;
}
template <class T>
inline T* const CSControlBlock<T>::Object()
{
	return myPtr;
}
template<class T>
inline const T * const CSControlBlock<T>::Object() const
{
	return myPtr;
}
template <class T>
inline const typename CSControlBlock<T>::size_type CSControlBlock<T>::RefCount() const
{
	return myRefCount.load(std::memory_order_acquire);
}
template <class T>
inline void CSControlBlock<T>::Destroy()
{
	myDeleter(Object());
	(*this).~CSControlBlock<T>();
	::operator delete (reinterpret_cast<void*>(this));
}
template <class T>
class DefaultDeleter
{
public:
	void operator()(T* aObject);
};
template<class T>
inline void DefaultDeleter<T>::operator()(T * aObject)
{
	delete aObject;
}
// Constructs a ConcurrentSharedPtr object using the default MoveType
template<class T, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveDefault> MakeConcurrentShared(Args&& ...aArgs)
{
	return MakeConcurrentShared<T, CSMoveDefault>(std::forward<Args&&>(aArgs)...);
};
// Constructs a ConcurrentSharedPtr object using explicit MoveType
template<class T, class CSMoveType, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveType> MakeConcurrentShared(Args&& ...aArgs)
{
	ConcurrentSharedPtr<T, CSMoveType> returnValue;

	const size_t controlBlockSize(sizeof(CSControlBlock<T>));
	const size_t objectSize(sizeof(T));
	const size_t alignmentPadding(controlBlockSize % 8);

	void* const block(::operator new(controlBlockSize + objectSize + alignmentPadding));

	const size_t controlBlockOffset(0);
	const size_t objectOffset(controlBlockOffset + controlBlockSize + alignmentPadding);

	CSControlBlock<T> * controlBlock(nullptr);
	T* object(nullptr);
	try {
		object = new (reinterpret_cast<uint8_t*>(block) + objectOffset) T(std::forward<Args&&>(aArgs)...);
		controlBlock = new (reinterpret_cast<uint8_t*>(block) + controlBlockOffset) CSControlBlock<T>(object);
	}
	catch (...) {
		if (object) {
			(*object).~T();
		}
		::operator delete (block);
		throw;
	}

	returnValue.myStorage.MyVal().myQWords[ConcurrentSharedPtr<T, CSMoveType>::STORAGE_QWORD_CONTROLBLOCKPTR] = reinterpret_cast<uint64_t>(controlBlock);
	returnValue.myStorage.MyVal().myQWords[ConcurrentSharedPtr<T, CSMoveType>::STORAGE_QWORD_OBJECTPTR] = reinterpret_cast<uint64_t>(object);

	return returnValue;
};

#pragma warning(pop)
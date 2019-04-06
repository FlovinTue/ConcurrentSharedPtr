#pragma once

//Copyright(c) 2019 Flovin Michaelsen
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <atomic>
#include <functional>
#include <stdint.h>
#include "AtomicOWord.h"

template <class T>
class CSSharedBlock;
template <class T, class CSMoveType>
class ConcurrentSharedPtr;

class CSMoveSafe;
class CSMoveUnsafe;

typedef CSMoveUnsafe CSMoveDefault;

template <class T, class CSMoveType, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveType> MakeConcurrentShared(Args&&... aArgs);
template <class T, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveDefault> MakeConcurrentShared(Args&&... aArgs);

template <class T, class CSMoveType = CSMoveDefault>
class ConcurrentSharedPtr
{
public:
	typedef size_t size_type;

	inline ConcurrentSharedPtr();
	inline ~ConcurrentSharedPtr();

	template <class Deleter>
	inline ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter);
	inline ConcurrentSharedPtr(T* const aObject);


							/*	Concurrency SAFE	*/

	// May be used from any thread at any time
	//--------------------------------------------------------------------------------------------------//
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType>& aOther);								//		
	inline ConcurrentSharedPtr<T, CSMoveType>& operator=(ConcurrentSharedPtr<T, CSMoveType>& aOther);	//
																										//
	inline void SafeClaim(T* const aObject);															//
	template <class Deleter>																			//
	inline void SafeClaim(T* const aObject, Deleter&& aDeleter);										//
																										//
	inline void SafeReset();																			//
	inline void SafeMove(ConcurrentSharedPtr<T>&& aFrom);												//
	//--------------------------------------------------------------------------------------------------//


							/*	Concurrency SAFE	*/

	// Safe to use at all times, however, may yield fleeting results if this object is 
	// reassigned during use
	//--------------------------------------------------------------------------------------//
	inline operator bool() const;															//
																							//
	inline const bool operator==(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const;	//
	inline const bool operator!=(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const;	//
	//--------------------------------------------------------------------------------------//


							/*	Concurrency SAFE	*/

	// Safe versions of move constructor / operator. Disabled by default. May be enabled
	// by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>* = nullptr>	//
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType>&& aOther);										//
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>* = nullptr>	//
	inline ConcurrentSharedPtr<T, CSMoveType>& operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther);				//
	//--------------------------------------------------------------------------------------------------------------//


							/*	Concurrency UNSAFE	*/

	// Default versions of move constructor / operator. Assumes FROM object is unused
	// by other threads. May be turned safe by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveUnsafe>::value>* = nullptr>	//
	inline ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType>&& aOther);										//
	template <class U = T, class V = CSMoveType, std::enable_if_t<std::is_same<V, CSMoveUnsafe>::value>* = nullptr> //
	inline ConcurrentSharedPtr<T, CSMoveType>& operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther);				//
	//--------------------------------------------------------------------------------------------------------------//


							/*	Concurrency UNSAFE	*/

	// These methods may be used for faster performance when TO and FROM objects are
	// not in use by other threads
	//------------------------------------------------------------------------------//
	inline void UnsafeSwap(ConcurrentSharedPtr<T, CSMoveType>&& aOther);			//
	inline void UnsafeAssign(ConcurrentSharedPtr<T, CSMoveType>& aOther);			//
	inline void UnsafeMove(ConcurrentSharedPtr<T, CSMoveType>&& aOther);			//
	inline void UnsafeReset();														//
	inline void UnsafeClaim(T* const aObject);										//
	template <class Deleter>														//
	inline void UnsafeClaim(T* const aObject, Deleter&& aDeleter);					//
	//------------------------------------------------------------------------------//


							/*	Concurrency UNSAFE	*/

	// These methods may be used safely so long as no other 
	// thread is reassigning or otherwise altering the state of 
	// this pointer object
	//----------------------------------------------------------//
	inline const size_type UseCount() const;					//
																//
																//
	inline T* operator->();										//
	inline T& operator*();										//
																//
	inline const T* operator->() const;							//
	inline const T& operator*() const;							//
																//
																//
	inline const T& operator[](const size_type aIndex) const;	//
	inline T& operator[](const size_type aIndex);				//
																//
	const CSSharedBlock<T>* const Shared() const;				//
	const T* const Object() const;								//
	CSSharedBlock<T>* const Shared();							//
	T* const Object();											//
	//----------------------------------------------------------//

private:

	inline const OWord SafeCopy();
	inline const OWord UnsafeCopy();

	inline void UnsafeStore(const OWord& aFrom);
	inline void SafeStore(const OWord& aFrom);

	inline const OWord UnsafeSteal();
	inline const OWord SafeSteal();

	inline void SafeStorePtr(const OWord& aFrom);
	inline void UnsafeStorePtr(const OWord& aFrom);

	inline const OWord SafeExchange(const OWord& aTo, const bool aDecrementPrevious);

	template<class Deleter>
	inline const OWord CreateShared(T* const aObject, Deleter&& aDeleter);
	inline const bool TrySwapShared(OWord& aExpected, const OWord& aDesired);

	enum STORAGE_QWORD : uint8_t
	{
		STORAGE_QWORD_SHAREDBLOCKPTR = 0,
		STORAGE_QWORD_OBJECTBLOCKPTR = 0
	};
	enum STORAGE_DWORD : uint8_t
	{
		STORAGE_DWORD_REASSIGNINDEX = 2
	};
	enum STORAGE_WORD : uint8_t
	{
		STORAGE_WORD_COPYREQUEST = 6,
		STORAGE_WORD_UNSAFEFLAG = 7
	};

	template <class U = T, class CSMoveType, class ...Args>
	friend ConcurrentSharedPtr<U, CSMoveType> MakeConcurrentShared<U, CSMoveType, Args>(Args&&...);

	AtomicOWord myObjectStore;
	AtomicOWord mySharedStore;
	T* const &myPtr;
};
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr()
	: mySharedStore()
	, myPtr(reinterpret_cast<T*&>(myObjectStore.MyVal().myQWords[STORAGE_QWORD_OBJECTBLOCKPTR]))
{
	static_assert(std::is_same<CSMoveType, CSMoveSafe>() | std::is_same<CSMoveType, CSMoveUnsafe>(), "Only CSMoveSafe and CSMoveUnsafe valid arguments for CSMoveType");
}
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType> & aOther)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	UnsafeStore(aOther.SafeCopy());
}
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveUnsafe>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType> && aOther)
	: ConcurrentSharedPtr<U, CSMoveType>()
{
	UnsafeStore(aOther.UnsafeSteal());
}
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(ConcurrentSharedPtr<T, CSMoveType> && aOther)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	UnsafeStore(aOther.SafeSteal());
}
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(T * aObject)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	UnsafeClaim(aObject);
}
template <class T, class CSMoveType>
template<class Deleter>
inline ConcurrentSharedPtr<T, CSMoveType>::ConcurrentSharedPtr(T* const aObject, Deleter&& aDeleter)
	: ConcurrentSharedPtr<T, CSMoveType>()
{
	UnsafeClaim(aObject, std::forward<Deleter&&>(aDeleter));
}
template <class T, class CSMoveType>
template<class Deleter>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeClaim(T * const aObject, Deleter && aDeleter)
{
	const OWord toStore(CreateShared(aObject, std::forward<Deleter&&>(aDeleter)));
	SafeStore(toStore);
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeClaim(T * const aObject)
{
	auto deleter = [](T* aObject)
	{
		delete aObject;
	};
	SafeClaim(aObject, std::move(deleter));
}
template <class T, class CSMoveType>
template<class Deleter>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::CreateShared(T* const aObject, Deleter&& aDeleter)
{
	CSSharedBlock<T>* shared(nullptr);
	void* block(nullptr);

	try {
		block = ::operator new(sizeof(CSSharedBlock<T>));
		shared = new (block) CSSharedBlock<T>(aObject, std::forward<Deleter&&>(aDeleter));
	}
	catch (...) {
		::operator delete(block);
		aDeleter(aObject);
		throw;
	}

	uint64_t const sharedAsInteger(reinterpret_cast<uint64_t>(shared));

	OWord returnValue;
	returnValue.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR] = sharedAsInteger;

	return returnValue;
}
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::~ConcurrentSharedPtr()
{
	UnsafeReset();
}
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveUnsafe>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType> & ConcurrentSharedPtr<T, CSMoveType>::operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	SafeStore(aOther.UnsafeSteal());

	return *this;
}
template <class T, class CSMoveType>
template <class U, class V, std::enable_if_t<std::is_same<V, CSMoveSafe>::value>*>
inline ConcurrentSharedPtr<T, CSMoveType> & ConcurrentSharedPtr<T, CSMoveType>::operator=(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	SafeStore(aOther.SafeSteal());

	return *this;
}
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
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeSwap(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	const OWord otherShared(aOther.mySharedStore.MyVal());
	const OWord otherObject(aOther.myObjectStore.MyVal());

	aOther.mySharedStore.MyVal() = mySharedStore.MyVal();
	aOther.myObjectStore.MyVal() = myObjectStore.MyVal();

	mySharedStore.MyVal() = otherShared;
	myObjectStore.MyVal() = otherObject;
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeAssign(ConcurrentSharedPtr<T, CSMoveType>& aOther)
{
	UnsafeStore(aOther.UnsafeCopy());
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeMove(ConcurrentSharedPtr<T, CSMoveType>&& aOther)
{
	UnsafeStore(aOther.UnsafeSteal());
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeReset()
{
	UnsafeStore(OWord());
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeClaim(T * const aObject)
{
	auto deleter = [](T* aObject)
	{
		delete aObject;
	};
	UnsafeClaim(aObject, std::move(deleter));
}
template <class T, class CSMoveType>
template<class Deleter>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeClaim(T * const aObject, Deleter && aDeleter)
{
	const OWord toStore(CreateShared(aObject, std::forward<Deleter&&>(aDeleter)));
	UnsafeStore(toStore);
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeReset()
{
	SafeStore(OWord());
}
template<class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeMove(ConcurrentSharedPtr<T>&& aFrom)
{
	SafeStore(aFrom.SafeSteal());
}
template <class T, class CSMoveType>
inline const typename ConcurrentSharedPtr<T, CSMoveType>::size_type ConcurrentSharedPtr<T, CSMoveType>::UseCount() const
{
	if (operator bool()) {
		return 0;
	}

	return Shared()->RefCount();
}
template <class T, class CSMoveType>
inline ConcurrentSharedPtr<T, CSMoveType>::operator bool() const
{
	return static_cast<bool>(myPtr);
}
template <class T, class CSMoveType>
inline T * ConcurrentSharedPtr<T, CSMoveType>::operator->()
{
	return myPtr;
}

template <class T, class CSMoveType>
inline T & ConcurrentSharedPtr<T, CSMoveType>::operator*()
{
	return *myPtr;
}

template <class T, class CSMoveType>
inline const T * ConcurrentSharedPtr<T, CSMoveType>::operator->() const
{
	return myPtr;
}

template <class T, class CSMoveType>
inline const T & ConcurrentSharedPtr<T, CSMoveType>::operator*() const
{
	return *myPtr;
}
template <class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::operator==(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const
{
	return myPtr == aOther.myPtr;
}
template <class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::operator!=(const ConcurrentSharedPtr<T, CSMoveType>& aOther) const
{
	return !operator==(aOther);
}
template <class T, class CSMoveType>
inline const bool operator==(std::nullptr_t /*aNullptr*/, const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr)
{
	return !aConcurrentSharedPtr;
}
template <class T, class CSMoveType>
inline const bool operator!=(std::nullptr_t /*aNullptr*/, const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr)
{
	return aConcurrentSharedPtr.operator bool();
}
template <class T, class CSMoveType>
inline const bool operator==(const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr, std::nullptr_t /*aNullptr*/)
{
	return !aConcurrentSharedPtr.operator bool();
}
template <class T, class CSMoveType>
inline const bool operator!=(const ConcurrentSharedPtr<T, CSMoveType>& aConcurrentSharedPtr, std::nullptr_t /*aNullptr*/)
{
	return aConcurrentSharedPtr.operator bool();
}
template <class T, class CSMoveType>
inline const T & ConcurrentSharedPtr<T, CSMoveType>::operator[](const size_type aIndex) const
{
	return myPtr[aIndex];
}
template <class T, class CSMoveType>
inline T & ConcurrentSharedPtr<T, CSMoveType>::operator[](const size_type aIndex)
{
	return myPtr[aIndex];
}
template <class T, class CSMoveType>
inline const CSSharedBlock<T>* const ConcurrentSharedPtr<T, CSMoveType>::Shared() const
{
	return reinterpret_cast<const CSSharedBlock<T>*>(mySharedStore.MyVal().myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]);
}
template <class T, class CSMoveType>
inline const T * const ConcurrentSharedPtr<T, CSMoveType>::Object() const
{
	return reinterpret_cast<const T*>(myObjectStore.MyVal().myQWords[STORAGE_QWORD_OBJECTBLOCKPTR]);
}
template<class T, class CSMoveType>
inline CSSharedBlock<T>* const ConcurrentSharedPtr<T, CSMoveType>::Shared()
{
	return reinterpret_cast<CSSharedBlock<T>*>(mySharedStore.MyVal().myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]);
}
template<class T, class CSMoveType>
inline T * const ConcurrentSharedPtr<T, CSMoveType>::Object()
{
	return reinterpret_cast<T*>(myObjectStore.MyVal().myQWords[STORAGE_QWORD_OBJECTBLOCKPTR]);
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeStore(const OWord& aFrom)
{
	SafeExchange(aFrom, true);
}
template<class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::UnsafeSteal()
{
	const OWord toSteal(mySharedStore.MyVal());
	mySharedStore.MyVal() = OWord();
	myObjectStore.MyVal() = OWord();

	return toSteal;
}
template<class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::SafeSteal()
{
	return SafeExchange(OWord(), false);
}
template <class T, class CSMoveType>
inline const bool ConcurrentSharedPtr<T, CSMoveType>::TrySwapShared(OWord & aExpected, const OWord & aDesired)
{
	const OWord initial(aExpected);

	CSSharedBlock<T>* const shared(reinterpret_cast<CSSharedBlock<T>*>(initial.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]));

	OWord desired;

	bool returnValue(false);
	do {
		desired = aExpected;
		desired.myWords[STORAGE_WORD_COPYREQUEST] -= 1;
		if (!desired.myWords[STORAGE_WORD_COPYREQUEST]) {
			desired.myDWords[STORAGE_DWORD_REASSIGNINDEX] += 1;
			desired.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR] = aDesired.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR];
		}

		if (shared)
			++(*shared);

		if (!mySharedStore.CompareAndSwap(aExpected, desired)) {
			if (shared)
				--(*shared);
		}
		else {
			aExpected = desired;
			returnValue = !desired.myWords[STORAGE_WORD_COPYREQUEST];
		}

	} while (
		aExpected.myWords[STORAGE_WORD_COPYREQUEST] &&
		aExpected.myDWords[STORAGE_DWORD_REASSIGNINDEX] == initial.myDWords[STORAGE_DWORD_REASSIGNINDEX]);

	return returnValue;
}
template <class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::SafeCopy()
{
	OWord initial(mySharedStore.FetchAddToWord(1, STORAGE_WORD_COPYREQUEST));
	initial.myWords[STORAGE_WORD_COPYREQUEST] += 1;

	if (initial.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]) {
		OWord expected(initial);
		TrySwapShared(expected, expected);
	}

	return initial;
}
template <class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::UnsafeCopy()
{
	if (operator bool()) {
		++(*Shared());
	}
	return mySharedStore.MyVal();
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeStore(const OWord & aFrom)
{
	if (operator bool()) {
		--(*Shared());
	}
	mySharedStore.MyVal().myQWords[STORAGE_QWORD_SHAREDBLOCKPTR] = aFrom.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR];
	UnsafeStorePtr(mySharedStore.MyVal());
}
template <class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::SafeStorePtr(const OWord & aFromShared)
{
	CSSharedBlock<T>* const shared(reinterpret_cast<CSSharedBlock<T>*>(aFromShared.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]));
	T* const object(shared ? shared->Object() : nullptr);

	OWord desired;
	desired.myQWords[STORAGE_QWORD_OBJECTBLOCKPTR] = reinterpret_cast<uint64_t>(object);
	desired.myDWords[STORAGE_DWORD_REASSIGNINDEX] = aFromShared.myDWords[STORAGE_DWORD_REASSIGNINDEX];

	OWord expected(myObjectStore.MyVal());

	const uint32_t veryHigh(std::numeric_limits<uint32_t>::max() / 2);
	while (veryHigh < (expected.myDWords[STORAGE_DWORD_REASSIGNINDEX] - aFromShared.myDWords[STORAGE_DWORD_REASSIGNINDEX])
		&& !myObjectStore.CompareAndSwap(expected, desired));
}
template<class T, class CSMoveType>
inline void ConcurrentSharedPtr<T, CSMoveType>::UnsafeStorePtr(const OWord & aFromShared)
{
	CSSharedBlock<T>* const shared(reinterpret_cast<CSSharedBlock<T>*>(aFromShared.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]));
	T* const object(shared ? shared->Object() : nullptr);

	OWord desired;
	desired.myQWords[STORAGE_QWORD_OBJECTBLOCKPTR] = reinterpret_cast<uint64_t>(object);
	desired.myDWords[STORAGE_DWORD_REASSIGNINDEX] = aFromShared.myDWords[STORAGE_DWORD_REASSIGNINDEX];

	myObjectStore.MyVal() = desired;
}
template<class T, class CSMoveType>
inline const OWord ConcurrentSharedPtr<T, CSMoveType>::SafeExchange(const OWord & aTo, const bool aDecrementPrevious)
{
	OWord previous;
	for (bool success(false); !success;) {

		OWord expected(mySharedStore.FetchAddToWord(1, STORAGE_WORD_COPYREQUEST));
		expected.myWords[STORAGE_WORD_COPYREQUEST] += 1;

		previous = expected;

		CSSharedBlock<T>* const shared(reinterpret_cast<CSSharedBlock<T>*>(expected.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR]));

		OWord desired;
		desired.myDWords[STORAGE_DWORD_REASSIGNINDEX] = expected.myDWords[STORAGE_DWORD_REASSIGNINDEX];
		desired.myDWords[STORAGE_DWORD_REASSIGNINDEX] += 1;
		desired.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR] = aTo.myQWords[STORAGE_QWORD_SHAREDBLOCKPTR];

		SafeStorePtr(desired);

		if (TrySwapShared(expected, desired)) {
			success = true;
		}
		if (shared) {
			(*shared) -= 1 + static_cast<size_type>(aDecrementPrevious & success);
		}
	}
	return previous;
}
template <class T>
class CSSharedBlock
{
public:
	typedef typename ConcurrentSharedPtr<T>::size_type size_type;

	CSSharedBlock(T* const aObject);
	template <class Deleter>
	CSSharedBlock(T* const aObject, Deleter&& aDeleter);

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
inline CSSharedBlock<T>::CSSharedBlock(T* const aObject)
	: myPtr(aObject)
	, myRefCount(1)
	, myDeleter([](T* aObject) { (*aObject).~T(); })
{
}
template <class T>
template<class Deleter>
inline CSSharedBlock<T>::CSSharedBlock(T* const aObject, Deleter&& aDeleter)
	: myPtr(aObject)
	, myRefCount(1)
	, myDeleter(std::forward<Deleter&&>(aDeleter))
{
}
template <class T>
inline const typename CSSharedBlock<T>::size_type CSSharedBlock<T>::operator--()
{
	const size_type refCount(--myRefCount);
	if (!refCount) {
		Destroy();
	}
	return refCount;
}
template <class T>
inline const typename CSSharedBlock<T>::size_type CSSharedBlock<T>::operator++()
{
	return ++myRefCount;
}
template <class T>
inline const typename CSSharedBlock<T>::size_type CSSharedBlock<T>::operator-=(const size_type aDecrement)
{
	const size_type refCount(myRefCount -= aDecrement);
	if (!refCount) {
		Destroy();
	}
	return refCount;
}
template <class T>
inline const typename CSSharedBlock<T>::size_type CSSharedBlock<T>::operator+=(const size_type aIncrement)
{
	return myRefCount += aIncrement;
}
template <class T>
inline T* const CSSharedBlock<T>::Object()
{
	return myPtr;
}
template<class T>
inline const T * const CSSharedBlock<T>::Object() const
{
	return myPtr;
}
template <class T>
inline const typename CSSharedBlock<T>::size_type CSSharedBlock<T>::RefCount() const
{
	return myRefCount._My_val;
}
template <class T>
inline void CSSharedBlock<T>::Destroy()
{
	myDeleter(Object());
	(*this).~CSSharedBlock<T>();
	::operator delete (reinterpret_cast<void*>(this));
}
template<class T, class CSMoveType, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveType> MakeConcurrentShared(Args&& ...aArgs)
{
	ConcurrentSharedPtr<T, CSMoveType> returnValue;

	const size_t sharedSize(sizeof(CSSharedBlock<T>));
	const size_t objectSize(sizeof(T));
	const size_t alignmentPadding(8);

	void* const block(::operator new(sharedSize + objectSize + alignmentPadding));

	const size_t sharedOffset(0);
	const size_t objectOffset(sharedOffset + sharedSize);

	CSSharedBlock<T> * shared(nullptr);
	T* object(nullptr);
	try {
		object = new (reinterpret_cast<uint8_t*>(block) + objectOffset) T(std::forward<Args&&>(aArgs)...);
		shared = new (reinterpret_cast<uint8_t*>(block) + sharedOffset) CSSharedBlock<T>(object);
	}
	catch (...) {
		if (object) {
			(*object).~T();
		}
		::operator delete (block);
		throw;
	}

	uint64_t const sharedAsInteger(reinterpret_cast<uint64_t>(shared));
	uint64_t const objectAsInteger(reinterpret_cast<uint64_t>(shared->Object()));

	returnValue.mySharedStore.MyVal().myQWords[ConcurrentSharedPtr<T, CSMoveType>::STORAGE_QWORD_SHAREDBLOCKPTR] = sharedAsInteger;
	returnValue.myObjectStore.MyVal().myQWords[ConcurrentSharedPtr<T, CSMoveType>::STORAGE_QWORD_OBJECTBLOCKPTR] = objectAsInteger;

	return returnValue;
};
template<class T, class ...Args>
inline ConcurrentSharedPtr<T, CSMoveDefault> MakeConcurrentShared(Args&& ...aArgs)
{
	return MakeConcurrentShared<T, CSMoveDefault>(std::forward<Args&&>(aArgs)...);
};
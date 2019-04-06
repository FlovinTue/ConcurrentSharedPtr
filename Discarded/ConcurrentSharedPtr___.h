#pragma once

#include <atomic>
#include <limits>
#include <assert.h>
#include <thread>

#ifndef MAKE_UNIQUE_NAME 
#define _CONCAT_(a,b)  a##b
#define _EXPAND_AND_CONCAT_(a, b) _CONCAT_(a,b)
#define MAKE_UNIQUE_NAME(prefix) _EXPAND_AND_CONCAT_(prefix, __COUNTER__)
#endif

#define CSP_PADDING(bytes) const uint8_t MAKE_UNIQUE_NAME(trash)[bytes] {}

class OctWord
{
public:
	OctWord();

	void Store(uint64_t* const &aOctWord);
	uint64_t* const Load() const;

	volatile __int64 myStorage[3];
	volatile __int64* const myAlignedRef;
};
OctWord::OctWord()
	: myStorage{ 0 }
	, myAlignedRef(myStorage + (reinterpret_cast<uint64_t>(&myStorage[0]) % 16) / sizeof(uint64_t))
{

}

void OctWord::Store(uint64_t* const&aOctWord)
{
	__int64 desiredA = static_cast<__int64>(aOctWord[0]);
	__int64 desiredB = static_cast<__int64>(aOctWord[1]);
	__int64 expected[2] = { 0,0 };
	_InterlockedCompareExchange128(myAlignedRef, desiredB, desiredA, expected);
}

template <class T>
class ConcurrentSharedPtr;
template <class T>
class CSPControlBlock;

template <class T, class ...Args>
inline ConcurrentSharedPtr<T> MakeConcurrentSharedPtr(Args&&... aArgs);

template <class T>
class ConcurrentSharedPtr
{
public:
	typedef size_t size_type;

	// Concurrency safe -----------------------------
	ConcurrentSharedPtr();
	ConcurrentSharedPtr(ConcurrentSharedPtr& aOther);
	ConcurrentSharedPtr(ConcurrentSharedPtr&& aOther);

	ConcurrentSharedPtr& operator=(ConcurrentSharedPtr&& aOther);
	ConcurrentSharedPtr& operator=(ConcurrentSharedPtr& aOther);

	void Reset();
	// ----------------------------------------------

	// Concurrency unsafe ---------------------------
	~ConcurrentSharedPtr();

	operator bool() const;

	T* operator->();
	T& operator*();

	const T* operator->() const;
	const T& operator*() const;
	// ----------------------------------------------

private:
	template <class U = T, class ...Args>
	friend ConcurrentSharedPtr<U> MakeConcurrentSharedPtr<U, Args>(Args&&...);

	static const uint64_t PointerMask = ~uint64_t(std::numeric_limits<uint16_t>::max());
	static const uint64_t CounterMask = uint64_t(std::numeric_limits<uint16_t>::max());

	void UnregisterFrom(CSPControlBlock<T>* const aControlBlock);

	CSPControlBlock<T>* const ToControlBlock(const uint64_t aBlock) const;
	const uint16_t ToCounter(const uint64_t aBlock) const;

	T* myPtr;

	union
	{
		uint16_t myClientCount;
		std::atomic<uint64_t> myStorageBlock;
	};
};
template<class T>
inline ConcurrentSharedPtr<T>::ConcurrentSharedPtr()
	: myPtr(nullptr)
	, myStorageBlock(0)
{
}

template<class T>
inline ConcurrentSharedPtr<T>::ConcurrentSharedPtr(ConcurrentSharedPtr<T> & aOther)
	: ConcurrentSharedPtr()
{
	operator=(aOther);
}

template<class T>
inline ConcurrentSharedPtr<T>::ConcurrentSharedPtr(ConcurrentSharedPtr && aOther)
	: ConcurrentSharedPtr()
{
	operator=(std::move(aOther));
}

template<class T>
inline ConcurrentSharedPtr<T>::~ConcurrentSharedPtr()
{
	Reset();
}
template<class T>
inline ConcurrentSharedPtr<T> & ConcurrentSharedPtr<T>::operator=(ConcurrentSharedPtr<T> && aOther)
{
	return operator=(aOther);
}

template<class T>
inline ConcurrentSharedPtr<T> & ConcurrentSharedPtr<T>::operator=(ConcurrentSharedPtr & aOther)
{
	const uint64_t otherStorageBlock = ++aOther.myStorageBlock;
	const uint64_t otherPointerPortion(otherStorageBlock & PointerMask);
	const uint64_t newStorageBlock(uint64_t(myClientCount) | otherPointerPortion);

	CSPControlBlock<T>* const controlBlock(ToControlBlock(otherStorageBlock)); 
	if (controlBlock) {
		++(*controlBlock);
		myPtr = controlBlock->Object();
	}

	const uint64_t currentStorageBlock(myStorageBlock.exchange(newStorageBlock));
	--aOther.myStorageBlock;

	UnregisterFrom(ToControlBlock(currentStorageBlock));

	return *this;
}
template<class T>
inline void ConcurrentSharedPtr<T>::Reset()
{
	myPtr = nullptr;

	const uint64_t desired(0);
	uint64_t expected(0);
	for (expected = (myStorageBlock.load());; expected = myStorageBlock.load()) {
		const uint16_t counter(ToCounter(expected));
		if (!counter) {
			if (myStorageBlock.compare_exchange_strong(expected, desired)) {
				break;
			}
		}
		else {
			std::this_thread::yield();
		}
	}
	UnregisterFrom(ToControlBlock(expected));
}
template<class T>
inline ConcurrentSharedPtr<T>::operator bool() const
{
	return static_cast<bool>(myPtr);
}
template<class T>
inline T * ConcurrentSharedPtr<T>::operator->()
{
	return myPtr;
}

template<class T>
inline T & ConcurrentSharedPtr<T>::operator*()
{
	return *myPtr;
}

template<class T>
inline const T * ConcurrentSharedPtr<T>::operator->() const
{
	return operator->();
}

template<class T>
inline const T & ConcurrentSharedPtr<T>::operator*() const
{
	return operator*();
}
template<class T>
inline void ConcurrentSharedPtr<T>::UnregisterFrom(CSPControlBlock<T>* const aControlBlock)
{
	if (aControlBlock) {
		--(*aControlBlock);
	}
}
template<class T>
inline CSPControlBlock<T>* const ConcurrentSharedPtr<T>::ToControlBlock(const uint64_t aBlock) const
{
	return reinterpret_cast<CSPControlBlock<T>*>(aBlock >> 16);
}
template<class T>
inline const uint16_t ConcurrentSharedPtr<T>::ToCounter(const uint64_t aBlock) const
{
	return static_cast<uint16_t>(uint64_t(aBlock & CounterMask));
}
template <class T>
class CSPControlBlock
{
public:
	typedef typename ConcurrentSharedPtr<T>::size_type size_type;

	template <class ...Args>
	CSPControlBlock(Args&&... aArgs);

	T* Object();

	void operator--();
	void operator++();

private:
	void Destroy();

	CSP_PADDING(8);
	T myObject;
	CSP_PADDING(64 - (sizeof(T) % 64));
	std::atomic<size_type> myRefCount;
};

template<class T>
template<class ...Args>
inline CSPControlBlock<T>::CSPControlBlock(Args && ...aArgs)
	: myObject(aArgs...)
	, myRefCount(1)
{

}
template<class T>
inline void CSPControlBlock<T>::operator--()
{
	const size_type referenceCount(--myRefCount);
	if (!referenceCount) {
		Destroy();
	}
}
template<class T>
inline void CSPControlBlock<T>::operator++()
{
	++myRefCount;
}
template<class T>
inline T * CSPControlBlock<T>::Object()
{
	return &myObject;
}
template<class T>
inline void CSPControlBlock<T>::Destroy()
{
	delete this;
}

template<class T, class ...Args>
inline ConcurrentSharedPtr<T> MakeConcurrentSharedPtr(Args && ...aArgs)
{
	ConcurrentSharedPtr<T> returnValue;

	CSPControlBlock<T>* const controlBlock = new CSPControlBlock<T>(std::forward<Args>(aArgs)...);

	uint64_t const toStore(reinterpret_cast<uint64_t>(controlBlock));
	returnValue.myStorageBlock = toStore;
	returnValue.myStorageBlock = returnValue.myStorageBlock._My_val << 16;
	returnValue.myPtr = returnValue.ToControlBlock(returnValue.myStorageBlock._My_val)->Object();

	return std::move(returnValue);
}



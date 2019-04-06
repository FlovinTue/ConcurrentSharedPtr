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

	void UnregisterFrom(CSPControlBlock<T>* const aControlBlock);

	static const uint64_t ControlBlockMask = ~uint64_t(std::numeric_limits<uint16_t>::max());
	static const uint64_t ClientIndexMask = uint64_t(std::numeric_limits<uint8_t>::max());
	static const uint64_t ReassignmentIndexMask = uint64_t(std::numeric_limits<uint8_t>::max() << 8);

	CSPControlBlock<T>* const ToControlBlock(const uint64_t aBlock) const;
	const uint8_t ToClientIndex(const uint64_t aBlock) const;
	const uint8_t ToReassignmentIndex(const uint64_t aBlock) const;

	uint64_t IncrementSwapPointer(const uint64_t aPointer);

	T* myPtr;

	std::atomic<uint64_t> myStorageBlock;
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

	CSPControlBlock<T>* const controlBlock(ToControlBlock(otherStorageBlock));
	if (controlBlock) {
		++(*controlBlock);
		myPtr = controlBlock->Object();
	}

	const uint64_t previousBlock(IncrementSwapPointer(otherStorageBlock & ControlBlockMask));

	const uint8_t previousReassignmentIndex(ToReassignmentIndex(otherStorageBlock));
	const uint8_t clientIndex(ToClientIndex(otherStorageBlock));
	const uint8_t reassignmentIndex(ToReassignmentIndex(aOther.myStorageBlock.load()));

	const bool reassignmentMatch(!(reassignmentIndex ^ previousReassignmentIndex));
	const bool clientReassignmentMatch(!(previousReassignmentIndex ^ clientIndex));

	const bool doDeferredUnregister(reassignmentMatch & clientReassignmentMatch);
	if (doDeferredUnregister) {
		UnregisterFrom(controlBlock);
	}

	UnregisterFrom(ToControlBlock(previousBlock));

	return *this;
}
template<class T>
inline void ConcurrentSharedPtr<T>::Reset()
{
	myPtr = nullptr;

	const uint64_t previous(IncrementSwapPointer(0));

	//UnregisterFrom(ToControlBlock(expected));
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
inline const uint8_t ConcurrentSharedPtr<T>::ToClientIndex(const uint64_t aBlock) const
{
	return static_cast<uint8_t>(aBlock & ClientIndexMask);
}
template<class T>
inline const uint8_t ConcurrentSharedPtr<T>::ToReassignmentIndex(const uint64_t aBlock) const
{
	return static_cast<uint8_t>((aBlock & ReassignmentIndexMask) >> 8);
}
template<class T>
inline uint64_t ConcurrentSharedPtr<T>::IncrementSwapPointer(const uint64_t aPointer)
{
	uint64_t expected(0);
	for (;;) {
		expected = myStorageBlock.load();
		const uint64_t indexPortion(expected & ~ControlBlockMask);
		const uint64_t indexPortionIncremented(indexPortion + uint64_t(1 << 8));
		//const uint64_t desired(aBlock & indexPortionIncremented);

		//if (myStorageBlock.compare_exchange_strong(expected, desired)) {
		//	break;
		//}
	}
	return expected;
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



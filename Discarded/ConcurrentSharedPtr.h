#pragma once

#include <atomic>
#include <limits>
#include <assert.h>

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
	typedef uint32_t size_type;

	ConcurrentSharedPtr();
	ConcurrentSharedPtr(ConcurrentSharedPtr& aOther);
	ConcurrentSharedPtr(ConcurrentSharedPtr&& aOther);
	~ConcurrentSharedPtr();

	ConcurrentSharedPtr& operator=(ConcurrentSharedPtr&& aOther);
	ConcurrentSharedPtr& operator=(ConcurrentSharedPtr& aOther);

	void Reset();

	operator bool() const;

	T* operator->();
	T& operator*();

	const T* operator->() const;
	const T& operator*() const;

private:
	template <class U = T, class ...Args>
	friend ConcurrentSharedPtr<U> MakeConcurrentSharedPtr<U, Args>(Args&&...);

	static const uint64_t PointerMask = ~uint64_t(std::numeric_limits<uint16_t>::max());
	static const uint64_t CounterMask = uint64_t(std::numeric_limits<uint16_t>::max());

	void Swap(ConcurrentSharedPtr<T>& aOther);

	void UnregisterFrom(uint64_t aControlBlockLocal);

	T* myPtr;

	CSPControlBlock<T>* const ControlBlock() const;

	union
	{
		uint16_t myExpectedCount;
		std::atomic<uint64_t> myControlBlock;
	};
};
template<class T>
inline ConcurrentSharedPtr<T>::ConcurrentSharedPtr()
	: myPtr(nullptr)
	, myControlBlock(0)
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
	Swap(aOther);

	return *this;
}

template<class T>
inline ConcurrentSharedPtr<T> & ConcurrentSharedPtr<T>::operator=(ConcurrentSharedPtr & aOther)
{
	const uint64_t controlblock = ++aOther.myControlBlock;

	assert((controlblock & CounterMask) != 0 && "Max copies exceeded");

	const uint64_t currentControlBlock(myControlBlock.exchange(controlblock));

	if (ControlBlock()) {
		myPtr = ControlBlock()->Object();
	}

	UnregisterFrom(currentControlBlock);

	return *this;
}

template<class T>
inline void ConcurrentSharedPtr<T>::Reset()
{
	myPtr = nullptr;

	const uint64_t controlblock(myControlBlock.exchange(0));

	UnregisterFrom(controlblock);
}
template<class T>
inline ConcurrentSharedPtr<T>::operator bool() const
{
	return static_cast<bool>(ControlBlock());
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
inline void ConcurrentSharedPtr<T>::Swap(ConcurrentSharedPtr<T>& aOther)
{
	const uint64_t local(myControlBlock.load());
	myControlBlock.store(aOther.myControlBlock.load());
	aOther.myControlBlock.store(local);

	if (aOther.ControlBlock()) {
		aOther.myPtr = aOther.ControlBlock()->Object();
	}
	else {
		aOther.myPtr = nullptr;
	}
	if (ControlBlock()) {
		myPtr = ControlBlock()->Object();
	}
	else {
		myPtr = nullptr;
	}
}
template<class T>
inline void ConcurrentSharedPtr<T>::UnregisterFrom(uint64_t aControlBlock)
{
	const uint64_t pointerPortion(aControlBlock & PointerMask);

	if (pointerPortion) {
		const uint16_t expectedcount(static_cast<uint16_t>(aControlBlock & CounterMask));
		CSPControlBlock<T>* const controlblock(reinterpret_cast<CSPControlBlock<T>*>(pointerPortion >> 16));

		controlblock->TryChangeReferenceCounter(expectedcount);
		
		--(*controlblock);
	}
}
template<class T>
inline CSPControlBlock<T>* const ConcurrentSharedPtr<T>::ControlBlock() const
{
	return reinterpret_cast<CSPControlBlock<T>*>(myControlBlock._My_val >> 16);
}
template <class T>
class CSPControlBlock
{
public:
	typedef typename ConcurrentSharedPtr<T>::size_type size_type;

	template <class ...Args>
	CSPControlBlock(Args&&... aArgs);

	T* Object();
	const T* Object() const;

	void operator--();

	void TryChangeReferenceCounter(const size_type aTo);

private:
	void Destroy();

	CSP_PADDING(8);
	T myObject;
	std::atomic<size_type> myReferenceCounter;
	std::atomic<size_type> myDecrements;
	std::atomic<size_type> myIncrements;
};

template<class T>
template<class ...Args>
inline CSPControlBlock<T>::CSPControlBlock(Args && ...aArgs)
	: myObject(aArgs...)
	, myReferenceCounter(1)
	, myDecrements(0)
{

}
template<class T>
inline const T * CSPControlBlock<T>::Object() const
{
	return &myObject;
}
template<class T>
inline void CSPControlBlock<T>::operator--()
{
	const size_type decrements(++myDecrements);
	const size_type referenceCount(myReferenceCounter.load());
	const bool match(decrements == referenceCount);
	if (match) {
		Destroy();
	}
}
template<class T>
inline void CSPControlBlock<T>::TryChangeReferenceCounter(const size_type aTo)
{
	for (size_type referencecount(myReferenceCounter.load());; referencecount = myReferenceCounter.load()) {
		if (!(referencecount < aTo)) {
			break;
		}
		size_type expected(referencecount);
		const size_type desired(aTo);

		if (myReferenceCounter.compare_exchange_strong(expected, desired)) {
			break;
		}
	}
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
	returnValue.myControlBlock = toStore;
	returnValue.myControlBlock = returnValue.myControlBlock << 16;
	returnValue.myPtr = returnValue.ControlBlock()->Object();
	returnValue.myExpectedCount = 1;

	return std::move(returnValue);
}



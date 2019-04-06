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
};
template<class T>
inline ConcurrentSharedPtr<T>::ConcurrentSharedPtr()
	: myPtr(nullptr)
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


	return *this;
}

template<class T>
inline void ConcurrentSharedPtr<T>::Reset()
{
	
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
inline void ConcurrentSharedPtr<T>::Swap(ConcurrentSharedPtr<T>& aOther)
{
	aOther;
}
template<class T>
inline void ConcurrentSharedPtr<T>::UnregisterFrom(uint64_t aControlBlock)
{
	aControlBlock;
}


template<class T, class ...Args>
inline ConcurrentSharedPtr<T> MakeConcurrentSharedPtr(Args && ...aArgs)
{
	ConcurrentSharedPtr<T> returnValue;


}



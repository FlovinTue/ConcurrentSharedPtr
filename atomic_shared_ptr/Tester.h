#pragma once

#include "ThreadPool.h"
#include "atomic_shared_ptr.h"
#include <random>
#include <string>
#include "Timer.h"
#include <mutex>

using namespace gdul;

template <class T>
struct ReferenceComparison
{
	T* myPtr;
	uint8_t trash[sizeof(atomic_shared_ptr<int>) - 8];
};

template <class T>
struct MutextedWrapper
{
	MutextedWrapper() = default;
	MutextedWrapper(std::shared_ptr<T>&& aPtr)
	{
		ptr.swap(aPtr);
	}
	MutextedWrapper(MutextedWrapper& aOther)
	{
		operator=(aOther);
	}
	MutextedWrapper(MutextedWrapper&& aOther)
	{
		operator=(aOther);
	}
	static std::mutex lock;

	MutextedWrapper& operator=(MutextedWrapper& aOther)
	{
		lock.lock();
		ptr = aOther.ptr;
		lock.unlock();

		return *this;
	}
	MutextedWrapper& operator=(MutextedWrapper&& aOther)
	{
		lock.lock();
		ptr.swap(aOther);
		lock.unlock();
	}
	void Reset()
	{
		lock.lock();
		ptr.reset();
		lock.unlock();
	}
	std::shared_ptr<T> ptr;
};
template <class T>
std::mutex MutextedWrapper<T>::lock;

template <class T, uint32_t ArraySize, uint32_t NumThreads>
class Tester
{
public:
	template <class ...InitArgs>
	Tester(bool aDoInitializeArray = true, InitArgs && ...aArgs);
	~Tester();

	float Execute(uint32_t aArrayPasses, bool aDoAssign = true, bool aDoReassign = true, bool aDoCASTest = true, bool aDoReferenceTest = true);


	void WorkAssign(uint32_t aArrayPasses);
	void WorkReassign(uint32_t aArrayPasses);
	void WorkReferenceTest(uint32_t aArrayPasses);
	void WorkCAS(uint32_t aArrayPasses);

	void CheckPointers() const;

	ThreadPool myWorker;

#ifdef CSP_MUTEX_COMPARE
	MutextedWrapper<T> myTestArray[ArraySize];
#else
	atomic_shared_ptr<T> myTestArray[ArraySize];
	ReferenceComparison<T> myReferenceComparison[ArraySize];
#endif
	std::atomic_flag myWorkBlock;

	std::atomic<size_t> mySummary;

	std::random_device myRd;
	std::mt19937 myRng;
};

template<class T, uint32_t ArraySize, uint32_t NumThreads>
template<class ...InitArgs>
inline Tester<T, ArraySize, NumThreads>::Tester(bool aDoInitializeArray, InitArgs && ...aArgs)
	: myWorker(NumThreads)
	, myRng(myRd())
#ifndef CSP_MUTEX_COMPARE
	, myReferenceComparison{ nullptr }
#endif
{
	if (aDoInitializeArray) {
		for (uint32_t i = 0; i < ArraySize; ++i) {

#ifdef CSP_MUTEX_COMPARE
			MutextedWrapper<T> wrapper;
			wrapper.ptr = std::make_shared<T>(std::forward<InitArgs&&>(aArgs)...);
			myTestArray[i] = wrapper;
#else
			myTestArray[i] = make_shared<T>(std::forward<InitArgs&&>(aArgs)...);
			myReferenceComparison[i].myPtr = new T(std::forward<InitArgs&&>(aArgs)...);
#endif
		}
	}
}

template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline Tester<T, ArraySize, NumThreads>::~Tester()
{
#ifndef CSP_MUTEX_COMPARE
	for (uint32_t i = 0; i < ArraySize; ++i) {
		delete myReferenceComparison[i].myPtr;
	}
#endif
}

template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline float Tester<T, ArraySize, NumThreads>::Execute(uint32_t aArrayPasses, bool aDoAssign, bool aDoReassign, bool aDoCASTest, bool aDoReferenceTest)
{
	myWorkBlock.clear();

	for (uint32_t thread = 0; thread < NumThreads; ++thread) {
		if (aDoAssign) {
			myWorker.AddTask([this, aArrayPasses]() { WorkAssign(aArrayPasses); });
		}
		if (aDoReassign) {
			myWorker.AddTask([this, aArrayPasses]() { WorkReassign(aArrayPasses); });
		}
		if (aDoReferenceTest) {
			myWorker.AddTask([this, aArrayPasses]() { WorkReferenceTest(aArrayPasses); });
		}
		if (aDoCASTest) {
			myWorker.AddTask([this, aArrayPasses]() { WorkCAS(aArrayPasses); });
		}
	}

	Timer timer;

	myWorkBlock.test_and_set();

	while (myWorker.HasUnfinishedTasks()) {
		std::this_thread::yield();
	}

	CheckPointers();

	std::cout << "Checksum" << mySummary << std::endl;

	return timer.GetTotalTime();
}

template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline void Tester<T, ArraySize, NumThreads>::WorkAssign(uint32_t aArrayPasses)
{
	while (!myWorkBlock._My_flag) {
		std::this_thread::yield();
	}

	for (uint32_t pass = 0; pass < aArrayPasses; ++pass) {
		for (uint32_t i = 0; i < ArraySize; ++i) {
#ifdef CSP_MUTEX_COMPARE
			MutextedWrapper<T> wrapper;
			wrapper.ptr = std::make_shared<T>();
			myTestArray[i] = wrapper;
#else
			myTestArray[i] = make_shared<T>();
#endif
		}
	}
}

template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline void Tester<T, ArraySize, NumThreads>::WorkReassign(uint32_t aArrayPasses)
{
	while (!myWorkBlock._My_flag) {
		std::this_thread::yield();
	}

	for (uint32_t pass = 0; pass < aArrayPasses; ++pass) {
		for (uint32_t i = 0; i < ArraySize; ++i) {
			myTestArray[i] = myTestArray[(i + myRng()) % ArraySize].load();
		}
	}

}
template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline void Tester<T, ArraySize, NumThreads>::WorkReferenceTest(uint32_t aArrayPasses)
{
#ifndef CSP_MUTEX_COMPARE
	while (!myWorkBlock._My_flag) {
		std::this_thread::yield();
	}

	size_t localSum(0);

	for (uint32_t pass = 0; pass < aArrayPasses; ++pass) {
		for (uint32_t i = 0; i < ArraySize; ++i) {
			localSum += *myReferenceComparison[i].myPtr;
			//localSum += *myTestArray[i];
		}
	}
	mySummary += localSum;
#else
	aArrayPasses;
#endif
}

template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline void Tester<T, ArraySize, NumThreads>::WorkCAS(uint32_t aArrayPasses)
{
#ifndef CSP_MUTEX_COMPARE
	while (!myWorkBlock._My_flag) {
		std::this_thread::yield();
	}

	size_t localSum(0);

	for (uint32_t pass = 0; pass < aArrayPasses; ++pass) {
		for (uint32_t i = 0; i < ArraySize; ++i) {
			shared_ptr<T> desired(make_shared<T>());
			versioned_raw_ptr<T> expected(myTestArray[i].get_versioned_raw_ptr());
			const bool resulta = myTestArray[i].compare_exchange_strong(expected, desired);

			shared_ptr<T> desired_(make_shared<T>());
			shared_ptr<T> expected_(myTestArray[i].load());
			const bool resultb = myTestArray[i].compare_exchange_strong(expected, std::move(desired));
		}
	}
	mySummary += localSum;
#else
	aArrayPasses;
#endif
}

template<class T, uint32_t ArraySize, uint32_t NumThreads>
inline void Tester<T, ArraySize, NumThreads>::CheckPointers() const
{
#ifndef CSP_MUTEX_COMPARE
	uint32_t count(0);
	for (uint32_t i = 0; i < ArraySize; ++i) {
		const gdul::aspdetail::control_block<T, gdul::aspdetail::default_allocator>* const controlBlock(myTestArray[i].get_control_block());
		const T* const directObject(myTestArray[i].get_owned());
		const T* const sharedObject(controlBlock->get_owned());

		if (directObject != sharedObject) {
			++count;
		}
	}
	std::cout << "Mismatch shared / object count: " << count << std::endl;
#endif
}

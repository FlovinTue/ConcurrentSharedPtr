// ConcurrentSharedPtr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "ConcurrentSharedPtr.h"
#include <iostream>

//#define CSP_MUTEX_COMPARE
#include "Tester.h"

#include <intrin.h>
#include "Timer.h"
#include <functional>
#include <assert.h>
#include <vld.h>

int main()
{
	const uint32_t testArraySize(32);
	const uint32_t numThreads(8);
	Tester<uint64_t, testArraySize, numThreads> tester(true, rand());

	const bool
		doassign(true),
		doreassign(true),
		doclaim(true),
		doCAStest(true),
		doreferencetest(false);

	uint32_t arraySweeps(10000);
	uint32_t runs(32);
	float time(0.f);
	for (uint32_t i = 0; i < runs; ++i) {
		time += tester.Execute(arraySweeps, doassign, doreassign, doclaim, doCAStest, doreferencetest);
	}

#ifdef _DEBUG
	std::string config("DEBUG");
#else
	std::string config("RELEASE");
#endif
	std::string assign(doassign ? ", assign" : "");
	std::string reassign(doreassign ? ", reassign" : "");
	std::string referencetest(doreferencetest ? ", referencetest" : "");

	std::cout 
		<< "Executed " 
		<< runs 
		<< " runs with " 
		<< arraySweeps 
		<< " array sweeps over " 
		<< time 
		<< " seconds averaging " 
		<< time / runs 
		<< " seconds per run in "
		<< config
		<< " mode"
		<< " using tests "
		<< assign
		<< reassign
		<< referencetest
		<< ". The number of threads used were " 
		<< numThreads
		<< std::endl;

	ConcurrentSharedPtr<int> one = MakeConcurrentShared<int>(1);
	ConcurrentSharedPtr<int, CSMoveSafe> two = MakeConcurrentShared<int, CSMoveSafe>(2);
	ConcurrentSharedPtr<int> three = one;
	ConcurrentSharedPtr<int> four = std::move(three);
	ConcurrentSharedPtr<int, CSMoveSafe> five = std::move(two);
	ConcurrentSharedPtr<int, CSMoveSafe> six;
	six = std::move(five);
	
	ConcurrentSharedPtr<int> seven;
	seven = std::move(four);
	
	ConcurrentSharedPtr<int> eight;
	eight = seven;
	
	ConcurrentSharedPtr<int> nine(new int);
	
	int* integer = new int;
	ConcurrentSharedPtr<int> ten(integer, [](int* obj) { delete obj; });
	
	int* integer2 = new int;
	ConcurrentSharedPtr<int> eleven;
	eleven.SafeClaim(integer2);
	
	int* integer3 = new int;
	
	ConcurrentSharedPtr<int> twelve;
	twelve.UnsafeClaim(integer3);
	
	int* integer4 = new int;
	ConcurrentSharedPtr<int> thirteen;
	thirteen.SafeClaim(integer4, [](int* aInt) { delete aInt; });
	
	int* integer5 = new int;
	ConcurrentSharedPtr<int> fourteen;
	fourteen.UnsafeClaim(integer5, [](int* aInt) { delete aInt; });
	
	ConcurrentSharedPtr<int> fifteen;
	fifteen.SafeMove(std::move(fourteen));
	
	ConcurrentSharedPtr<int> sixteen;
	sixteen.UnsafeAssign(fifteen);
	sixteen.UnsafeMove(std::move(fourteen));
	sixteen.UnsafeReset();
	sixteen.UnsafeSwap(std::move(twelve));
	sixteen.SafeMove(std::move(ten));
	
	sixteen.SafeReset();
	
	ConcurrentSharedPtr<int> seventeen;
	seventeen.PrivateAssign(twelve);
	seventeen.PrivateMove(std::move(nine));
	
	ConcurrentSharedPtr<int>::size_type usecount = seventeen.UseCount();
	usecount;

	return 0;
}

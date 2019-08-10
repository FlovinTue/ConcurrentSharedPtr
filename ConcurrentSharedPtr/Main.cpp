
#include "pch.h"
#include "concurrent_shared_ptr.h"
#include <iostream>

//#define CSP_MUTEX_COMPARE
#include "Tester.h"

#include <intrin.h>
#include "Timer.h"
#include <functional>
#include <assert.h>
//#include <vld.h>

int main()
{
		const uint32_t testArraySize(32);
		const uint32_t numThreads(8);
		Tester<uint64_t, testArraySize, numThreads> tester(true, rand());
	
		const bool
			doassign(true),
			doreassign(true),
			doclaim(false),
			doCAStest(false),
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
	
		concurrent_shared_ptr<int> one = make_concurrent_shared<int>(1);
		concurrent_shared_ptr<int, csp::move_safe> two = make_concurrent_shared<int, csp::move_safe>(2);
		concurrent_shared_ptr<int> three = one;
		concurrent_shared_ptr<int> four = std::move(three);
		concurrent_shared_ptr<int, csp::move_safe> five = std::move(two);
		concurrent_shared_ptr<int, csp::move_safe> six;
		six = std::move(five);
		
		concurrent_shared_ptr<int> seven;
		seven = std::move(four);
		
		concurrent_shared_ptr<int> eight;
		eight = seven;
		
		concurrent_shared_ptr<int> nine(new int);
		
		int* integer = new int;
		concurrent_shared_ptr<int> ten(integer, [](int* obj) { delete obj; });
		
		int* integer2 = new int;
		concurrent_shared_ptr<int> eleven;
		eleven.claim(integer2);
		
		int* integer3 = new int;
		
		concurrent_shared_ptr<int> twelve;
		twelve.unsafe_claim(integer3);
		
		int* integer4 = new int;
		concurrent_shared_ptr<int> thirteen;
		thirteen.claim(integer4, [](int* aInt) { delete aInt; });
		
		int* integer5 = new int;
		concurrent_shared_ptr<int> fourteen;
		fourteen.unsafe_claim(integer5, [](int* aInt) { delete aInt; });
		
		concurrent_shared_ptr<int> fifteen;
		fifteen.move(std::move(fourteen));
		
		concurrent_shared_ptr<int> sixteen;
		sixteen.unsafe_assign(fifteen);
		sixteen.unsafe_move(std::move(fourteen));
		sixteen.unsafe_reset();
		sixteen.unsafe_swap(std::move(twelve));
		sixteen.move(std::move(ten));
		
		sixteen.reset();
		
		concurrent_shared_ptr<int> seventeen;
		seventeen.private_assign(twelve);
		seventeen.private_move(std::move(nine));
		
		concurrent_shared_ptr<int>::size_type usecount = seventeen.use_count();
		
		std::allocator<uint8_t> alloc;
		concurrent_shared_ptr<int, csp::move_default, std::allocator<uint8_t>> allocTest(make_concurrent_shared<int, csp::move_default, std::allocator<uint8_t>>(alloc, 1));
		usecount;

		return 0;
}


#include "pch.h"
#include "atomic_shared_ptr.h"
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
			doCAStest(true),
			doreferencetest(true);
	
		uint32_t arraySweeps(10000);
		uint32_t runs(32);
		float time(0.f);
		for (uint32_t i = 0; i < runs; ++i) {
			time += tester.Execute(arraySweeps, doassign, doreassign, doCAStest, doreferencetest);
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

		using namespace asp;

		std::allocator<uint8_t> alloc;

		shared_ptr<int> first;
		shared_ptr<int> second(nullptr);
		shared_ptr<int> third(make_shared<int>(3));
		shared_ptr<int> fourth(third);
		shared_ptr<int> fifth(std::move(fourth));
		shared_ptr<int> sixth(new int(6));
		shared_ptr<int> seventh(new int(7), [](int* arg) {delete arg; });	
		shared_ptr<int> eighth(new int(8), [](int* arg) {delete arg; }, alloc);
		shared_ptr<int, std::allocator<uint8_t>> ninth(make_shared<int, std::allocator<uint8_t>>(alloc, 8));
		shared_ptr<int, std::allocator<uint8_t>> tenth;
		tenth = ninth;
		shared_ptr<int, std::allocator<uint8_t>> eleventh;
		eleventh = std::move(ninth);
		
		atomic_shared_ptr<int> afirst;
		atomic_shared_ptr<int> asecond(nullptr);
		atomic_shared_ptr<int> athird(make_shared<int>(3));
		
		shared_ptr<int> afourthsrc(make_shared<int>(4));
		atomic_shared_ptr<int> afourth(afourthsrc);
		atomic_shared_ptr<int> afifth(std::move(afourthsrc));
		atomic_shared_ptr<int> asixth(afifth.load());
		atomic_shared_ptr<int> aseventh(asixth.unsafe_load());
		atomic_shared_ptr<int> aeighth(make_shared<int>(8));
		shared_ptr<int> aeightTarget(aeighth.exchange(make_shared<int>(88)));
		atomic_shared_ptr<int> aninth(make_shared<int>(9));
		aninth.exchange(aeightTarget);
		atomic_shared_ptr<int> atenth(make_shared<int>(10));
		shared_ptr<int> atenthexp(atenth.load());
		shared_ptr<int> atenthdes(make_shared<int>(1010));
		const bool tenres = atenth.compare_exchange_strong(atenthexp, atenthdes);
		
		atomic_shared_ptr<int> aeleventh(make_shared<int>(11));
		versioned_raw_ptr<int> aeleventhexp(aeleventh.get_versioned_raw_ptr());
		shared_ptr<int> aeleventhdes(make_shared<int>(1111));
		const bool eleres = aeleventh.compare_exchange_strong(aeleventhexp, aeleventhdes);
		
		atomic_shared_ptr<int> atwelfth(make_shared<int>(12));
		shared_ptr<int> atwelfthexp(make_shared<int>(121));
		shared_ptr<int> atwelfthdes(make_shared<int>(1212));
		const bool twelres = atwelfth.compare_exchange_strong(atwelfthexp, atwelfthdes);
		
		atomic_shared_ptr<int> athirteenth(make_shared<int>(13));
		versioned_raw_ptr<int> athirteenthexp(nullptr);
		shared_ptr<int> athirteenthdes(make_shared<int>(131));
		const bool thirtres = athirteenth.compare_exchange_strong(athirteenthexp, athirteenthdes);

		return 0;
}
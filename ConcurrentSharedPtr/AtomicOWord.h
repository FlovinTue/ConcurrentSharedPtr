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

#include <intrin.h>
#include <stdint.h>
#include <type_traits>
#include <assert.h>

union OWord
{
	OWord() : myQWords{ 0 } {}
	OWord(volatile int64_t* aSrc)
	{
		myQWords_s[0] = aSrc[0];
		myQWords_s[1] = aSrc[1];
	}
	uint64_t myQWords[2];
	uint32_t myDWords[4];
	uint16_t myWords[8];
	uint8_t myBytes[16];

	int64_t myQWords_s[2];
	int32_t myDWords_s[4];
	int16_t myWords_s[8];
	int8_t myBytes_s[16];
};

#pragma warning(push)
#pragma warning(disable : 4324)

class alignas(16) AtomicOWord
{
private:
	template <class T>
	struct DisableDeduction
	{
		using type = T;
	};
public:
	constexpr AtomicOWord();

	AtomicOWord(AtomicOWord& aOther);

	AtomicOWord& operator=(AtomicOWord& aOther);

	const bool CompareAndSwap(OWord& aExpected, const OWord& aDesired);

	const OWord Exchange(const OWord& aDesired);
	const OWord ExchangeQWord(const uint64_t aValue, const uint8_t aAtIndex);
	const OWord ExchangeDWord(const uint32_t aValue, const uint8_t aAtIndex);
	const OWord ExchangeWord(const uint16_t aValue, const uint8_t aAtIndex);
	const OWord ExchangeByte(const uint8_t aValue, const uint8_t aAtIndex);

	void Store(const OWord& aDesired);
	const OWord Load();

	const OWord FetchAddToQWord(const uint64_t aValue, const uint8_t aAtIndex);
	const OWord FetchAddToDWord(const uint32_t aValue, const uint8_t aAtIndex);
	const OWord FetchAddToWord(const uint16_t aValue, const uint8_t aAtIndex);
	const OWord FetchAddToByte(const uint8_t aValue, const uint8_t aAtIndex);
	const OWord FetchSubToQWord(const uint64_t aValue, const uint8_t aAtIndex);
	const OWord FetchSubToDWord(const uint32_t aValue, const uint8_t aAtIndex);
	const OWord FetchSubToWord(const uint16_t aValue, const uint8_t aAtIndex);
	const OWord FetchSubToByte(const uint8_t aValue, const uint8_t aAtIndex);

	template <class WordType>
	const OWord FetchSubToWordType(const typename DisableDeduction<WordType>::type& aValue, const uint8_t aAtIndex);
	template <class WordType>
	const OWord FetchAddToWordType(const typename DisableDeduction<WordType>::type& aValue, const uint8_t aAtIndex);
	template <class WordType>
	const OWord ExchangeWordType(const typename DisableDeduction<WordType>::type& aValue, const uint8_t aAtIndex);

	constexpr const OWord& MyVal() const;
	constexpr OWord& MyVal();

private:
	union
	{
		OWord myVal;
		volatile int64_t myStorage[2];
	};
	const bool CompareAndSwapInternal(int64_t* const aExpected, const int64_t* const aDesired);
};
template<class WordType>
inline const OWord AtomicOWord::FetchSubToWordType(const typename DisableDeduction<WordType>::type& aValue, const uint8_t aAtIndex)
{
	static_assert(std::is_integral<WordType>(), "Only integers allowed as value type");

	typedef typename std::remove_const<WordType>::type WordType_NoConst;

	const uint8_t index(aAtIndex);
	const uint8_t scaledIndex(index * sizeof(WordType));

	assert((scaledIndex < 16) && "Index out of bounds");

	OWord expected(MyVal());
	OWord desired;
	WordType_NoConst& target(*reinterpret_cast<WordType_NoConst*>(&desired.myBytes[scaledIndex]));

	do {
		desired = expected;
		target -= aValue;
	} while (!CompareAndSwapInternal(expected.myQWords_s, desired.myQWords_s));

	return expected;
}

template<class WordType>
inline const OWord AtomicOWord::FetchAddToWordType(const typename DisableDeduction<WordType>::type& aValue, const uint8_t aAtIndex)
{
	static_assert(std::is_integral<WordType>(), "Only integers allowed as value type");

	typedef typename std::remove_const<WordType>::type WordType_NoConst;

	const uint8_t index(aAtIndex);
	const uint8_t scaledIndex(index * sizeof(WordType));

	assert((scaledIndex < 16) && "Index out of bounds");

	OWord expected(MyVal());
	OWord desired;
	WordType_NoConst& target(*reinterpret_cast<WordType_NoConst*>(&desired.myBytes[scaledIndex]));

	do {
		desired = expected;
		target += aValue;
	} while (!CompareAndSwapInternal(expected.myQWords_s, desired.myQWords_s));

	return expected;
}
template<class WordType>
inline const OWord AtomicOWord::ExchangeWordType(const typename DisableDeduction<WordType>::type& aValue, const uint8_t aAtIndex)
{
	static_assert(std::is_integral<WordType>(), "Only integers allowed as value type");

	typedef typename std::remove_const<WordType>::type WordType_NoConst;

	const uint8_t index(aAtIndex);
	const uint8_t scaledIndex(index * sizeof(WordType));

	assert((scaledIndex < 16) && "Index out of bounds");

	OWord expected(MyVal());
	OWord desired;
	WordType_NoConst& target(*reinterpret_cast<WordType_NoConst*>(&desired.myBytes[scaledIndex]));

	do {
		desired = expected;
		target = aValue;
	} while (!CompareAndSwapInternal(expected.myQWords_s, desired.myQWords_s));
	
	return expected;
}
constexpr AtomicOWord::AtomicOWord()
	: myStorage{ 0 }
{
}
AtomicOWord::AtomicOWord(AtomicOWord & aOther)
	: AtomicOWord()
{
	operator=(aOther);
}
AtomicOWord& AtomicOWord::operator=(AtomicOWord & aOther)
{
	Store(aOther.Load());
	return *this;
}
const bool AtomicOWord::CompareAndSwap(OWord & aExpected, const OWord & aDesired)
{
	return CompareAndSwapInternal(aExpected.myQWords_s, aDesired.myQWords_s);
}
const OWord AtomicOWord::Exchange(const OWord& aDesired)
{
	OWord expected(MyVal());
	while (!CompareAndSwap(expected, aDesired));
	return expected;
}
const OWord AtomicOWord::ExchangeQWord(const uint64_t aValue, const uint8_t aAtIndex)
{
	return ExchangeWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::ExchangeDWord(const uint32_t aValue, const uint8_t aAtIndex)
{
	return ExchangeWordType<decltype(aValue)>(aValue, aAtIndex);
}

const OWord AtomicOWord::ExchangeWord(const uint16_t aValue, const uint8_t aAtIndex)
{
	return ExchangeWordType<decltype(aValue)>(aValue, aAtIndex);
}

const OWord AtomicOWord::ExchangeByte(const uint8_t aValue, const uint8_t aAtIndex)
{
	return ExchangeWordType<decltype(aValue)>(aValue, aAtIndex);
}

void AtomicOWord::Store(const OWord & aDesired)
{
	OWord expected(MyVal());
	while (!CompareAndSwap(expected, aDesired));
}

inline const OWord AtomicOWord::Load()
{
	OWord expectedDesired;
	CompareAndSwapInternal(expectedDesired.myQWords_s, expectedDesired.myQWords_s);
	return expectedDesired;
}
const OWord AtomicOWord::FetchAddToQWord(const uint64_t aValue, const uint8_t aAtIndex)
{
	return FetchAddToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchAddToDWord(const uint32_t aValue, const uint8_t aAtIndex)
{
	return FetchAddToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchAddToWord(const uint16_t aValue, const uint8_t aAtIndex)
{
	return FetchAddToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchAddToByte(const uint8_t aValue, const uint8_t aAtIndex)
{
	return FetchAddToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchSubToQWord(const uint64_t aValue, const uint8_t aAtIndex)
{
	return FetchSubToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchSubToDWord(const uint32_t aValue, const uint8_t aAtIndex)
{
	return FetchSubToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchSubToWord(const uint16_t aValue, const uint8_t aAtIndex)
{
	return FetchSubToWordType<decltype(aValue)>(aValue, aAtIndex);
}
const OWord AtomicOWord::FetchSubToByte(const uint8_t aValue, const uint8_t aAtIndex)
{
	return FetchSubToWordType<decltype(aValue)>(aValue, aAtIndex);
}
constexpr const OWord & AtomicOWord::MyVal() const
{
	return myVal;
}
constexpr OWord & AtomicOWord::MyVal()
{
	return myVal;
}
#ifdef _MSC_VER
const bool AtomicOWord::CompareAndSwapInternal(int64_t* const aExpected, const int64_t* const aDesired)
{
	return _InterlockedCompareExchange128(&myStorage[0], aDesired[1], aDesired[0], aExpected);
}
#elif __GNUC__
const bool AtomicOWord::CompareAndSwapInternal(int64_t* const aExpected, const int64_t* const aDesired)
{
	bool result;
	__asm__ __volatile__
	(
		"lock cmpxchg16b %1\n\t"
		"setz %0"
		: "=q" (result)
		, "+m" (myStorage[0])
		, "+d" (aExpected[1])
		, "+a" (aExpected[0])
		: "c" (aDesired[1])
		, "b" (aDesired[0])
		: "cc", "memory"
	);
	return result;
}
#endif

#pragma warning(pop)
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

#pragma warning(push)
#pragma warning(disable : 4324)

namespace gdul{

union oword
{
	oword() : myU64{ 0 } {}
	oword(volatile int64_t* from)
	{
		myS64[0] = from[0];
		myS64[1] = from[1];
	}
	inline constexpr bool operator==(const oword& other) const {
		return (myU64[0] == other.myU64[0]) & (myU64[1] == other.myU64[1]);
	}
	inline constexpr bool operator!=(const oword& other) const {
		return !operator==(other);
	}
	uint64_t myU64[2];
	uint32_t myU32[4];
	uint16_t myU16[8];
	uint8_t myU8[16];

	int64_t myS64[2];
	int32_t myS32[4];
	int16_t myS16[8];
	int8_t myS8[16];
};

class alignas(16) atomic_oword
{
private:
	template <class T>
	struct disable_deduction
	{
		using type = T;
	};
public:
	constexpr atomic_oword();

	atomic_oword(oword& value);

	bool compare_exchange_strong(oword& expected, const oword& desired);

	oword exchange(const oword& desired);
	oword exchange_qword(uint64_t value, uint8_t atIndex);
	oword exchange_dword(uint32_t value, uint8_t atIndex);
	oword exchange_word(uint16_t value, uint8_t atIndex);
	oword exchange_byte(uint8_t value, uint8_t atIndex);

	void store(const oword& desired);
	oword load();

	oword fetch_add_to_qword(uint64_t value, uint8_t atIndex);
	oword fetch_add_to_dword(uint32_t value, uint8_t atIndex);
	oword fetch_add_to_word(uint16_t value, uint8_t atIndex);
	oword fetch_add_to_byte(uint8_t value, uint8_t atIndex);
	oword fetch_sub_to_qword(uint64_t value, uint8_t atIndex);
	oword fetch_sub_to_dword(uint32_t value, uint8_t atIndex);
	oword fetch_sub_to_word(uint16_t value, uint8_t atIndex);
	oword fetch_sub_to_byte(uint8_t value, uint8_t atIndex);

	template <class word_type>
	oword fetch_add_to_word_type(const typename disable_deduction<word_type>::type& value, uint8_t atIndex);
	template <class word_type>
	oword fetch_sub_to_word_type(const typename disable_deduction<word_type>::type& value, uint8_t atIndex);
	template <class word_type>
	oword exchange_word_type(const typename disable_deduction<word_type>::type& value, uint8_t atIndex);

	constexpr const oword& my_val() const;
	constexpr oword& my_val();

private:
	union
	{
		oword myValue;
		volatile int64_t myStorage[2];
	};
	bool cas_internal(int64_t* expected, const int64_t* desired);
};

template<class word_type>
inline oword atomic_oword::fetch_add_to_word_type(const typename disable_deduction<word_type>::type& value, uint8_t atIndex)
{
	static_assert(std::is_integral<word_type>(), "Only integers allowed as value type");

	typedef typename std::remove_const<word_type>::type word_type_noconst;

	const uint8_t index(atIndex);
	const uint8_t scaledIndex(index * sizeof(word_type));

	assert((scaledIndex < 16) && "Index out of bounds");

	oword expected(my_val());
	oword desired;
	word_type_noconst& target(*reinterpret_cast<word_type_noconst*>(&desired.myU8[scaledIndex]));

	do {
		desired = expected;
		target += value;
	} while (!cas_internal(expected.myS64, desired.myS64));

	return expected;
}
template<class word_type>
inline oword atomic_oword::fetch_sub_to_word_type(const typename disable_deduction<word_type>::type& value, uint8_t atIndex)
{
	static_assert(std::is_integral<word_type>(), "Only integers allowed as value type");

	typedef typename std::remove_const<word_type>::type word_type_no_const;

	const uint8_t index(atIndex);
	const uint8_t scaledIndex(index * sizeof(word_type));

	assert((scaledIndex < 16) && "Index out of bounds");

	oword expected(my_val());
	oword desired;
	word_type_no_const& target(*reinterpret_cast<word_type_no_const*>(&desired.myU8[scaledIndex]));

	do {
		desired = expected;
		target -= value;
	} while (!cas_internal(expected.myS64, desired.myS64));

	return expected;
}
template<class word_type>
inline oword atomic_oword::exchange_word_type(const typename disable_deduction<word_type>::type& value, uint8_t atIndex)
{
	static_assert(std::is_integral<word_type>(), "Only integers allowed as value type");

	typedef typename std::remove_const<word_type>::type word_type_noconst;

	const uint8_t index(atIndex);
	const uint8_t scaledIndex(index * sizeof(word_type));

	assert((scaledIndex < 16) && "Index out of bounds");

	oword expected(my_val());
	oword desired;
	word_type_noconst& target(*reinterpret_cast<word_type_noconst*>(&desired.myU8[scaledIndex]));

	do {
		desired = expected;
		target = value;
	} while (!cas_internal(expected.myS64, desired.myS64));

	return expected;
}
constexpr atomic_oword::atomic_oword()
	: myStorage{ 0 }
{
}
inline atomic_oword::atomic_oword(oword & value)
	: myValue(value)
{
}
bool atomic_oword::compare_exchange_strong(oword & expected, const oword& desired)
{
	return cas_internal(expected.myS64, desired.myS64);
}
oword atomic_oword::exchange(const oword& desired)
{
	oword expected(my_val());
	while (!compare_exchange_strong(expected, desired));
	return expected;
}
oword atomic_oword::exchange_qword(uint64_t value, uint8_t atIndex)
{
	return exchange_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::exchange_dword(uint32_t value, uint8_t atIndex)
{
	return exchange_word_type<decltype(value)>(value, atIndex);
}

oword atomic_oword::exchange_word(uint16_t value, uint8_t atIndex)
{
	return exchange_word_type<decltype(value)>(value, atIndex);
}

oword atomic_oword::exchange_byte(const uint8_t value, uint8_t atIndex)
{
	return exchange_word_type<decltype(value)>(value, atIndex);
}

void atomic_oword::store(const oword& desired)
{
	oword expected(my_val());
	while (!compare_exchange_strong(expected, desired));
}

inline oword atomic_oword::load()
{
	oword expectedDesired;
	cas_internal(expectedDesired.myS64, expectedDesired.myS64);
	return expectedDesired;
}
oword atomic_oword::fetch_add_to_qword(uint64_t value, uint8_t atIndex)
{
	return fetch_add_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_add_to_dword(uint32_t value, uint8_t atIndex)
{
	return fetch_add_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_add_to_word(uint16_t value, uint8_t atIndex)
{
	return fetch_add_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_add_to_byte(const uint8_t value, uint8_t atIndex)
{
	return fetch_add_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_sub_to_qword(uint64_t value, uint8_t atIndex)
{
	return fetch_sub_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_sub_to_dword(uint32_t value, uint8_t atIndex)
{
	return fetch_sub_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_sub_to_word(uint16_t value, uint8_t atIndex)
{
	return fetch_sub_to_word_type<decltype(value)>(value, atIndex);
}
oword atomic_oword::fetch_sub_to_byte(const uint8_t value, uint8_t atIndex)
{
	return fetch_sub_to_word_type<decltype(value)>(value, atIndex);
}
constexpr const oword& atomic_oword::my_val() const
{
	return myValue;
}
constexpr oword & atomic_oword::my_val()
{
	return myValue;
}
#ifdef _MSC_VER
bool atomic_oword::cas_internal(int64_t* const expected, const int64_t* desired)
{
	return _InterlockedCompareExchange128(&myStorage[0], desired[1], desired[0], expected);
}
#elif __GNUC__
bool atomic_oword::cas_internal(int64_t* const expected, const int64_t* desired)
{
	bool result;
	__asm__ __volatile__
	(
		"lock cmpxchg16b %1\n\t"
		"setz %0"
		: "=q" (result)
		, "+m" (myStorage[0])
		, "+d" (expected[1])
		, "+a" (expected[0])
		: "c" (desired[1])
		, "b" (desired[0])
		: "cc", "memory"
	);
	return result;
}
#endif
}

#pragma warning(pop)
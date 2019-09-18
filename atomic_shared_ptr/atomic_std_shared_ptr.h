// Copyright(c) 2019 Flovin Michaelsen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <atomic>
#include <functional>
#include <stdint.h>
#include <atomic_oword.h>
#include <memory>

#undef max

#pragma warning(push)
#pragma warning(disable : 4201)
namespace gdul
{

namespace aspdetail
{

static const uint64_t Ptr_Mask = (std::numeric_limits<uint64_t>::max() >> 16);

template <class T>
struct disable_deduction;
}

template <class T>
class atomic_shared_ptr
{
public:
	typedef std::size_t size_type;

	inline constexpr atomic_shared_ptr();
	inline constexpr atomic_shared_ptr(std::nullptr_t);

	inline atomic_shared_ptr(const std::shared_ptr<T>& from);
	inline atomic_shared_ptr(std::shared_ptr<T>&& from);

	inline ~atomic_shared_ptr();

	inline bool compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired);
	inline bool compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired);

	inline atomic_shared_ptr<T>& operator=(const std::shared_ptr<T>& from);
	inline atomic_shared_ptr<T>& operator=(std::shared_ptr<T>&& from);

	inline std::shared_ptr<T> load();

	inline void store(const std::shared_ptr<T>& from);
	inline void store(std::shared_ptr<T>&& from);

	inline std::shared_ptr<T> exchange(const std::shared_ptr<T>& with);
	inline std::shared_ptr<T> exchange(std::shared_ptr<T>&& with);

	inline std::shared_ptr<T> unsafe_load();

	inline std::shared_ptr<T> unsafe_exchange(const std::shared_ptr<T>& with);
	inline std::shared_ptr<T> unsafe_exchange(std::shared_ptr<T>&& with);

	inline void unsafe_store(const std::shared_ptr<T>& from);
	inline void unsafe_store(std::shared_ptr<T>&& from);

private:

	inline oword copy_internal();
	inline oword unsafe_copy_internal();
	inline oword unsafe_exchange_internal(const oword& with);

	inline void unsafe_store_internal(const oword& from);
	inline void store_internal(const oword& from);

	inline oword exchange_internal(const oword& to, bool decrementPrevious);

	inline bool increment_and_try_swap(oword& expected, const oword& desired);
	inline bool cas_internal(oword& expected, const oword& desired, bool decrementPrevious, bool captureOnFailure = false);
	inline void try_increment(oword& expected);

	inline oword convert_to_oword(std::shared_ptr<T>&& shared);
	inline std::shared_ptr<T> convert_to_shared_ptr(oword&& storage);

	inline void increment(const oword& storage, size_type count);
	inline void decrement(const oword& storage, size_type count);

	enum STORAGE_QWORD : uint8_t
	{
		STORAGE_QWORD_OBJECTPTR = 0,
		STORAGE_QWORD_CONTROLBLOCKPTR = 1
	};
	enum STORAGE_WORD : uint8_t
	{
		STORAGE_WORD_COPYREQUEST = 7
	};

	atomic_oword myStorage;
};


template <class T>
inline constexpr atomic_shared_ptr<T>::atomic_shared_ptr()
{
}
template<class T>
inline constexpr atomic_shared_ptr<T>::atomic_shared_ptr(std::nullptr_t)
	: atomic_shared_ptr<T>()
{
}
template <class T>
inline atomic_shared_ptr<T>::atomic_shared_ptr(std::shared_ptr<T>&& other)
	: atomic_shared_ptr<T>()
{
	unsafe_store(std::move(other));
}
template<class T>
inline atomic_shared_ptr<T>::atomic_shared_ptr(const std::shared_ptr<T>& from)
	: atomic_shared_ptr<T>()
{
	unsafe_store(from);
}
template <class T>
inline atomic_shared_ptr<T>::~atomic_shared_ptr()
{
	unsafe_store_internal(oword());
}
template<class T>
inline bool atomic_shared_ptr<T>::compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired)
{
	return compare_exchange_strong(expected, std::shared_ptr<T>(desired));
}
template<class T>
inline bool atomic_shared_ptr<T>::compare_exchange_strong(std::shared_ptr<T>& expected, std::shared_ptr<T>&& desired)
{
	const oword desired_(*reinterpret_cast<oword*>(&desired));
	oword expected_(myStorage.my_val());
	expected_.myQWords[STORAGE_QWORD_OBJECTPTR] =(*reinterpret_cast<oword*>(&expected)).myQWords[STORAGE_QWORD_OBJECTPTR];
	const oword initial(expected_);

	do
	{
		if (cas_internal(expected_, desired_, true, true))
		{
			const oword storage(convert_to_oword(std::move(desired)));
			decrement(storage, 1);

			return true;
		}

	} while (expected_.myQWords[STORAGE_QWORD_OBJECTPTR] == initial.myQWords[STORAGE_QWORD_OBJECTPTR]);

	expected = convert_to_shared_ptr(std::move(expected_));

	return false;
}
template<class T>
inline atomic_shared_ptr<T>& atomic_shared_ptr<T>::operator=(const std::shared_ptr<T>& from)
{
	store(from);
	return *this;
}
template<class T>
inline atomic_shared_ptr<T>& atomic_shared_ptr<T>::operator=(std::shared_ptr<T>&& from)
{
	store(std::move(from));
	return *this;
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::load()
{
	oword storage(copy_internal());
	return convert_to_shared_ptr(std::move(storage));
}
template<class T>
inline void atomic_shared_ptr<T>::store(const std::shared_ptr<T>& from)
{
	store(std::shared_ptr<T>(from));
}
template<class T>
inline void atomic_shared_ptr<T>::store(std::shared_ptr<T>&& from)
{
	const oword storage(convert_to_oword(std::move(from)));
	store_internal(storage);
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::exchange(const std::shared_ptr<T>& with)
{
	return exchange(std::shared_ptr<T>(with));
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::exchange(std::shared_ptr<T>&& with)
{
	const oword storage(convert_to_oword(std::move(with)));
	oword previous(exchange_internal(storage, false));
	return convert_to_shared_ptr(std::move(previous));
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::unsafe_load()
{
	oword storage(unsafe_copy_internal());
	return convert_to_shared_ptr(std::move(storage));
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::unsafe_exchange(const std::shared_ptr<T>& with)
{
	return unsafe_exchange(std::shared_ptr<T>(with));
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::unsafe_exchange(std::shared_ptr<T>&& with)
{
	const oword storage(convert_to_oword(std::move(with)));
	oword previous(unsafe_exchange_internal(storage));
	return convert_to_shared_ptr(std::move(previous));
}
template<class T>
inline void atomic_shared_ptr<T>::unsafe_store(const std::shared_ptr<T>& from)
{
	unsafe_store(std::shared_ptr<T>(from));
}
template<class T>
inline void atomic_shared_ptr<T>::unsafe_store(std::shared_ptr<T>&& from)
{
	unsafe_store_internal(convert_to_oword(std::move(from)));
}

// ------------------------------------------------------------------------------------

// -------------------------Internals--------------------------------------------------

// ------------------------------------------------------------------------------------
template <class T>
inline void atomic_shared_ptr<T>::store_internal(const oword& from)
{
	exchange_internal(from, true);
}
template <class T>
inline bool atomic_shared_ptr<T>::increment_and_try_swap(oword & expected, const oword & desired)
{
	const uint64_t initPtr(expected.myQWords[STORAGE_QWORD_OBJECTPTR]);

	oword desired_(desired);
	desired_.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	do
	{
		const uint16_t copyRequests(expected.myWords[STORAGE_WORD_COPYREQUEST]);

		increment(expected, copyRequests);

		if (myStorage.compare_exchange_strong(expected, desired_))
		{
			return true;
		}
		else

			decrement(expected, copyRequests);

	} while (expected.myQWords[STORAGE_QWORD_OBJECTPTR] == initPtr);

	return false;
}
template<class T>
inline void atomic_shared_ptr<T>::try_increment(oword & expected)
{
	const uint64_t initPtr(expected.myQWords[STORAGE_QWORD_OBJECTPTR]);

	if (!expected.myQWords[STORAGE_QWORD_OBJECTPTR])
	{
		return;
	}

	oword desired(expected);
	desired.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	do
	{
		const uint16_t copyRequests(expected.myWords[STORAGE_WORD_COPYREQUEST]);

		increment(expected, copyRequests);

		if (myStorage.compare_exchange_strong(expected, desired))
		{
			return;
		}
		decrement(expected, copyRequests);

	} while (
		expected.myQWords[STORAGE_QWORD_OBJECTPTR] == initPtr &&
		expected.myWords[STORAGE_WORD_COPYREQUEST]);
}
template<class T>
inline oword atomic_shared_ptr<T>::convert_to_oword(std::shared_ptr<T>&& shared)
{
	oword storage;
	void* addr(reinterpret_cast<void*>(&storage));
	new (addr) std::shared_ptr<T>(std::move(shared));

	return storage;
}
template<class T>
inline std::shared_ptr<T> atomic_shared_ptr<T>::convert_to_shared_ptr(oword&& storage)
{
	storage.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	std::shared_ptr<T>* ref(reinterpret_cast<std::shared_ptr<T>*>(&storage));

	return std::shared_ptr<T>(std::move(*ref));
}
template<class T>
inline void atomic_shared_ptr<T>::increment(const oword & storage, size_type count)
{
	oword cleaned(storage);
	cleaned.myWords[STORAGE_WORD_COPYREQUEST] = 0;
	std::shared_ptr<T>* const src(reinterpret_cast<std::shared_ptr<T>*>(&cleaned));

	for (size_t i = 0; i < count; ++i){
		oword target;
		uint8_t* addr(reinterpret_cast<uint8_t*>(&target));
		new (addr) std::shared_ptr<T>(*src);
	}
}
template<class T>
inline void atomic_shared_ptr<T>::decrement(const oword & storage, size_type count)
{
	for (size_type i = 0; i < count; ++i)
	{
		oword src(storage);
		convert_to_shared_ptr(std::move(src));
	}
}
template<class T>
inline bool atomic_shared_ptr<T>::cas_internal(oword & expected, const oword & desired, bool decrementPrevious, bool captureOnFailure)
{
	bool success(false);

	oword desired_(desired);
	desired_.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	if (expected.myWords[STORAGE_WORD_COPYREQUEST])
	{
		oword expected_(myStorage.fetch_add_to_word(1, STORAGE_WORD_COPYREQUEST));
		expected_.myWords[STORAGE_WORD_COPYREQUEST] += 1;

		const uint64_t oldObjectBlock(expected.myQWords[STORAGE_QWORD_OBJECTPTR]);
		expected = expected_;

		if (expected_.myQWords[STORAGE_QWORD_OBJECTPTR] == oldObjectBlock)
		{
			success = increment_and_try_swap(expected, desired_);
		}
		else
		{
			try_increment(expected_);
		}

		decrement(expected_, static_cast<size_type>(!(captureOnFailure & !success)) + static_cast<size_type>(decrementPrevious & success));
	}
	else
	{
		success = myStorage.compare_exchange_strong(expected, desired_);

		if (decrementPrevious & success)
		{
			decrement(expected, 1);
		}
		if (!success & captureOnFailure)
		{
			expected = copy_internal();
		}
	}
	return success;
}

template <class T>
inline oword atomic_shared_ptr<T>::copy_internal()
{
	oword initial(myStorage.fetch_add_to_word(1, STORAGE_WORD_COPYREQUEST));
	initial.myWords[STORAGE_WORD_COPYREQUEST] += 1;

	if (initial.myQWords[STORAGE_QWORD_OBJECTPTR])
	{
		oword expected(initial);
		try_increment(expected);
	}

	initial.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	return initial;
}
template <class T>
inline oword atomic_shared_ptr<T>::unsafe_copy_internal()
{
	std::atomic_thread_fence(std::memory_order_acquire);

	increment(myStorage.my_val(), 1);

	return myStorage.my_val();
}
template<class T>
inline oword atomic_shared_ptr<T>::unsafe_exchange_internal(const oword & with)
{
	std::atomic_thread_fence(std::memory_order_acquire);

	const oword old(myStorage.my_val());

	myStorage.my_val() = with;

	std::atomic_thread_fence(std::memory_order_release);

	return old;
}
template <class T>
inline void atomic_shared_ptr<T>::unsafe_store_internal(const oword & from)
{
	std::atomic_thread_fence(std::memory_order_acquire);

	const oword previous(myStorage.my_val());
	myStorage.my_val() = from;

	decrement(previous, 1);

	if (!previous.myQWords[STORAGE_QWORD_OBJECTPTR])
	{
		std::atomic_thread_fence(std::memory_order_release);
	}
}
template<class T>
inline oword atomic_shared_ptr<T>::exchange_internal(const oword & to, bool decrementPrevious)
{
	oword expected(myStorage.my_val());
	while (!cas_internal(expected, to, decrementPrevious));
	return expected;
}
namespace aspdetail
{
template <class T>
struct disable_deduction
{
	using type = T;
};
}
}
#pragma warning(pop)
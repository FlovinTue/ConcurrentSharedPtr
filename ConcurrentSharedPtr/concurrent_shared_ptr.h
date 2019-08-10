#pragma once

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

#include <atomic>
#include <functional>
#include <stdint.h>
#include <atomic_oword.h>

namespace csp {

	class move_safe;
	class move_fast;

	typedef move_fast move_default;
	typedef std::allocator<uint8_t> default_allocator;

	template <class T>
	class default_deleter;
	template <class T, class Allocator>
	class control_block;

	static const uint64_t PtrMask = std::numeric_limits<uint64_t>::max() >> 16;
}

template <class T, class MoveType = csp::move_default, class Allocator = csp::default_allocator>
class concurrent_shared_ptr;

template <class T, class ...Args>
inline concurrent_shared_ptr<T, csp::move_default> make_concurrent_shared(Args&&...);
template <class T, class MoveType, class ...Args>
inline concurrent_shared_ptr<T, MoveType, csp::default_allocator> make_concurrent_shared(Args&&...);
template <class T, class MoveType, class Allocator, class ...Args>
inline concurrent_shared_ptr<T, MoveType, Allocator> make_concurrent_shared(Allocator&, Args&&...);

#pragma warning(push)
#pragma warning(disable : 4201)

template <class T, class MoveType, class Allocator>
class concurrent_shared_ptr
{
public:
	typedef std::size_t size_type;

	// The amount of memory make_concurrent_shared will request from the allocator
	static constexpr std::size_t AllocationSize = sizeof(csp::control_block<T, Allocator>) + alignof(T) + sizeof(T);

	inline constexpr concurrent_shared_ptr();
	inline constexpr concurrent_shared_ptr(const std::nullptr_t);

	inline ~concurrent_shared_ptr();

	inline explicit concurrent_shared_ptr(T* const object);
	template <class Deleter>
	inline explicit concurrent_shared_ptr(T* const object, Deleter&& deleter);
	template <class Deleter>
	inline explicit concurrent_shared_ptr(T* const object, Deleter&& deleter, Allocator& allocator);

	/*	Concurrency SAFE	*/

	// May be used from any thread at any time
	//--------------------------------------------------------------------------------------------------//
	inline concurrent_shared_ptr(concurrent_shared_ptr<T, MoveType, Allocator>& other);
	inline concurrent_shared_ptr<T, MoveType, Allocator>& operator=(concurrent_shared_ptr<T, MoveType, Allocator>& other);

	inline void claim(T* const object);
	template <class Deleter>
	inline void claim(T* const object, Deleter&& deleter);
	template <class Deleter>
	inline void claim(T* const object, Deleter&& deleter, Allocator& allocator);

	inline void reset();
	inline void move(concurrent_shared_ptr<T, MoveType, Allocator>&& from);

	inline const bool compare_exchange_strong(const T* &expected, concurrent_shared_ptr<T, MoveType, Allocator>& desired);

	//--------------------------------------------------------------------------------------------------//


	/*	Concurrency SAFE	*/

	// Safe versions of move constructor / operator. Disabled by default. May be enabled
	// by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::move_safe>::value>* = nullptr>
	inline concurrent_shared_ptr(concurrent_shared_ptr<T, MoveType, Allocator>&& other);
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::move_safe>::value>* = nullptr>
	inline concurrent_shared_ptr<T, MoveType, Allocator>& operator=(concurrent_shared_ptr<T, MoveType, Allocator>&& other);
	//--------------------------------------------------------------------------------------------------------------//


	/*	Concurrency SAFE	*/

	// Safe to use, however, may yield fleeting results if this object is reassigned 
	// during use
	//--------------------------------------------------------------------------------------//
	inline constexpr operator bool() const;

	inline constexpr const bool operator==(const concurrent_shared_ptr<T, MoveType, Allocator>& other) const;
	inline constexpr const bool operator!=(const concurrent_shared_ptr<T, MoveType, Allocator>& other) const;
	//--------------------------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// Default versions of move constructor / operator. Assumes from object is unused
	// by other threads. May be turned safe by template argument
	//--------------------------------------------------------------------------------------------------------------//
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::move_fast>::value>* = nullptr>
	inline concurrent_shared_ptr<T, MoveType, Allocator>(concurrent_shared_ptr<T, MoveType, Allocator>&& other);
	template <class U = T, class V = MoveType, std::enable_if_t<std::is_same<V, csp::move_fast>::value>* = nullptr>
	inline concurrent_shared_ptr<T, MoveType, Allocator>& operator=(concurrent_shared_ptr<T, MoveType, Allocator>&& other);
	//--------------------------------------------------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// May be used for faster performance when TO object is unused by other 
	// threads
	//----------------------------------------------------------------------//
	inline void private_assign(concurrent_shared_ptr<T, MoveType, Allocator>& from);
	inline void private_move(concurrent_shared_ptr<T, MoveType, Allocator>&& from);
	//----------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// These methods may be used for faster performance when TO and from objects are
	// not in use by other threads
	//----------------------------------------------------------------------//
	inline void unsafe_swap(concurrent_shared_ptr<T, MoveType, Allocator>&& other);
	inline void unsafe_assign(concurrent_shared_ptr<T, MoveType, Allocator>& from);
	inline void unsafe_move(concurrent_shared_ptr<T, MoveType, Allocator>&& from);
	inline void unsafe_reset();
	inline void unsafe_claim(T* const object);
	template <class Deleter>
	inline void unsafe_claim(T* const object, Deleter&& deleter);
	template <class Deleter>
	inline void unsafe_claim(T* const object, Deleter&& deleter, Allocator& allocator);
	//----------------------------------------------------------------------//


	/*	Concurrency UNSAFE	*/

	// These methods may be used safely so long as no other 
	// thread is reassigning or otherwise altering the state of 
	// this object
	//--------------------------------------------------------------//
	inline const size_type use_count() const;


	inline constexpr T* const operator->();
	inline constexpr T& operator*();

	inline constexpr const T* const operator->() const;
	inline constexpr const T& operator*() const;

	inline const T& operator[](const size_type index) const;
	inline T& operator[](const size_type index);
	//--------------------------------------------------------------//

	/*  Concurrency SAFE  */

	// These adresses are stored locally in this object and are 
	// safe to fetch. Accessing them is safe so long as no other 
	// thread is reassigning or otherwise altering the state of 
	// this object
	//--------------------------------------------------------------//
	constexpr const csp::control_block<T, Allocator>* const get_control_block() const;
	constexpr const T* const get() const;
	
	constexpr csp::control_block<T, Allocator>* const get_control_block();
	constexpr T* const get();

	inline constexpr explicit operator T*();
	inline constexpr explicit operator const T*() const;

	inline constexpr explicit operator csp::control_block<T, Allocator>*();
	inline constexpr explicit operator const csp::control_block<T, Allocator>*() const;
	//--------------------------------------------------------------//

private:

	inline const oword copy();
	inline const oword unsafe_copy();

	inline void unsafe_store(const oword& from);
	inline void store(const oword& from);

	inline const oword unsafe_steal();
	inline const oword steal();

	inline const oword exchange(const oword& to, const bool decrementPrevious);

	template<class Deleter>
	inline const oword create_control_block(T* const object, Deleter&& deleter, Allocator& allocator);
	inline const bool increment_and_try_swap(oword& expected, const oword& desired);
	inline const bool cas_internal(oword& expected, const oword& desired, const bool decrementPrevious);
	inline void try_increment(oword& expected);

	constexpr csp::control_block<T, Allocator>* const to_control_block(const oword& from);
	constexpr T* const to_object(const oword& from);

	constexpr const csp::control_block<T, Allocator>* const to_control_block(const oword& from) const;
	constexpr const T* const to_object(const oword& from) const;

	enum STORAGE_QWORD : uint8_t
	{
		STORAGE_QWORD_CONTROLBLOCKPTR = 0,
		STORAGE_QWORD_OBJECTPTR = 1
	};
	enum STORAGE_WORD : uint8_t
	{
		STORAGE_WORD_COPYREQUEST = 3,
		STORAGE_WORD_REASSIGNINDEX = 7
	};

	template <class T, class MoveType, class Allocator, class ...Args>
	friend concurrent_shared_ptr<T, MoveType, Allocator> make_concurrent_shared<T, MoveType, Allocator>(Allocator&, Args&&...);

	atomic_oword myStorage;
};
// Default constructor
template <class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr()
	: myStorage()
{
	static_assert(std::is_same<MoveType, csp::move_safe>() | std::is_same<MoveType, csp::move_fast>(), "Only csp::move_fast and csp::move_safe valid arguments for MoveType");
	static_assert(std::is_same<Allocator::value_type, uint8_t>(), "value_type for allocator must be uint8_t");
}
// Nullptr constructor
template<class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(const std::nullptr_t)
	: concurrent_shared_ptr()
{
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(concurrent_shared_ptr<T, MoveType, Allocator> & other)
	: concurrent_shared_ptr<T, MoveType, Allocator>()
{
	private_assign(other);
}
// Concurrency UNSAFE
// Default version of move constructor. Assumes from object is unused by other threads.
// May be turned safe by template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::move_fast>::value>*>
inline concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(concurrent_shared_ptr<T, MoveType, Allocator> && other)
	: concurrent_shared_ptr<U, MoveType>()
{
	unsafe_move(std::move(other));
}
// Concurrency SAFE
// Safe version of move constructor. Disabled by default. May be enabled by 
// template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::move_safe>::value>*>
inline concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(concurrent_shared_ptr<T, MoveType, Allocator> && other)
	: concurrent_shared_ptr<T, MoveType, Allocator>()
{
	private_move(std::move(other));
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(T * object)
	: concurrent_shared_ptr<T, MoveType, Allocator>()
{
	unsafe_claim(object);
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template <class T, class MoveType, class Allocator>
template<class Deleter>
inline concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(T* const object, Deleter&& deleter)
	: concurrent_shared_ptr<T, MoveType, Allocator>()
{
	unsafe_claim(object, std::forward<Deleter&&>(deleter));
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template<class T, class MoveType, class Allocator>
template<class Deleter>
inline concurrent_shared_ptr<T, MoveType, Allocator>::concurrent_shared_ptr(T * const object, Deleter && deleter, Allocator & allocator)
	: concurrent_shared_ptr<T, MoveType, Allocator>()
{
	unsafe_claim(object, std::forward<Deleter&&>(deleter), allocator);
}
// Concurrency SAFE. The Deleter callable has signature void(T* arg)
template <class T, class MoveType, class Allocator>
template<class Deleter>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::claim(T * const object, Deleter && deleter)
{
	Allocator alloc;
	claim(object, std::forward<Deleter&&>(deleter), alloc);
}
template<class T, class MoveType, class Allocator>
template<class Deleter>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::claim(T * const object, Deleter && deleter, Allocator & allocator)
{
	store(create_control_block(object, std::forward<Deleter&&>(deleter), allocator));
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::claim(T * const object)
{
	claim(object, csp::default_deleter<T>());
}
// Destructor
template <class T, class MoveType, class Allocator>
inline concurrent_shared_ptr<T, MoveType, Allocator>::~concurrent_shared_ptr()
{
	unsafe_reset();
}
// Concurrency UNSAFE
// Default version of move operator. Assumes from object is unused by other threads.
// May be turned safe by template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::move_fast>::value>*>
inline concurrent_shared_ptr<T, MoveType, Allocator> & concurrent_shared_ptr<T, MoveType, Allocator>::operator=(concurrent_shared_ptr<T, MoveType, Allocator>&& other)
{
	store(other.unsafe_steal());

	return *this;
}
// Concurrency SAFE
// Safe version of move operator. Disabled by default. May be enabled by 
// template argument
template <class T, class MoveType, class Allocator>
template <class U, class V, std::enable_if_t<std::is_same<V, csp::move_safe>::value>*>
inline concurrent_shared_ptr<T, MoveType, Allocator> & concurrent_shared_ptr<T, MoveType, Allocator>::operator=(concurrent_shared_ptr<T, MoveType, Allocator>&& other)
{
	store(other.steal());

	return *this;
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline concurrent_shared_ptr<T, MoveType, Allocator>& concurrent_shared_ptr<T, MoveType, Allocator>::operator=(concurrent_shared_ptr<T, MoveType, Allocator> & other)
{
	const bool sameobj(&other == this);
	const bool sameref(other == *this);

	if (sameobj | sameref) {
		return *this;
	}

	const oword toStore(other.copy());

	store(toStore);

	return *this;
}
// Concurrency UNSAFE
// May be used for faster performance when TO object is unused by other 
// threads
template<class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::private_assign(concurrent_shared_ptr<T, MoveType, Allocator>& other)
{
	unsafe_store(other.copy());
}
// Concurrency UNSAFE
// May be used for faster performance when TO object is unused by other 
// threads
template<class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::private_move(concurrent_shared_ptr<T, MoveType, Allocator>&& other)
{
	unsafe_store(other.steal());
}
// Concurrency UNSAFE
// May be used for faster performance when TO and from objects are unused
// by other threads
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_swap(concurrent_shared_ptr<T, MoveType, Allocator>&& other)
{
	const oword otherStorage(other.myStorage.my_val());
	other.myStorage.my_val() = myStorage.my_val();
	myStorage.my_val() = otherStorage;
}
// Concurrency UNSAFE
// May be used for faster performance when TO and from objects are unused
// by other threads
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_assign(concurrent_shared_ptr<T, MoveType, Allocator>& other)
{
	unsafe_store(other.unsafe_copy());
}
// Concurrency UNSAFE
// May be used for faster performance when TO and from objects are unused
// by other threads
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_move(concurrent_shared_ptr<T, MoveType, Allocator>&& other)
{
	unsafe_store(other.unsafe_steal());
}
// Concurrency UNSAFE
// May be used for faster performance when this object is unused by other
// threads
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_reset()
{
	unsafe_store(oword());
}
// Concurrency UNSAFE
// May be used for faster performance when this object is unused by other
// threads
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_claim(T * const object)
{
	unsafe_claim(object, csp::default_deleter<T>());
}
// Concurrency UNSAFE. The Deleter callable has signature void(T* arg)
// May be used for faster performance when this object is unused by other
// threads
template <class T, class MoveType, class Allocator>
template <class Deleter>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_claim(T * const object, Deleter && deleter)
{
	Allocator alloc;
	unsafe_claim(object, std::forward<Deleter&&>(deleter), alloc);
}
// Concurrency UNSAFE. The Deleter callable has signature void(T* arg)
// May be used for faster performance when this object is unused by other
// threads
template<class T, class MoveType, class Allocator>
template<class Deleter>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_claim(T * const object, Deleter && deleter, Allocator & allocator)
{
	unsafe_store(create_control_block(object, std::forward<Deleter&&>(deleter), allocator));
}
// Concurrency SAFE
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::reset()
{
	store(oword());
}
// Concurrency SAFE
template<class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::move(concurrent_shared_ptr<T, MoveType, Allocator>&& from)
{
	store(from.steal());
}
// Concurrency SAFE
template<class T, class MoveType, class Allocator>
inline const bool concurrent_shared_ptr<T, MoveType, Allocator>::compare_exchange_strong(const T *& expected, concurrent_shared_ptr<T, MoveType, Allocator>& desired)
{
	const oword desired_(desired.copy());
	oword expected_(myStorage.my_val());

	csp::control_block<T, Allocator>* const otherStorage(to_control_block(desired_));

	T* object(nullptr);
	do {
		object = to_object(expected);

		if (cas_internal(expected_, desired_, true)) {
			return true;
		}

	} while (object == expected);

	expected = object;

	if (otherStorage)
		--(*otherStorage);

	return false;
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline const typename concurrent_shared_ptr<T, MoveType, Allocator>::size_type concurrent_shared_ptr<T, MoveType, Allocator>::use_count() const
{
	if (!operator bool()) {
		return 0;
	}

	return get_control_block()->use_count();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::operator bool() const
{
	return static_cast<bool>(get());
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline constexpr T * const concurrent_shared_ptr<T, MoveType, Allocator>::operator->()
{
	return get();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline constexpr T & concurrent_shared_ptr<T, MoveType, Allocator>::operator*()
{
	return *(get());
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline constexpr const T * const concurrent_shared_ptr<T, MoveType, Allocator>::operator->() const
{
	return get();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline constexpr const T & concurrent_shared_ptr<T, MoveType, Allocator>::operator*() const
{
	return *(get());
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline constexpr const bool concurrent_shared_ptr<T, MoveType, Allocator>::operator==(const concurrent_shared_ptr<T, MoveType, Allocator>& other) const
{
	return operator->() == other.operator->();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline constexpr const bool concurrent_shared_ptr<T, MoveType, Allocator>::operator!=(const concurrent_shared_ptr<T, MoveType, Allocator>& other) const
{
	return !operator==(other);
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline constexpr const bool operator==(std::nullptr_t /*aNullptr*/, const concurrent_shared_ptr<T, MoveType, Allocator>& concurrent_shared_ptr)
{
	return !concurrent_shared_ptr;
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline constexpr const bool operator!=(std::nullptr_t /*aNullptr*/, const concurrent_shared_ptr<T, MoveType, Allocator>& concurrent_shared_ptr)
{
	return concurrent_shared_ptr.operator bool();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline constexpr const bool operator==(const concurrent_shared_ptr<T, MoveType, Allocator>& concurrent_shared_ptr, std::nullptr_t /*aNullptr*/)
{
	return !concurrent_shared_ptr.operator bool();
}
// Concurrency SAFE
// Safe to use, however, may yield fleeting results if this object is reassigned 
// during use
template <class T, class MoveType, class Allocator>
inline constexpr const bool operator!=(const concurrent_shared_ptr<T, MoveType, Allocator>& concurrent_shared_ptr, std::nullptr_t /*aNullptr*/)
{
	return concurrent_shared_ptr.operator bool();
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline const T & concurrent_shared_ptr<T, MoveType, Allocator>::operator[](const size_type index) const
{
	return (get())[index];
}
// Concurrency UNSAFE
// May be used safely so long as no other thread is reassigning or
// otherwise altering the state of this object
template <class T, class MoveType, class Allocator>
inline T & concurrent_shared_ptr<T, MoveType, Allocator>::operator[](const size_type index)
{
	return (get())[index];
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template <class T, class MoveType, class Allocator>
inline constexpr const csp::control_block<T, Allocator>* const concurrent_shared_ptr<T, MoveType, Allocator>::get_control_block() const
{
	return to_control_block(myStorage.my_val());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template <class T, class MoveType, class Allocator>
inline constexpr const T * const concurrent_shared_ptr<T, MoveType, Allocator>::get() const
{
	return to_object(myStorage.my_val());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline constexpr csp::control_block<T, Allocator>* const concurrent_shared_ptr<T, MoveType, Allocator>::get_control_block()
{
	return to_control_block(myStorage.my_val());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline constexpr T * const concurrent_shared_ptr<T, MoveType, Allocator>::get()
{
	return to_object(myStorage.my_val());
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::operator T*()
{
	return get();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::operator const T*() const
{
	return get();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::operator const csp::control_block<T, Allocator>*() const
{
	return get_control_block();
}
// Concurrency SAFE
// These adresses are stored locally in this object and are 
// safe to fetch. Accessing them is safe so long as no other 
// thread is reassigning or otherwise altering the state of 
// this object
template<class T, class MoveType, class Allocator>
inline constexpr concurrent_shared_ptr<T, MoveType, Allocator>::operator csp::control_block<T, Allocator>*()
{
	return get_control_block();
}
// ------------------------------------------------------------------------------------

// -------------------------Internals--------------------------------------------------

// ------------------------------------------------------------------------------------
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::store(const oword& from)
{
	exchange(from, true);
}
template<class T, class MoveType, class Allocator>
inline const oword concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_steal()
{
	const oword toSteal(myStorage.my_val());
	myStorage.my_val() = oword();

	return toSteal;
}
template<class T, class MoveType, class Allocator>
inline const oword concurrent_shared_ptr<T, MoveType, Allocator>::steal()
{
	return exchange(oword(), false);
}
template <class T, class MoveType, class Allocator>
inline const bool concurrent_shared_ptr<T, MoveType, Allocator>::increment_and_try_swap(oword & expected, const oword & desired)
{
	const uint16_t initialReassignIndex(expected.myWords[STORAGE_WORD_REASSIGNINDEX]);

	csp::control_block<T, Allocator>* const controlBlock(to_control_block(expected));

	oword desired_(desired);
	desired_.myWords[STORAGE_WORD_REASSIGNINDEX] = initialReassignIndex + 1;
	desired_.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	bool returnValue(false);
	do {
		const uint16_t copyRequests(expected.myWords[STORAGE_WORD_COPYREQUEST]);

		if (controlBlock)
			(*controlBlock) += copyRequests;

		if (!myStorage.compare_exchange_strong(expected, desired_)) {
			if (controlBlock)
				(*controlBlock) -= copyRequests;
		}
		else {
			returnValue = !static_cast<bool>(desired_.myWords[STORAGE_WORD_COPYREQUEST]);
		}

	} while (
		expected.myWords[STORAGE_WORD_COPYREQUEST] &&
		expected.myWords[STORAGE_WORD_REASSIGNINDEX] == initialReassignIndex);

	return returnValue;
}
template<class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::try_increment(oword & expected)
{
	const uint16_t initialReassignIndex(expected.myWords[STORAGE_WORD_REASSIGNINDEX]);

	csp::control_block<T, Allocator>* const controlBlock(to_control_block(expected));

	if (!controlBlock) {
		return;
	}

	oword desired(expected);
	desired.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	do {
		const uint16_t copyRequests(expected.myWords[STORAGE_WORD_COPYREQUEST]);

		(*controlBlock) += copyRequests;

		if (!myStorage.compare_exchange_strong(expected, desired)) {
			(*controlBlock) -= copyRequests;
		}

	} while (
		expected.myWords[STORAGE_WORD_COPYREQUEST] &&
		expected.myWords[STORAGE_WORD_REASSIGNINDEX] == initialReassignIndex);
}
template<class T, class MoveType, class Allocator>
inline const bool concurrent_shared_ptr<T, MoveType, Allocator>::cas_internal(oword & expected, const oword & desired, const bool decrementPrevious)
{
	bool success(false);

	csp::control_block<T, Allocator>* controlBlock(to_control_block(expected));

	oword desired_(desired);
	desired_.myWords[STORAGE_WORD_REASSIGNINDEX] = expected.myWords[STORAGE_WORD_REASSIGNINDEX] + 1;
	desired_.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	if (expected.myWords[STORAGE_WORD_COPYREQUEST]) {
		expected = myStorage.fetch_add_to_word(1, STORAGE_WORD_COPYREQUEST);
		expected.myWords[STORAGE_WORD_COPYREQUEST] += 1;

		desired_.myWords[STORAGE_WORD_REASSIGNINDEX] = expected.myWords[STORAGE_WORD_REASSIGNINDEX] + 1;

		controlBlock = to_control_block(expected);
		success = increment_and_try_swap(expected, desired_);

		if (controlBlock) {
			(*controlBlock) -= 1 + static_cast<size_type>(decrementPrevious & success);
		}
	}
	else {
		success = myStorage.compare_exchange_strong(expected, desired_);

		if (static_cast<bool>(controlBlock) & decrementPrevious & success) {
			--(*controlBlock);
		}
	}
	return success;
}
template<class T, class MoveType, class Allocator>
inline constexpr csp::control_block<T, Allocator>* const concurrent_shared_ptr<T, MoveType, Allocator>::to_control_block(const oword & from)
{
	return reinterpret_cast<csp::control_block<T, Allocator>* const>(from.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] & csp::PtrMask);
}
template<class T, class MoveType, class Allocator>
inline constexpr T* const concurrent_shared_ptr<T, MoveType, Allocator>::to_object(const oword & from)
{
	return reinterpret_cast<T* const>(from.myQWords[STORAGE_QWORD_OBJECTPTR] & csp::PtrMask);
}
template<class T, class MoveType, class Allocator>
inline constexpr const csp::control_block<T, Allocator>* const concurrent_shared_ptr<T, MoveType, Allocator>::to_control_block(const oword & from) const
{
	return reinterpret_cast<const csp::control_block<T, Allocator>* const>(from.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] & csp::PtrMask);
}
template<class T, class MoveType, class Allocator>
inline constexpr const T * const concurrent_shared_ptr<T, MoveType, Allocator>::to_object(const oword & from) const
{
	return reinterpret_cast<const T* const>(from.myQWords[STORAGE_QWORD_OBJECTPTR] & csp::PtrMask);
}
template <class T, class MoveType, class Allocator>
inline const oword concurrent_shared_ptr<T, MoveType, Allocator>::copy()
{
	oword initial(myStorage.fetch_add_to_word(1, STORAGE_WORD_COPYREQUEST));
	initial.myWords[STORAGE_WORD_COPYREQUEST] += 1;

	if (to_control_block(initial)) {
		oword expected(initial);
		try_increment(expected);
	}
	
	initial.myWords[STORAGE_WORD_COPYREQUEST] = 0;

	return initial;
}
template <class T, class MoveType, class Allocator>
inline const oword concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_copy()
{
	if (operator bool()) {
		++(*get_control_block());
	}
	return myStorage.my_val();
}
template <class T, class MoveType, class Allocator>
inline void concurrent_shared_ptr<T, MoveType, Allocator>::unsafe_store(const oword & from)
{
	if (operator bool()) {
		--(*get_control_block());
	}
	myStorage.my_val().myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] = from.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR];
	myStorage.my_val().myQWords[STORAGE_QWORD_OBJECTPTR] = from.myQWords[STORAGE_QWORD_OBJECTPTR];
}
template<class T, class MoveType, class Allocator>
inline const oword concurrent_shared_ptr<T, MoveType, Allocator>::exchange(const oword & to, const bool decrementPrevious)
{
	oword expected(myStorage.my_val());
	while (!cas_internal(expected, to, decrementPrevious));
	return expected;
}
template <class T, class MoveType, class Allocator>
template<class Deleter>
inline const oword concurrent_shared_ptr<T, MoveType, Allocator>::create_control_block(T* const object, Deleter&& deleter, Allocator& allocator)
{
	csp::control_block<T, Allocator>* controlBlock(nullptr);

	const std::size_t blockSize(sizeof(csp::control_block<T, Allocator>));

	void* block(nullptr);

	try {
		block = allocator.allocate(blockSize);
		controlBlock = new (block) csp::control_block<T, Allocator>(blockSize, object, std::forward<Deleter&&>(deleter), allocator);
	}
	catch (...) {
		allocator.deallocate(static_cast<uint8_t*>(block), blockSize);
		deleter(object);
		throw;
	}

	oword returnValue;
	returnValue.myQWords[STORAGE_QWORD_CONTROLBLOCKPTR] = reinterpret_cast<uint64_t>(controlBlock);
	returnValue.myQWords[STORAGE_QWORD_OBJECTPTR] = reinterpret_cast<uint64_t>(object);

	return returnValue;
}
namespace csp {
	template <class T, class Allocator>
	class control_block
	{
	public:
		typedef typename concurrent_shared_ptr<T>::size_type size_type;

		control_block(const std::size_t blockSize, T* const object, Allocator& allocator);
		template <class Deleter>
		control_block(const std::size_t blockSize, T* const object, Deleter&& deleter, Allocator& allocator);

		T* const get();
		const T* const get() const;

		const size_type use_count() const;

		const size_type operator--();
		const size_type operator++();
		const size_type operator-=(const size_type decrement);
		const size_type operator+=(const size_type increment);

	private:
		friend class concurrent_shared_ptr<T>;

		void destroy();

		std::atomic<size_type> myUseCount;
		std::function<void(T*)> myDeleter;
		const std::size_t myBlockSize;
		T* const myPtr;
		Allocator myAllocator;
	};
	template<class T, class Allocator>
	inline control_block<T, Allocator>::control_block(const std::size_t blockSize, T* const object, Allocator& allocator)
		: myUseCount(1)
		, myDeleter([](T* object) { object->~T(); })
		, myPtr(object)
		, myBlockSize(blockSize)
		, myAllocator(allocator)
	{
	}
	template <class T, class Allocator>
	template<class Deleter>
	inline control_block<T, Allocator>::control_block(const std::size_t blockSize, T* const object, Deleter&& deleter, Allocator& allocator)
		: myUseCount(1)
		, myDeleter(std::forward<Deleter&&>(deleter))
		, myPtr(object)
		, myBlockSize(blockSize)
		, myAllocator(allocator)
	{
	}
	template <class T, class Allocator>
	inline const typename control_block<T, Allocator>::size_type control_block<T, Allocator>::operator--()
	{
		const size_type useCount(myUseCount.fetch_sub(1, std::memory_order_acq_rel) - 1);
		if (!useCount) {
			destroy();
		}
		return useCount;
	}
	template <class T, class Allocator>
	inline const typename control_block<T, Allocator>::size_type control_block<T, Allocator>::operator++()
	{
		return myUseCount.fetch_add(1, std::memory_order_acq_rel) + 1;
	}
	template <class T, class Allocator>
	inline const typename control_block<T, Allocator>::size_type control_block<T, Allocator>::operator-=(const size_type decrement)
	{
		const size_type useCount(myUseCount.fetch_sub(decrement, std::memory_order_acq_rel) - decrement);
		if (!useCount) {
			destroy();
		}
		return useCount;
	}
	template <class T, class Allocator>
	inline const typename control_block<T, Allocator>::size_type control_block<T, Allocator>::operator+=(const size_type increment)
	{
		return myUseCount.fetch_add(increment, std::memory_order_acq_rel) + increment;
	}
	template <class T, class Allocator>
	inline T* const control_block<T, Allocator>::get()
	{
		return myPtr;
	}
	template<class T, class Allocator>
	inline const T * const control_block<T, Allocator>::get() const
	{
		return myPtr;
	}
	template <class T, class Allocator>
	inline const typename control_block<T, Allocator>::size_type control_block<T, Allocator>::use_count() const
	{
		return myUseCount.load(std::memory_order_acquire);
	}
	template <class T, class Allocator>
	inline void control_block<T, Allocator>::destroy()
	{
		myDeleter(get());
		(*this).~control_block<T, Allocator>();
		myAllocator.deallocate(reinterpret_cast<uint8_t*>(this), myBlockSize);
	}
	template <class T>
	class default_deleter
	{
	public:
		void operator()(T* const object);
	};
	template<class T>
	inline void default_deleter<T>::operator()(T * const object)
	{
		delete object;
	}
}
// Constructs a ConcurrentSharedPtr object using the default MoveType and allocator
template<class T, class ...Args>
inline concurrent_shared_ptr<T, csp::move_default> make_concurrent_shared(Args&& ...args)
{
	csp::default_allocator alloc;
	return make_concurrent_shared<T, csp::move_default>(alloc, std::forward<Args&&>(args)...);
}
// Constructs a ConcurrentSharedPtr object using explicit MoveType and default allocator
template<class T, class MoveType, class ...Args>
inline concurrent_shared_ptr<T, MoveType> make_concurrent_shared(Args&& ...args)
{
	csp::default_allocator alloc;
	return make_concurrent_shared<T, MoveType>(alloc, std::forward<Args&&>(args)...);
}
// Constructs a ConcurrentSharedPtr object using explicit MoveType and explicit allocator
template<class T, class MoveType, class Allocator, class ...Args>
inline concurrent_shared_ptr<T, MoveType, Allocator> make_concurrent_shared(Allocator& allocator, Args&& ...args)
{
	concurrent_shared_ptr<T, MoveType, Allocator> returnValue;

	const std::size_t controlBlockSize(sizeof(csp::control_block<T, Allocator>));
	const std::size_t objectSize(sizeof(T));
	const std::size_t alignment(alignof(T));
	const std::size_t blockSize(controlBlockSize + objectSize + alignment);

	uint8_t* const block(allocator.allocate(blockSize));

	const std::size_t controlBlockOffset(0);
	const std::size_t controlBlockEndAddr(reinterpret_cast<std::size_t>(block + controlBlockSize));
	const std::size_t alignmentRemainder(controlBlockEndAddr % alignment);
	const std::size_t alignmentOffset(alignment - (alignmentRemainder ? alignmentRemainder : alignment));
	const std::size_t objectOffset(controlBlockOffset + controlBlockSize + alignmentOffset);

	csp::control_block<T, Allocator> * controlBlock(nullptr);
	T* object(nullptr);
	try {
		object = new (reinterpret_cast<uint8_t*>(block) + objectOffset) T(std::forward<Args&&>(args)...);
		controlBlock = new (reinterpret_cast<uint8_t*>(block) + controlBlockOffset) csp::control_block<T, Allocator>(blockSize, object, allocator);
	}
	catch (...) {
		if (object) {
			(*object).~T();
		}
		allocator.deallocate(block, blockSize);
		throw;
	}

	returnValue.myStorage.my_val().myQWords[concurrent_shared_ptr<T, MoveType, Allocator>::STORAGE_QWORD_CONTROLBLOCKPTR] = reinterpret_cast<uint64_t>(controlBlock);
	returnValue.myStorage.my_val().myQWords[concurrent_shared_ptr<T, MoveType, Allocator>::STORAGE_QWORD_OBJECTPTR] = reinterpret_cast<uint64_t>(object);

	return returnValue;
};

#pragma warning(pop)
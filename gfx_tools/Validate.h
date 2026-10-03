#pragma once
#include <type_traits> // std::enable_if_t, std::is_integral_v, std::common_type_t...
#include <stdexcept> // std::invalid_argument, std::out_of_range
#include <utility> // std::swap
#include <stddef.h> // size_t
#include <stdint.h> // uintptr_t

class Validate {
	public:
	// just a collection of static methods, don't instantiate
	~Validate() = delete;

	/*
	IsIntegerType allows bool. For methods that use std::make_unsigned_t,
	passing a bool will result in a compile time error. This is the
	intended outcome.
	*/
	template <typename Integer>
	using IsIntegerType = std::enable_if_t<
		std::is_integral_v<Integer>, bool>;

	template <typename Signed>
	using IsSignedType = std::enable_if_t<
		std::is_signed_v<Signed>, bool>;

	template <typename Unsigned>
	using IsUnsignedType = std::enable_if_t<
		std::is_unsigned_v<Unsigned>, bool>;

	/*
	If the unsigned type would be integer promoted, then use
	unsigned int instead. This prevents an unsigned short getting
	promoted to a signed int, which could've resulted in a
	signed integer overflow even if we explicitly casted to unsigned.

	Imagine, for example, both int and short are 16-bit. A 16-bit
	short would get casted to an int of the same size, then
	potentially overflow. Comparing sizeof for both types isn't
	good enough.

	By contrast, I want large types (unsigned long long) to stay
	that size.
	*/
	template <typename Integer, IsIntegerType<Integer> = true>
	using Unsigned = std::common_type_t<
		std::make_unsigned_t<Integer>, unsigned int>;

	template <typename Integer, IsIntegerType<Integer> = true,
		IsSignedType<Integer> = true>
	static void nonnegative(Integer offset) {
		if (offset < 0) {
			throw std::invalid_argument("offset must not be negative");
		}
	}

	/*
	Overload to shut up compiler warning if bool is passed.
	*/
	template <typename Integer, IsIntegerType<Integer> = true,
		IsUnsignedType<Integer> = true>
	static void nonnegative(Integer offset) {
	}

	/*
	This returns the found offset if it's valid, for convenience's sake.
	*/
	template <typename Integer, IsIntegerType<Integer> = true>
	static Integer overflow(Integer position, Integer size) {
		/*
		Perform addition with unsigned type to prevent undefined behaviour.
		Then convert back to the original type, so that we can compare if it
		is larger or smaller without a signed/unsigned mismatch.

		The conversion back to a signed type is implementation defined
		for an out of range value. I consider this acceptable.
		*/
		Integer offset = (Integer)((Unsigned<Integer>)position
			+ (Unsigned<Integer>)size);

		/*
		If the original type was unsigned, size will always be
		greater than or equal to zero. The else block exists
		purely for the benefit of signed types, to check for
		an underflow as well.
		*/
		if (size >= 0) {
			if (offset < position) {
				throw std::invalid_argument("data must not overflow");
			}
		} else {
			if (offset > position) {
				throw std::invalid_argument("data must not underflow");
			}
		}
		return offset;
	}

	template <typename Integer, IsIntegerType<Integer> = true>
	static void overlap(Integer position, Integer size,
		Integer position2, Integer size2) {
		Integer offset = rearrange(position, size);
		Integer offset2 = rearrange(position2, size2);

		if (position < offset2 && position2 < offset) {
			throw std::invalid_argument("data must not overlap");
		}
	}

	template <typename Integer, IsIntegerType<Integer> = true>
	static void bounds(Integer innerPosition, Integer innerSize,
		Integer outerPosition, Integer outerSize) {
		Integer innerOffset = rearrange(innerPosition, innerSize);
		Integer outerOffset = rearrange(outerPosition, outerSize);

		if (innerPosition < outerPosition || innerOffset > outerOffset) {
			throw std::out_of_range("data out of bounds");
		}
	}

	static void* overflow(void* pointer, size_t size) {
		return (void*)overflow(
			(uintptr_t)pointer, (uintptr_t)size);
	}

	static const void* overflow(const void* pointer, size_t size) {
		return (const void*)overflow(
			(uintptr_t)pointer, (uintptr_t)size);
	}

	static void overlap(const void* pointer, size_t size,
		const void* pointer2, size_t size2) {
		overlap((uintptr_t)pointer, (uintptr_t)size,
			(uintptr_t)pointer2, (uintptr_t)size2);
	}

	static void bounds(const void* innerPointer, size_t innerSize,
		const void* outerPointer, size_t outerSize) {
		bounds((uintptr_t)innerPointer, (uintptr_t)innerSize,
			(uintptr_t)outerPointer, (uintptr_t)outerSize);
	}

	private:
	template <typename Integer, IsIntegerType<Integer> = true>
	static Integer rearrange(Integer &position, Integer &size) {
		/*
		A range that overflows is nonsensical. Reject it.
		*/
		Integer offset = overflow(position, size);

		/*
		A signed size may be negative. What we want is to flip it
		so the position is the beginning and the size is positive.
		
		In this case, the position passed in is considered the end
		of the half open range, so is itself excluded. For example,
		for position 500 and size -5, the numbers in the range
		are 495, 496, 497, 498, and 499, excluding 500.

		This is implemented with std::streamoff in mind, where
		negative relative offsets are valid.
		*/
		if (size < 0) {
			size = position - offset;
			std::swap(position, offset);
		}
		return offset;
	}
};
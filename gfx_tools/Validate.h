#pragma once
#include <type_traits>
#include <stdexcept>
#include <stddef.h>
#include <stdint.h>

class Validate {
	public:
	/*
	IsIntegerType allows bool, but the use of std::make_unsigned_t
	will cause it to result in a compile time error, so it'll be
	rejected as intended.
	*/
	template <typename Integer>
	using IsIntegerType = std::enable_if_t<
		std::is_integral_v<Integer>, bool>;

	/*
	If the unsigned type would be integer promoted,
	then use unsigned int instead. This prevents
	an unsigned short getting promoted to a signed int,
	which could've resulted in a signed integer overflow
	even though we explicitly casted to unsigned.
	(Imagine, for example, both int and short are 16-bit.)
	*/
	template <typename Integer, IsIntegerType<Integer> = true>
	using Unsigned = std::common_type_t<
		std::make_unsigned_t<Integer>, unsigned int>;

	template <typename Integer, IsIntegerType<Integer> = true>
	static void overflow(Integer position, Integer size) {
		/*
		Perform addition with unsigned type to prevent undefined behaviour.
		Then convert back to the original type, so that we can compare if it
		is larger or smaller without a signed/unsigned mismatch.
		*/
		Integer sum = (Integer)((Unsigned<Integer>)position
			+ (Unsigned<Integer>)size);

		/*
		If the original type was unsigned, size will always be
		greater than or equal to zero. The second condition exists
		purely for the benefit of signed types, to check for
		an underflow as well.
		*/
		if ((size >= 0 && sum < position)
			|| (size < 0 && sum > position)) {
			throw std::invalid_argument("data must not overflow");
		}
	}

	template <typename Integer, IsIntegerType<Integer> = true>
	static void overlap(Integer position, Integer size,
		Integer position2, Integer size2) {
		rearrange(position, size);
		rearrange(position2, size2);

		/*
		rearrange() verifies there is no overflow, so directly
		adding these values should now be safe.
		*/
		if (position < position2 + size2
			&& position2 < position + size) {
			throw std::invalid_argument("data must not overlap");
		}
	}

	template <typename Integer, IsIntegerType<Integer> = true>
	static void bounds(Integer innerPosition, Integer innerSize,
		Integer outerPosition, Integer outerSize) {
		rearrange(innerPosition, innerSize);
		rearrange(outerPosition, outerSize);

		/*
		rearrange() verifies there is no overflow, so directly
		adding these values should now be safe.
		*/
		if (innerPosition < outerPosition
			|| innerPosition + innerSize > outerPosition + outerSize) {
			throw std::out_of_range("data out of bounds");
		}
	}

	static void overflow(const void* pointer, size_t size) {
		overflow((uintptr_t)pointer, (uintptr_t)size);
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
	static void rearrange(Integer &position, Integer &size) {
		/*
		A signed size may be negative. What we want is to flip it
		so the position is lower and the size is positive.

		Then, because this is a half open range, we need to add one
		to the position, to make the "end" (reinterpreted as the beginning)
		fall within the range, and the "beginning" (reinterpreted as the
		end) fall outside of it.
		*/
		if (size < 0) {
			/*
			Signed overflow is undefined behaviour, so perform the addition
			using the unsigned type instead. Then convert back to the
			original type. The conversion back to a signed type is
			implementation defined - I consider this acceptable.
			*/
			position = (Integer)((Unsigned<Integer>)1
				+ (Unsigned<Integer>)position
				+ (Unsigned<Integer>)size);

			/*
			We also need to convert to unsigned and back here, because
			the distance from zero to the smallest integer is larger
			than the distance from zero to the largest integer.
			*/
			size = (Integer)((Unsigned<Integer>)0
				- (Unsigned<Integer>)size);
		}

		/*
		The resulting integers should not overflow as their original type.
		If size was negative and this throws, that indicates there was
		previously an underflow, which became an overflow upon flipping
		it around.
		*/
		overflow(position, size);
	}
};
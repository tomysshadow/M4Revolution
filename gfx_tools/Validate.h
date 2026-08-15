#pragma once
#include <type_traits>
#include <stdexcept>
#include <stdint.h>

namespace Validate {
	// this template type is just a stupidly overcomplicated way of writing
	// "I only accept integer types, not pointers"
	// I copy pasted this off of cppreference
	template <typename Integer,
		std::enable_if_t<std::is_integral_v<Integer>, bool> = true>
	inline void overflow(Integer position, Integer size) {
		using UnsignedInteger = std::make_unsigned_t<Integer>;

		UnsignedInteger _position = (UnsignedInteger)position;
		UnsignedInteger _size = (UnsignedInteger)size;

		if (_position + _size < _position) {
			throw std::invalid_argument("data must not overflow");
		}
	}

	template <typename Integer,
		std::enable_if_t<std::is_integral_v<Integer>, bool> = true>
	inline void overlap(Integer position, Integer size,
		Integer position2, Integer size2) {
		using UnsignedInteger = std::make_unsigned_t<Integer>;

		UnsignedInteger _position = (UnsignedInteger)position;
		UnsignedInteger _size = (UnsignedInteger)size;

		overflow(_position, _size);

		UnsignedInteger _position2 = (UnsignedInteger)position2;
		UnsignedInteger _size2 = (UnsignedInteger)size2;

		overflow(_position2, _size2);

		if (_position < _position2 + _size2
			&& _position2 < _position + _size) {
			throw std::invalid_argument("data must not overlap");
		}
	}

	template <typename Integer,
		std::enable_if_t<std::is_integral_v<Integer>, bool> = true>
	inline void bounds(Integer innerPosition, Integer innerSize,
		Integer outerPosition, Integer outerSize) {
		using UnsignedInteger = std::make_unsigned_t<Integer>;

		UnsignedInteger _innerPosition = (UnsignedInteger)innerPosition;
		UnsignedInteger _innerSize = (UnsignedInteger)innerSize;

		overflow(_innerPosition, _innerSize);

		UnsignedInteger _outerPosition = (UnsignedInteger)outerPosition;
		UnsignedInteger _outerSize = (UnsignedInteger)outerSize;

		overflow(_outerPosition, _outerSize);

		if (_innerPosition < _outerPosition
			|| _innerPosition + _innerSize > _outerPosition + _outerSize) {
			throw std::out_of_range("data out of bounds");
		}
	}

	inline void overflow(const void* pointer, size_t size) {
		overflow((uintptr_t)pointer, (uintptr_t)size);
	}

	inline void overlap(const void* pointer, size_t size,
		const void* pointer2, size_t size2) {
		overlap((uintptr_t)pointer, (uintptr_t)size,
			(uintptr_t)pointer2, (uintptr_t)size2);
	}

	inline void bounds(const void* innerPointer, size_t innerSize,
		const void* outerPointer, size_t outerSize) {
		bounds((uintptr_t)innerPointer, (uintptr_t)innerSize,
			(uintptr_t)outerPointer, (uintptr_t)outerSize);
	}
}
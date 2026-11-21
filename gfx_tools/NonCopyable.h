#pragma once

class NonCopyable {
	protected:
	NonCopyable() = default;

	public:
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable &operator=(const NonCopyable&) = delete;
};

class MovableNonCopyable {
	protected:
	MovableNonCopyable() = default;

	public:
	MovableNonCopyable(const MovableNonCopyable&) = delete;
	MovableNonCopyable &operator=(const MovableNonCopyable&) = delete;

	MovableNonCopyable(MovableNonCopyable&&) noexcept = default;
	MovableNonCopyable &operator=(MovableNonCopyable&&) noexcept = default;
};
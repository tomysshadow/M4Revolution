#pragma once

class NonCopyable {
    protected:
    NonCopyable() = default;

    private:
    NonCopyable(const NonCopyable &nonCopyable) = delete;
    NonCopyable &operator=(const NonCopyable &nonCopyable) = delete;
};
/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_assert.h>

#include <type_traits>

namespace DevDriver
{

// no-value state indicator
struct NullOptType
{
    struct _Tag {};
    constexpr explicit NullOptType(_Tag) {}
};
inline constexpr NullOptType NullOpt{NullOptType::_Tag{}};

// Currently only support trivial copyable types, such as int, bool, etc.
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
class Optional
{
private:
    std::remove_const_t<T> m_value;
    bool                   m_hasValue;

public:
    using ValueType = T;

    constexpr Optional() noexcept
        : m_value{},
          m_hasValue{false}
    {
    }

    constexpr Optional(NullOptType) noexcept
        : m_value{}
        , m_hasValue{false}
    {}

    constexpr Optional(const Optional<T>& other)
        : m_hasValue(other.HasValue())
    {
        if (m_hasValue)
        {
            m_value = other.Value();
        }
    }

    constexpr explicit Optional(const T& val) noexcept
        : m_value(val)
        , m_hasValue(true)
    {}

    constexpr Optional& operator=(NullOptType) noexcept
    {
        Reset();
        return *this;
    }

    constexpr Optional& operator=(const T& val) noexcept
    {
        m_value = val;
        m_hasValue = true;
        return *this;
    }

    constexpr Optional& operator=(const Optional<T>& other) noexcept
    {
        if (other.HasValue())
        {
            m_value = other.Value();
            m_hasValue = true;
        }
        else
        {
            Reset();
        }
        return *this;
    }

    constexpr bool HasValue() const noexcept
    {
        return m_hasValue;
    }

    constexpr bool Empty() const noexcept
    {
        return !m_hasValue;
    }

    constexpr const T& Value() const&
    {
        if (!m_hasValue)
        {
            DD_ASSERT(false);
        }
        return m_value;
    }

    constexpr std::remove_const_t<T> ValueOr(const T& defaultValue) const&
    {
        if (HasValue())
        {
            return m_value;
        }
        else
        {
            return defaultValue;
        }
    }

private:
    void Reset() noexcept
    {
        m_hasValue = false;
    }
};

template<typename T>
constexpr bool operator==(const Optional<T>& left, NullOptType) noexcept
{
    return !left.HasValue();
}

template<typename T>
constexpr bool operator!=(const Optional<T>& left, NullOptType) noexcept
{
    return left.HasValue();
}

template<typename T>
constexpr bool operator==(NullOptType, const Optional<T>& right) noexcept
{
    return !right.HasValue();
}

template<typename T>
constexpr bool operator!=(NullOptType, const Optional<T>& right) noexcept
{
    return right.HasValue();
}

template<typename T1, typename T2>
constexpr bool operator==(const Optional<T1>& left, const Optional<T2>& right) noexcept
{
    return (left.HasValue() && right.HasValue()) ?
        (left.Value() == right.Value()) : (left.Empty() == right.Empty());
}

template<typename T1, typename T2>
constexpr bool operator==(const Optional<T1>& left, const T2& right) noexcept
{
    return (left.HasValue() && (left.Value() == right));
}

template<typename T1, typename T2>
constexpr bool operator==(const T1& left, const Optional<T2>& right) noexcept
{
    return (right.HasValue() && (left == right.Value()));
}

template<typename T1, typename T2>
constexpr bool operator!=(const Optional<T1>& left, const Optional<T2>& right) noexcept
{
    return !(left == right);
}

template<typename T1, typename T2>
constexpr bool operator!=(const Optional<T1>& left, const T2& right) noexcept
{
    return !(left == right);
}

template<typename T1, typename T2>
constexpr bool operator!=(const T1& left, const Optional<T2>& right) noexcept
{
    return !(left == right);
}

} // namespace DevDriver

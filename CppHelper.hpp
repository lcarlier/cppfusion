#pragma once

#include <type_traits>

template<class ENUM>
constexpr std::underlying_type_t<ENUM> to_underlying(ENUM e)
{
    return static_cast<std::underlying_type_t<ENUM>>(e);
}

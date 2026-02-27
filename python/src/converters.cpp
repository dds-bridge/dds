#include "converters.hpp"

namespace dds3_python
{

auto convert_string_placeholder(const std::string& text) -> std::string
{
    return text;
}

auto convert_enum_placeholder(const int value) -> int
{
    return value;
}

auto convert_array_placeholder(
    const int value_0,
    const int value_1,
    const int value_2) -> int
{
    return value_0 + value_1 + value_2;
}

}  // namespace dds3_python

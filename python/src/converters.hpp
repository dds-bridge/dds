#pragma once

#include <string>

namespace dds3_python
{

auto convert_string_placeholder(const std::string& text) -> std::string;
auto convert_enum_placeholder(int value) -> int;
auto convert_array_placeholder(int value_0, int value_1, int value_2) -> int;

}  // namespace dds3_python

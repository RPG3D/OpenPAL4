#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace pal4 {
// Validates the original one-%s GameString contract, then uses the host CRT.
std::string FormatGameTextArgument(std::string_view format,std::string_view argument);
// Original skill acquisition hints: player name then Stunt name (%s, %s).
std::string FormatGameTextArguments(std::string_view format,std::string_view first,std::string_view second);
// Original material template: one %s name followed by one %d deficit.
std::string FormatGameTextQuantity(std::string_view format,std::string_view name,std::int32_t quantity);
// Original money hints: one signed %d amount (or the empty-string fallback).
std::string FormatGameTextInteger(std::string_view format,std::int32_t value);
}

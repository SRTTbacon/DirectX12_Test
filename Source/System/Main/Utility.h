#pragma once

#include <regex>
#include <Windows.h>

extern std::string UTF8ToShiftJIS(std::string utf8Str);

extern UINT HashString(const std::string& str);
extern UINT FnvHash(const std::vector<uint8_t>& input, bool use32bits);
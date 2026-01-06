#pragma once
#pragma comment(lib, "psapi.lib")

#include <Windows.h>
#include <string>
#include <psapi.h>

#include "SigSearch.hpp"
using namespace SigSearch::literals;

inline const auto CompressedCreateAnchorSig = "41 B8 25 00 00 00 48 8D 15 ?? ?? ?? ?? 48 8D 4D C0"_sig;

template<typename T>
uintptr_t GetAddressBySignature(const T& sig);

uintptr_t FindCompressedCreate();
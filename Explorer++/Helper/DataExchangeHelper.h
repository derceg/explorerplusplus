// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <type_traits>
#include <optional>

template <class T>
wil::unique_hglobal WriteStringToGlobal(const T &str)
{
	static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>);

	size_t stringSize = (str.size() + 1) * sizeof(T::traits_type::char_type);
	wil::unique_hglobal global(GlobalAlloc(GMEM_MOVEABLE, stringSize));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	memcpy(mem.get(), str.c_str(), stringSize);

	return global;
}

template <class T>
std::optional<T> ReadStringFromGlobal(HGLOBAL global)
{
	static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>);

	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(mem.get());

	if (size == 0)
	{
		return std::nullopt;
	}

	using charType = T::traits_type::char_type;

	return T(static_cast<const charType *>(mem.get()), size / sizeof(charType));
}
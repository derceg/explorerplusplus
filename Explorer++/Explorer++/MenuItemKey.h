// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/container_hash/hash.hpp>
#include <cstdint>
#include <type_traits>

// This can be used to identify a menu item, without having to use an integer ID. Although a menu
// class is typically self-contained (i.e. the class will add items and then handle selection
// itself), tests need to be able to invoke specific items. This class exists to be able to support
// that.
//
// The idea here is that an enum value can be used to uniquely identify an item, with both the enum
// type and value being used for equality. That then means that two enum values of different types
// will always compare unequal, even if the underlying integer values are the same.
class MenuItemKey
{
public:
	template <typename Enum>
		requires std::is_enum_v<Enum>
	MenuItemKey(Enum value) :
		m_typeToken(GetTypeToken<Enum>()),
		m_value(static_cast<std::underlying_type_t<Enum>>(value))
	{
	}

	bool operator==(const MenuItemKey &) const = default;

	friend std::size_t hash_value(const MenuItemKey &key)
	{
		std::size_t seed = 0;
		boost::hash_combine(seed, key.m_typeToken);
		boost::hash_combine(seed, key.m_value);
		return seed;
	}

private:
	template <typename T>
	static const void *GetTypeToken()
	{
		static const int token = 0;
		return &token;
	}

	const void *m_typeToken;
	std::uint64_t m_value;
};

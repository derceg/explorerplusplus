// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <ShlObj.h>
#include <string>
#include <type_traits>

// The accessor class here simply stops PidlBase from being accessed directly (since PidlBase is
// only designed for a few types of template parameters, passing an arbitrary template parameter is
// wrong).
class PidlAccessor
{
private:
	// PidlBase wraps a pidl. It's designed to be somewhat similar to how std::string wraps a
	// character array. That is, std::string makes it simple to copy and move strings from one place
	// to another, while doing that manually with char* is more cumbersome.
	// The same principle applies here, with PidlBase being designed to make it easy to copy and
	// move a pidl.
	// Note that this class always copies a raw pidl that's passed in during
	// construction/assignment. That means a call like:
	//
	// PidlAbsolute pidl(SHSimpleIDListFromPath(path));
	//
	// will leak memory, since a copy of the return value from SHSimpleIDListFromPath() will be made
	// and stored. The original return value will be leaked. That's hard to prevent, since it's
	// genuinely useful to copy a raw pidl.
	// The TakeOwnership() function can be used to assume ownership of an existing pidl.
	template <typename IDListType, auto CloneFunction>
	class PidlBase
	{
	public:
		using Pointer = IDListType *;

		PidlBase() = default;

		PidlBase(const IDListType *pidl) : m_pidl(pidl ? CloneFunction(pidl) : nullptr)
		{
		}

		PidlBase(const PidlBase &other) :
			m_pidl(other.m_pidl ? CloneFunction(other.m_pidl.get()) : nullptr)
		{
		}

		PidlBase(PidlBase &&other) : m_pidl(std::move(other.m_pidl))
		{
		}

		PidlBase &operator=(const IDListType *pidl)
		{
			m_pidl.reset(pidl ? CloneFunction(pidl) : nullptr);
			return *this;
		}

		PidlBase &operator=(PidlBase other)
		{
			std::swap(m_pidl, other.m_pidl);
			return *this;
		}

		void TakeOwnership(IDListType *pidl)
		{
			m_pidl.reset(pidl);
		}

		bool HasValue() const
		{
			return m_pidl != nullptr;
		}

		const IDListType *Raw() const
		{
			return m_pidl.get();
		}

		IDListType *Raw()
		{
			return m_pidl.get();
		}

		void Reset()
		{
			m_pidl.reset();
		}

	private:
		wil::unique_cotaskmem_ptr<IDListType> m_pidl;
	};

public:
	using PidlAbsolute = PidlBase<ITEMIDLIST_ABSOLUTE, ILCloneFull>;
	using PidlRelative = PidlBase<ITEMIDLIST_RELATIVE, ILClone>;
	using PidlChild = PidlBase<ITEMID_CHILD, ILCloneChild>;
};

using PidlAbsolute = PidlAccessor::PidlAbsolute;
using PidlRelative = PidlAccessor::PidlRelative;
using PidlChild = PidlAccessor::PidlChild;

bool operator==(const PidlAbsolute &pidl1, const PidlAbsolute &pidl2);

// This allows PidlAbsolute instances to be used with containers that require items to be hashed.
// Boost containers will use this hash by default (via boost::hash), while it can be used with
// standard containers by passing the appropriate template parameter.
std::size_t hash_value(const PidlAbsolute &pidl);

template <typename T,
	typename = std::enable_if_t<std::is_same_v<T, PidlAbsolute> || std::is_same_v<T, PidlRelative>
		|| std::is_same_v<T, PidlChild>>>
class PidlOutParamType
{
public:
	typedef typename T::Pointer Pointer;

	PidlOutParamType(T &output) : wrapper(output)
	{
	}

	operator Pointer *()
	{
		return &raw;
	}

	~PidlOutParamType()
	{
		wrapper.TakeOwnership(raw);
	}

private:
	T &wrapper;
	Pointer raw = nullptr;
};

// Functions like SHParseDisplayName() take a PIDLIST_ABSOLUTE* out parameter. Using this helper
// allows one of the types above to be passed in as that parameter, without having to use an
// intermediate type or add a operator&() method to PidlBase. For example:
//
// PidlAbsolute pidl;
// SHParseDisplayName(L"C:\\", nullptr, PidlOutParam(pidl), 0, nullptr);
//
// Designed in a similar way to wil::out_param().
template <typename T>
PidlOutParamType<T> PidlOutParam(T &p)
{
	return PidlOutParamType<T>(p);
}

std::string EncodePidlToBase64(PCIDLIST_ABSOLUTE pidl);
PidlAbsolute DecodePidlFromBase64(const std::string &encodedPidl);

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <ShlObj.h>

template <typename IDListType, auto CloneFunction>
class PidlBase
{
public:
	PidlBase() = default;

	PidlBase(const IDListType *pidl) : m_pidl(pidl ? CloneFunction(pidl) : nullptr)
	{
	}

	PidlBase(PidlBase &other) : m_pidl(other.m_pidl ? CloneFunction(other.m_pidl.get()) : nullptr)
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

	bool HasValue() const
	{
		return m_pidl != nullptr;
	}

	const IDListType *Raw() const
	{
		return m_pidl.get();
	}

private:
	wil::unique_cotaskmem_ptr<std::remove_pointer_t<IDListType>> m_pidl;
};

using PidlAbsolute = PidlBase<ITEMIDLIST_ABSOLUTE, ILCloneFull>;
using PidlRelative = PidlBase<ITEMIDLIST_RELATIVE, ILClone>;
using PidlChild = PidlBase<ITEMID_CHILD, ILCloneChild>;

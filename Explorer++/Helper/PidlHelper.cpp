// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PidlHelper.h"
#include "Base64Wrapper.h"
#include "Helper.h"
#include "ShellHelper.h"
#include <boost/container_hash/hash.hpp>

template <typename IDListType, auto CloneFunction>
PidlAccessor::PidlBase<IDListType, CloneFunction>::PidlBase(const IDListType *pidl) :
	m_pidl(pidl ? CloneFunction(pidl) : nullptr)
{
	UpdateDebugInfo();
}

template <typename IDListType, auto CloneFunction>
PidlAccessor::PidlBase<IDListType, CloneFunction>::PidlBase(IDListType *pidl, Pidl::TakeOwnership) :
	m_pidl(pidl)
{
	UpdateDebugInfo();
}

template <typename IDListType, auto CloneFunction>
PidlAccessor::PidlBase<IDListType, CloneFunction>::PidlBase(const PidlBase &other) :
	m_pidl(other.m_pidl ? CloneFunction(other.m_pidl.get()) : nullptr)
{
	UpdateDebugInfo();
}

template <typename IDListType, auto CloneFunction>
PidlAccessor::PidlBase<IDListType, CloneFunction>::PidlBase(PidlBase &&other) :
	m_pidl(std::move(other.m_pidl))
{
	UpdateDebugInfo();
	other.UpdateDebugInfo();
}

template <typename IDListType, auto CloneFunction>
PidlAccessor::PidlBase<IDListType, CloneFunction> &PidlAccessor::PidlBase<IDListType,
	CloneFunction>::operator=(const IDListType *pidl)
{
	m_pidl.reset(pidl ? CloneFunction(pidl) : nullptr);
	UpdateDebugInfo();
	return *this;
}

template <typename IDListType, auto CloneFunction>
PidlAccessor::PidlBase<IDListType, CloneFunction> &PidlAccessor::PidlBase<IDListType,
	CloneFunction>::operator=(PidlBase other)
{
	std::swap(m_pidl, other.m_pidl);
	UpdateDebugInfo();
	return *this;
}

template <typename IDListType, auto CloneFunction>
bool PidlAccessor::PidlBase<IDListType, CloneFunction>::RemoveLastItem()
	requires std::same_as<IDListType, ITEMIDLIST_ABSOLUTE>
	|| std::same_as<IDListType, ITEMIDLIST_RELATIVE>
{
	if (!m_pidl)
	{
		return false;
	}

	bool res = ILRemoveLastID(m_pidl.get());

	if (res)
	{
		UpdateDebugInfo();
	}

	return res;
}

template <typename IDListType, auto CloneFunction>
bool PidlAccessor::PidlBase<IDListType, CloneFunction>::HasValue() const
{
	return m_pidl != nullptr;
}

template <typename IDListType, auto CloneFunction>
const IDListType *PidlAccessor::PidlBase<IDListType, CloneFunction>::Raw() const
{
	return m_pidl.get();
}

template <typename IDListType, auto CloneFunction>
void PidlAccessor::PidlBase<IDListType, CloneFunction>::Reset()
{
	m_pidl.reset();
	UpdateDebugInfo();
}

#ifndef NDEBUG
template <typename IDListType, auto CloneFunction>
void PidlAccessor::PidlBase<IDListType, CloneFunction>::UpdateDebugInfo()
{
	m_isEmpty = !m_pidl;

	if constexpr (std::is_same_v<IDListType, ITEMIDLIST_ABSOLUTE>)
	{
		if (m_pidl)
		{
			m_path = GetDisplayNameWithFallback(m_pidl.get(), SHGDN_FORPARSING);
		}
		else
		{
			m_path.clear();
		}
	}
}
#else
template <typename IDListType, auto CloneFunction>
void PidlAccessor::PidlBase<IDListType, CloneFunction>::UpdateDebugInfo()
{
}
#endif // !NDEBUG

template class PidlAccessor::PidlBase<ITEMIDLIST_ABSOLUTE, ILCloneFull>;
template class PidlAccessor::PidlBase<ITEMIDLIST_RELATIVE, ILClone>;
template class PidlAccessor::PidlBase<ITEMID_CHILD, ILCloneChild>;

bool operator==(const PidlAbsolute &pidl1, const PidlAbsolute &pidl2)
{
	if (!pidl1.HasValue() && !pidl2.HasValue())
	{
		return true;
	}
	else if (!pidl1.HasValue() && pidl2.HasValue())
	{
		return false;
	}
	else if (pidl1.HasValue() && !pidl2.HasValue())
	{
		return false;
	}
	else
	{
		return ArePidlsEquivalent(pidl1.Raw(), pidl2.Raw());
	}
}

std::size_t hash_value(const PidlAbsolute &pidl)
{
	// It's important that the hash here is the same for pidls that compare equal. That should be
	// the case, since:
	//
	// - For filesystem folders, the parsing path uniquely identifies an item.
	// - For shell items, the parsing path isn't unique. However, when generating a parsing path
	// from a pidl, it's always going to give a singular answer. For shell items that can be renamed
	// (e.g. the recycle bin), the parsing path will remain the same, even if the display name is
	// changed.
	//
	// For this not to be the case, a single shell item would have to return two different parsing
	// paths, which isn't considered to be a reasonable situation.
	//
	// Note that while the parsing path should always be the same for items that compare equal,
	// having the same parsing path doesn't guarantee that items are equal. That is, items that are
	// considered to be different can have the same parsing path.
	//
	// The path here should be constructed from the data stored in the pidl, so this shouldn't
	// require disk access.
	//
	// Also note that the return value of GetDisplayName() is only DCHECK'd. The call shouldn't
	// fail, but if it somehow does, there is no reasonable alternative that can be used, given that
	// the pidl is opaque. In which case, hashing an empty string is about the most that can be
	// done.
	boost::hash<std::wstring> hasher;
	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, parsingPath);
	DCHECK(SUCCEEDED(hr));
	return hasher(parsingPath);
}

PidlAbsolute CombinePidls(PCIDLIST_ABSOLUTE parent, PCUIDLIST_RELATIVE child)
{
	// ILCombine() has the unusual behavior of returning a clone of one of the parameters, if the
	// other is null. That's unusual, since the function is documented as returning an absolute
	// pidl. So, of the parent pidl were null, the returned pidl would be a copy of the child pidl.
	// Which likely isn't going to be absolute.
	//
	// Within the application, there shouldn't be instances where ILCombine() is being called with a
	// null parameter. That is, the intent within the application is to consistently join two pidls,
	// forming an absolute pidl, never to simply receive a copy of one of the pidls.
	//
	// So, if at least one the parameters is null, this CHECK will be triggered.
	CHECK(parent && child);

	return PidlAbsolute(ILCombine(parent, child), Pidl::takeOwnership);
}

std::string EncodePidlToBase64(PCIDLIST_ABSOLUTE pidl)
{
	return cppcodec::base64_rfc4648::encode(reinterpret_cast<const char *>(pidl), ILGetSize(pidl));
}

PidlAbsolute DecodePidlFromBase64(const std::string &encodedPidl)
{
	std::vector<uint8_t> decodedContent;

	try
	{
		decodedContent = cppcodec::base64_rfc4648::decode(encodedPidl);
	}
	catch (const cppcodec::parse_error &)
	{
		return {};
	}

	auto size = decodedContent.size();

	if (size == 0)
	{
		return {};
	}

	unique_pidl_absolute pidl(static_cast<PIDLIST_ABSOLUTE>(CoTaskMemAlloc(size)));

	if (!pidl)
	{
		return {};
	}

	std::memcpy(pidl.get(), decodedContent.data(), size);

	if (!IDListContainerIsConsistent(pidl.get(), CheckedNumericCast<UINT>(size)))
	{
		return {};
	}

	return pidl.get();
}

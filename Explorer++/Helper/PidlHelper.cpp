// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PidlHelper.h"
#include "Helper.h"
#include "ShellHelper.h"
#include <boost/container_hash/hash.hpp>
#include <cppcodec/base64_rfc4648.hpp>

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

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/OleWindow.h"
#include <cereal/types/string.hpp>
#include <concurrencpp/concurrencpp.h>
#include <wil/com.h>
#include <shtypes.h>
#include <string>
#include <system_error>
#include <vector>

class ClipboardStore;
class PidlAbsolute;
class ResourceLoader;
class Runtime;
class ServiceProvider;

namespace ClipboardOperations
{

struct PastedItem
{
	std::wstring path;
	std::error_code error;

	// This is only used in tests.
	bool operator==(const PastedItem &) const = default;

	template <class Archive>
	void save(Archive &archive) const
	{
		// As noted below, it's assumed that all errors are going to be in system_category.
		DCHECK(error.category() == std::system_category());

		archive(path, error.value());
	}

	template <class Archive>
	void load(Archive &archive)
	{
		int errorCode;
		archive(path, errorCode);

		// If a paste is performed in a separate, elevated, process, the results (stored within this
		// struct) will be communicated back to the original process. Only the error code is
		// serialized, which means that there's the implicit assumption here that the error is
		// always going to be in system_category.
		// That should be the case, given the std::filesystem methods being called.
		error = std::error_code(errorCode, std::system_category());
	}
};

using PastedItems = std::vector<PastedItem>;

bool CanPasteLinkInDirectory(const ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE pidl);

void Paste(ClipboardStore *clipboardStore, const PidlAbsolute &destination,
	GetWindowCallback getWindowCallback, ServiceProvider *serviceProvider, Runtime *runtime,
	const ResourceLoader *resourceLoader);
concurrencpp::null_result PasteCustomData(ClipboardStore *clipboardStore, PidlAbsolute destination,
	GetWindowCallback getWindowCallback, wil::com_ptr_nothrow<ServiceProvider> serviceProvider,
	Runtime *runtime, const ResourceLoader *resourceLoader);

// There are two types of paste operations used within the application:
//
// 1. A paste that occurs via the shell. In that situation, the pasted items will be selected via
//    the object registered via IObjectWithSite.
// 2. A paste that is really just a file operation that's performed internally.
//
// These functions allow for the second type of paste operation to be performed.
PastedItems PasteHardLinks(ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE destination);
PastedItems PasteSymLinks(ClipboardStore *clipboardStore, PCIDLIST_ABSOLUTE destination);

// This is intended to be used by an elevated process (to attempt symlink creation based on the
// parsed command line arguments). Symlink creation will only be attempted once (rather than
// launching an elevated process when creation fails due to missing privileges).
PastedItems PasteSymLinksToPath(ClipboardStore *clipboardStore, const std::wstring &destination);

}

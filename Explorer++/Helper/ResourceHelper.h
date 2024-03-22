// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

// The definition of this struct doesn't appear in any standard header file, which is why it's
// defined here. Note that this is a variable-size struct. There are additional variable-length
// items that appear after the last member listed below.
struct DLGTEMPLATEEX
{
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
};

struct DlgTemplateExDeleter
{
	void operator()(DLGTEMPLATEEX *dlgTemplateEx) const
	{
		::operator delete(dlgTemplateEx);
	}
};

using UniqueDlgTemplateEx = std::unique_ptr<DLGTEMPLATEEX, DlgTemplateExDeleter>;

std::optional<std::vector<std::byte>> CopyResource(HINSTANCE resourceInstance, UINT resourceId,
	const WCHAR *type);

// Creates an in-memory dialog template for an extended dialog box and sets the WS_EX_LAYOUTRTL
// style on that template.
UniqueDlgTemplateEx MakeRTLDialogTemplate(HINSTANCE resourceInstance, UINT dialogId);

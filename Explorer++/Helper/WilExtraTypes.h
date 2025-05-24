// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <exdisp.h>
#include <type_traits>

void ReleaseFormatEtc(FORMATETC *formatEtc);

using unique_pidl_absolute = wil::unique_cotaskmem_ptr<std::remove_pointer_t<PIDLIST_ABSOLUTE>>;
using unique_pidl_relative = wil::unique_cotaskmem_ptr<std::remove_pointer_t<PIDLIST_RELATIVE>>;
using unique_pidl_child = wil::unique_cotaskmem_ptr<std::remove_pointer_t<PITEMID_CHILD>>;

using unique_shell_window_cookie = wil::unique_com_token<IShellWindows, long,
	decltype(&IShellWindows::Revoke), &IShellWindows::Revoke>;

using unique_formatetc =
	wil::unique_struct<FORMATETC, decltype(&::ReleaseFormatEtc), ::ReleaseFormatEtc>;

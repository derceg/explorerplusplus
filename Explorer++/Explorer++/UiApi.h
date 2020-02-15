// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class UiTheming;

namespace Plugins
{
	class UiApi
	{
	public:

		UiApi(UiTheming *uiTheming);

		bool setListViewColors(const std::wstring &backgroundColorString, const std::wstring &textColorString);
		bool setTreeViewColors(const std::wstring &backgroundColorString, const std::wstring &textColorString);

	private:

		UiTheming *m_uiTheming;
	};
}
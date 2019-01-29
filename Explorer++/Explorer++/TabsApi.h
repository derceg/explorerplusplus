// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Explorer++_internal.h"

namespace Plugins
{
	class TabsApi
	{
	public:
		TabsApi(IExplorerplusplus *pexpp);
		~TabsApi();

		void create(std::wstring path);

	private:

		IExplorerplusplus *m_pexpp;
	};
}
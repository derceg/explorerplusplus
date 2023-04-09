// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace Applications
{

class ApplicationModel;

class ApplicationModelFactory
{
public:
	static ApplicationModelFactory *GetInstance();

	ApplicationModel *GetApplicationModel();

private:
	ApplicationModelFactory() = default;
	~ApplicationModelFactory();

	static inline ApplicationModelFactory *m_staticInstance = nullptr;

	std::unique_ptr<ApplicationModel> m_applicationModel;
};

}

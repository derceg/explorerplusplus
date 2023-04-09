// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationModelFactory.h"
#include "Application.h"
#include "ApplicationModel.h"

namespace Applications
{

ApplicationModelFactory::~ApplicationModelFactory() = default;

ApplicationModelFactory *ApplicationModelFactory::GetInstance()
{
	if (!m_staticInstance)
	{
		m_staticInstance = new ApplicationModelFactory();
	}

	return m_staticInstance;
}

ApplicationModel *ApplicationModelFactory::GetApplicationModel()
{
	if (!m_applicationModel)
	{
		m_applicationModel = std::make_unique<ApplicationModel>();
	}

	return m_applicationModel.get();
}

}

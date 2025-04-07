// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationExecutor.h"
#include <gmock/gmock.h>

class ApplicationExecutorMock : public Applications::ApplicationExecutor
{
public:
	MOCK_METHOD(void, Execute,
		(const Applications::Application *application, std::wstring extraParameters), (override));
};

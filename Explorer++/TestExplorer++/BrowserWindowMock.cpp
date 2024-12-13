// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserWindowMock.h"

using namespace testing;

BrowserWindowMock::BrowserWindowMock() : m_id(idCounter++)
{
	ON_CALL(*this, GetId()).WillByDefault(Return(m_id));
}

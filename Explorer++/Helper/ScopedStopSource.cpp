// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedStopSource.h"

ScopedStopSource::~ScopedStopSource()
{
	m_stopSource.request_stop();
}

std::stop_token ScopedStopSource::GetToken() const
{
	return m_stopSource.get_token();
}

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomFont.h"
#include <algorithm>

CustomFont::CustomFont(const std::wstring &name, int size) :
	m_name(name),
	m_size(std::clamp(size, MINIMUM_SIZE, MAXIMUM_SIZE))
{
}

std::wstring CustomFont::GetName() const
{
	return m_name;
}

int CustomFont::GetSize() const
{
	return m_size;
}

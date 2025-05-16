// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboardStore.h"

bool SimulatedClipboardStore::Open()
{
	// This is a no-op, since the purpose of this class isn't really to test that clipboard
	// ownership is being assigned correctly.
	return true;
}

bool SimulatedClipboardStore::Close()
{
	return true;
}

bool SimulatedClipboardStore::IsDataAvailable(UINT format) const
{
	return m_data.contains(format);
}

HGLOBAL SimulatedClipboardStore::GetData(UINT format) const
{
	auto itr = m_data.find(format);

	if (itr == m_data.end())
	{
		return nullptr;
	}

	return itr->second.get();
}

bool SimulatedClipboardStore::SetData(UINT format, wil::unique_hglobal global)
{
	m_data[format] = std::move(global);
	return true;
}

bool SimulatedClipboardStore::Clear()
{
	m_data.clear();
	return true;
}

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Win32ResourceLoader.h"
#include "ResourceHelper.h"

Win32ResourceLoader::Win32ResourceLoader(HINSTANCE resourceInstance) :
	m_resourceInstance(resourceInstance)
{
}

std::wstring Win32ResourceLoader::LoadString(UINT stringId)
{
	// TODO: The implementation for ResourceHelper::LoadString should be moved into this method,
	// once all calls to it have been migrated to this method.
	return ResourceHelper::LoadString(m_resourceInstance, stringId);
}

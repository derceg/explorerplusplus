// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceManager.h"

void ResourceManager::Initialize(std::unique_ptr<ResourceLoader> resourceLoader)
{
	DCHECK(!m_resourceLoader);
	m_resourceLoader = std::move(resourceLoader);
}

ResourceLoader *ResourceManager::GetResourceLoader()
{
	CHECK(m_resourceLoader);
	return m_resourceLoader.get();
}

namespace Resources
{

std::wstring Resources::LoadString(UINT stringId)
{
	return ResourceManager::GetResourceLoader()->LoadString(stringId);
}

}

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ResourceLoader.h"
#include <memory>

class ResourceManager
{
public:
	static void Initialize(std::unique_ptr<ResourceLoader> resourceLoader);
	static ResourceLoader *GetResourceLoader();

private:
	static inline std::unique_ptr<ResourceLoader> m_resourceLoader;
};

// This namespace simplifies the process of calling a resource loader method.
namespace Resources
{

std::wstring LoadString(UINT stringId);

}

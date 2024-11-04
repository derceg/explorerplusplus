// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class XmlAppStorage;

class XmlAppStorageFactory
{
public:
	static std::unique_ptr<XmlAppStorage> MaybeCreate();
};

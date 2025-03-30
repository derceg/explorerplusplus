// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <filesystem>

// Creates a unique directory in the temp folder. The directory will be removed when the class
// instance is destroyed.
class ScopedTestDir
{
public:
	ScopedTestDir();
	~ScopedTestDir();

	std::filesystem::path GetPath() const;

private:
	std::filesystem::path m_path;
};

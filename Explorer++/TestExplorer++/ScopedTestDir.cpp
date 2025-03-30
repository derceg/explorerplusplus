// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ScopedTestDir.h"
#include "../Helper/Helper.h"

ScopedTestDir::ScopedTestDir()
{
	// Note that there is no attempt made to check whether the directory already exists. The
	// presence of a GUID here should be enough to ensure that the directory is unique.
	// Additionally, even if the directory was verified to be unique and newly created, that
	// wouldn't stop other processes from modifying the directory or removing it.
	//
	// In other words, even if the directory were known to be newly created here, that wouldn't
	// guarantee that it would still be empty when the tests run.
	m_path = std::filesystem::temp_directory_path() / L"explorerpp-test" / CreateGUID();
	std::filesystem::create_directories(m_path);
}

ScopedTestDir::~ScopedTestDir()
{
	std::filesystem::remove_all(m_path);
}

std::filesystem::path ScopedTestDir::GetPath() const
{
	return m_path;
}

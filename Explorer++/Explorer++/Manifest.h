// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

namespace Plugins
{
	struct Manifest
	{
		std::wstring name;
		std::wstring description;
		std::wstring file;
		std::wstring version;
		std::wstring author;
		std::wstring homepage;
	};

	void from_json(const nlohmann::json &json, Manifest &manifest);

	boost::optional<Manifest> parseManifest(const boost::filesystem::path &manifestPath);
}
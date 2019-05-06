// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Manifest.h"
#include "AcceleratorParser.h"
#include "../Helper/StringHelper.h"
#include <fstream>

void Plugins::from_json(const nlohmann::json &json, Manifest &manifest)
{
	auto name = json.at("name").get<std::string>();
	auto file = json.at("file").get<std::string>();
	auto version = json.at("version").get<std::string>();

	manifest.name = strToWstr(name);
	manifest.file = strToWstr(file);
	manifest.version = strToWstr(version);

	if (json.count("description") != 0)
	{
		auto description = json.at("description").get<std::string>();
		manifest.description = strToWstr(description);
	}

	if (json.count("author") != 0)
	{
		auto author = json.at("author").get<std::string>();
		manifest.author = strToWstr(author);
	}

	if (json.count("homepage") != 0)
	{
		auto homepage = json.at("homepage").get<std::string>();
		manifest.homepage = strToWstr(homepage);
	}

	if (json.count("std_libs_required") != 0)
	{
		json.at("std_libs_required").get_to<std::vector<sol::lib>>(manifest.libraries);
	}

	if (json.count("commands") != 0)
	{
		json.at("commands").get_to(manifest.commands);
	}
}

void Plugins::from_json(const nlohmann::json &json, Command &command)
{
	auto key = json.at("key").get<std::string>();
	command.acceleratorString = strToWstr(key);

	auto name = json.at("name").get<std::string>();
	command.name = strToWstr(name);

	command.accelerator = parseAccelerator(command.acceleratorString);

	if (json.count("description") != 0)
	{
		auto description = json.at("description").get<std::string>();
		command.description = strToWstr(description);
	}
}

boost::optional<Plugins::Manifest> Plugins::parseManifest(const boost::filesystem::path &manifestPath)
{
	std::ifstream inputStream(manifestPath.wstring());

	try
	{
		nlohmann::json json;
		inputStream >> json;

		Manifest manifest = json.get<Manifest>();
		return manifest;
	}
	catch (nlohmann::json::exception &)
	{
		/* TODO: Ideally, the error details would be returned to the
		caller. */
	}

	return boost::none;
}
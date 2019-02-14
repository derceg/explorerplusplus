// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Manifest.h"
#include "AcceleratorParser.h"
#include <codecvt>
#include <fstream>

void Plugins::from_json(const nlohmann::json &json, Manifest &manifest)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	auto name = json.at("name").get<std::string>();
	auto file = json.at("file").get<std::string>();
	auto version = json.at("version").get<std::string>();

	manifest.name = converter.from_bytes(name);
	manifest.file = converter.from_bytes(file);
	manifest.version = converter.from_bytes(version);

	if (json.count("description") != 0)
	{
		auto description = json.at("description").get<std::string>();
		manifest.description = converter.from_bytes(description);
	}

	if (json.count("author") != 0)
	{
		auto author = json.at("author").get<std::string>();
		manifest.author = converter.from_bytes(author);
	}

	if (json.count("homepage") != 0)
	{
		auto homepage = json.at("homepage").get<std::string>();
		manifest.homepage = converter.from_bytes(homepage);
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
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	auto key = json.at("key").get<std::string>();
	command.acceleratorString = converter.from_bytes(key);

	command.accelerator = parseAccelerator(command.acceleratorString);

	if (json.count("description") != 0)
	{
		auto description = json.at("description").get<std::string>();
		command.description = converter.from_bytes(description);
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
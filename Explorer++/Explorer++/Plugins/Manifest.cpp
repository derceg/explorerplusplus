// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/Manifest.h"
#include "AcceleratorMappings.h"
#include "Plugins/AcceleratorParser.h"
#include "../Helper/StringHelper.h"
#include <filesystem>
#include <fstream>

void Plugins::from_json(const nlohmann::json &json, Manifest &manifest)
{
	auto name = json.at("name").get<std::string>();
	auto file = json.at("file").get<std::string>();
	auto version = json.at("version").get<std::string>();

	manifest.name = utf8StrToWstr(name);
	manifest.file = utf8StrToWstr(file);
	manifest.version = utf8StrToWstr(version);

	if (json.count("description") != 0)
	{
		auto description = json.at("description").get<std::string>();
		manifest.description = utf8StrToWstr(description);
	}

	if (json.count("author") != 0)
	{
		auto author = json.at("author").get<std::string>();
		manifest.author = utf8StrToWstr(author);
	}

	if (json.count("homepage") != 0)
	{
		auto homepage = json.at("homepage").get<std::string>();
		manifest.homepage = utf8StrToWstr(homepage);
	}

	if (json.count("std_libs_required") != 0)
	{
		json.at("std_libs_required").get_to<std::vector<sol::lib>>(manifest.libraries);
	}

	if (json.count("commands") != 0)
	{
		json.at("commands").get_to(manifest.commands);
	}

	if (json.count("shortcut_keys") != 0)
	{
		json.at("shortcut_keys").get_to(manifest.shortcutKeys);
	}
}

void Plugins::from_json(const nlohmann::json &json, Command &command)
{
	auto key = json.at("key").get<std::string>();
	command.acceleratorString = utf8StrToWstr(key);

	auto name = json.at("name").get<std::string>();
	command.name = utf8StrToWstr(name);

	command.accelerator = parseAccelerator(command.acceleratorString);

	if (json.count("description") != 0)
	{
		auto description = json.at("description").get<std::string>();
		command.description = utf8StrToWstr(description);
	}
}

void Plugins::from_json(const nlohmann::json &json, PluginShortcutKey &shortcutKey)
{
	auto command = json.at("command").get<std::string>();

	auto itr = ACCELERATOR_MAPPINGS.find(utf8StrToWstr(command));

	if (itr == ACCELERATOR_MAPPINGS.end())
	{
		return;
	}

	shortcutKey.command = itr->second;

	json.at("keys").get_to(shortcutKey.pluginAccelerators);
}

void Plugins::from_json(const nlohmann::json &json, PluginAccelerator &pluginAccelerator)
{
	auto key = json.get<std::string>();
	pluginAccelerator.acceleratorString = utf8StrToWstr(key);

	pluginAccelerator.accelerator = parseAccelerator(pluginAccelerator.acceleratorString);
}

std::optional<Plugins::Manifest> Plugins::parseManifest(const std::filesystem::path &manifestPath)
{
	std::ifstream inputStream(manifestPath.wstring());

	try
	{
		nlohmann::json json;
		inputStream >> json;

		auto manifest = json.get<Manifest>();
		return manifest;
	}
	catch (nlohmann::json::exception &)
	{
		/* TODO: Ideally, the error details would be returned to the
		caller. */
	}

	return std::nullopt;
}
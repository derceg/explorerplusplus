// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Accelerator.h"
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <optional>

namespace std
{
namespace filesystem
{
class path;
}
}

namespace Plugins
{
struct PluginAccelerator
{
	std::wstring acceleratorString;
	std::optional<Accelerator> accelerator;
};

struct PluginShortcutKey
{
	std::optional<int> command;
	std::vector<PluginAccelerator> pluginAccelerators;
};

struct Command
{
	std::wstring name;
	std::wstring acceleratorString;
	std::optional<Accelerator> accelerator;
	std::wstring description;
};

struct Manifest
{
	std::wstring name;
	std::wstring description;
	std::wstring file;
	std::wstring version;
	std::wstring author;
	std::wstring homepage;

	std::vector<sol::lib> libraries;
	std::vector<Command> commands;
	std::vector<PluginShortcutKey> shortcutKeys;
};

NLOHMANN_JSON_SERIALIZE_ENUM(sol::lib,
	{
		{ sol::lib::base, "base" },
		{ sol::lib::package, "package" },
		{ sol::lib::coroutine, "coroutine" },
		{ sol::lib::string, "string" },
		{ sol::lib::os, "os" },
		{ sol::lib::math, "math" },
		{ sol::lib::table, "table" },
		{ sol::lib::debug, "debug" },
		{ sol::lib::io, "io" },
		{ sol::lib::utf8, "utf8" },
	});

void from_json(const nlohmann::json &json, Manifest &manifest);
void from_json(const nlohmann::json &json, Command &command);
void from_json(const nlohmann::json &json, PluginShortcutKey &shortcutKey);
void from_json(const nlohmann::json &json, PluginAccelerator &pluginAccelerator);

std::optional<Manifest> parseManifest(const std::filesystem::path &manifestPath);
}

namespace nlohmann
{
// This allows sol::lib to be unserialized, even though the
// NLOHMANN_JSON_SERIALIZE_ENUM macro declared above isn't in the
// sol namespace (which it usually needs to be).
template <>
struct adl_serializer<sol::lib>
{
	static void from_json(const json &j, sol::lib &lib)
	{
		Plugins::from_json(j, lib);
	}
};
}

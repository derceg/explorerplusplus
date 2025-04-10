// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/PluginManager.h"
#include "AcceleratorUpdater.h"
#include "Plugins/Manifest.h"
#include "Plugins/PluginCommandManager.h"
#include <sol/forward.hpp>
#include <filesystem>

const std::wstring Plugins::PluginManager::MANIFEST_NAME = L"plugin.json";

std::vector<ShortcutKey> convertPluginShortcutKeys(
	const std::vector<Plugins::PluginShortcutKey> &pluginShortcutKeys);

Plugins::PluginManager::PluginManager(PluginInterface *pluginInterface, const Config *config) :
	m_pluginInterface(pluginInterface),
	m_config(config)
{
}

void Plugins::PluginManager::loadAllPlugins(const std::filesystem::path &pluginDirectory)
{
	std::error_code error;

	/* TODO: Ideally, any error would be logged somewhere. For now, it's
	ignored. */
	for (const auto &entry : std::filesystem::directory_iterator(pluginDirectory, error))
	{
		std::error_code statusError;
		auto status = entry.status(statusError);

		if (!statusError && std::filesystem::is_directory(status))
		{
			/* TODO: This should return an error code, perhaps using
			something like std::expected or boost::outcome, once either
			is available. */
			attemptToLoadPlugin(entry);
		}
	}
}

bool Plugins::PluginManager::attemptToLoadPlugin(const std::filesystem::path &directory)
{
	auto manifestPath = directory / MANIFEST_NAME;
	auto manifest = parseManifest(manifestPath);

	if (!manifest)
	{
		return false;
	}

	return registerPlugin(directory, *manifest);
}

bool Plugins::PluginManager::registerPlugin(const std::filesystem::path &directory,
	const Manifest &manifest)
{
	auto plugin =
		std::make_unique<LuaPlugin>(directory.wstring(), manifest, m_pluginInterface, m_config);

	for (auto library : manifest.libraries)
	{
		// open_libraries takes an rvalue reference. It also checks that
		// the arguments passed in are all of type sol::lib. If library
		// is passed in directly, the type T will be "sol::lib &", which
		// won't match the type sol::lib. To ensure that they do match,
		// an rvalue reference needs to be passed in (which makes sense,
		// as this function is designed to be called with enum values
		// directly (e.g. sol::lib::base), which are rvalues). This is
		// why std::move is being used here.
		plugin->GetLuaState().open_libraries(std::move(library));
	}

	auto pluginFile = directory / manifest.file;

	// There's a potential race issue here. The file could exist at this
	// point, but not when used by the safe_script_file call below. That
	// doesn't really matter though.
	if (!std::filesystem::exists(pluginFile))
	{
		return false;
	}

	try
	{
		plugin->GetLuaState().safe_script_file(pluginFile.string());
	}
	catch (const sol::error &)
	{
		// Ignore the error. An exception can be thrown for something
		// simple like a Lua script trying to use a variable that
		// doesn't exist. That definitely shouldn't result in the
		// application being terminated because of an uncaught
		// exception.
		// The assumption here is that since the panic handler wasn't
		// called, the Lua state is still usable. Loading the plugin
		// even if there's an error can be potentially useful for users,
		// as it means that the plugin might still offer some of its
		// functionality (if that functionality was set up before the
		// error occurred).
	}
	catch (const LuaPanicException &)
	{
		// If a panic has occurred, the Lua state is irretrievably
		// broken. It's not safe to attempt to continue to use it.
		// Returning here will ensure that the state is simply
		// destroyed.
		return false;
	}

	m_pluginInterface->GetAccleratorUpdater()->update(
		convertPluginShortcutKeys(manifest.shortcutKeys));
	m_pluginInterface->GetPluginCommandManager()->addCommands(plugin->GetId(), manifest.commands);

	m_plugins.push_back(std::move(plugin));

	return true;
}

std::vector<ShortcutKey> convertPluginShortcutKeys(
	const std::vector<Plugins::PluginShortcutKey> &pluginShortcutKeys)
{
	std::vector<ShortcutKey> shortcutKeys;

	for (auto &pluginShortcutKey : pluginShortcutKeys)
	{
		if (!pluginShortcutKey.command)
		{
			continue;
		}

		ShortcutKey shortcutKey;
		shortcutKey.command = *pluginShortcutKey.command;

		for (auto &pluginAccelerator : pluginShortcutKey.pluginAccelerators)
		{
			if (!pluginAccelerator.accelerator)
			{
				continue;
			}

			shortcutKey.accelerators.push_back(*pluginAccelerator.accelerator);
		}

		shortcutKeys.push_back(shortcutKey);
	}

	return shortcutKeys;
}

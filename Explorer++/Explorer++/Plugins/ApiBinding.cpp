// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/ApiBinding.h"
#include "Plugins/CommandApi/Events/CommandInvoked.h"
#include "Plugins/MenuApi.h"
#include "Plugins/PluginMenuManager.h"
#include "Plugins/TabsApi/Events/TabCreated.h"
#include "Plugins/TabsApi/Events/TabMoved.h"
#include "Plugins/TabsApi/Events/TabRemoved.h"
#include "Plugins/TabsApi/Events/TabUpdated.h"
#include "Plugins/TabsApi/TabsApi.h"
#include "Plugins/UiApi.h"
#include "ShellBrowser/SortModes.h"
#include "ShellBrowser/ViewModes.h"
#include "TabContainerImpl.h"
#include "UiTheming.h"
#include <sol/sol.hpp>

void BindTabsAPI(sol::state &state, TabEvents *tabEvents, TabContainerImpl *tabContainerImpl,
	const Config *config);
void BindMenuApi(sol::state &state, Plugins::PluginMenuManager *pluginMenuManager);
void BindUiApi(sol::state &state, UiTheming *uiTheming);
void BindCommandApi(int pluginId, sol::state &state,
	Plugins::PluginCommandManager *pluginCommandManager);
template <typename T>
void BindObserverMethods(sol::state &state, sol::table &parentTable,
	const std::string &observerTableName, const std::shared_ptr<T> &object);
template <typename T>
void AddEnum(sol::state &state, sol::table &parentTable, const std::string &name);
sol::table MarkTableReadOnly(sol::state &state, sol::table &table);
int deny(lua_State *state);

void Plugins::BindAllApiMethods(int pluginId, sol::state &state, PluginInterface *pluginInterface,
	const Config *config)
{
	BindTabsAPI(state, pluginInterface->GetTabEvents(), pluginInterface->GetTabContainerImpl(),
		config);
	BindMenuApi(state, pluginInterface->GetPluginMenuManager());
	BindUiApi(state, pluginInterface->GetUiTheming());
	BindCommandApi(pluginId, state, pluginInterface->GetPluginCommandManager());
}

void BindTabsAPI(sol::state &state, TabEvents *tabEvents, TabContainerImpl *tabContainerImpl,
	const Config *config)
{
	std::shared_ptr<Plugins::TabsApi> tabsApi =
		std::make_shared<Plugins::TabsApi>(tabContainerImpl, config);

	sol::table tabsTable = state.create_named_table("tabs");
	sol::table tabsMetaTable = MarkTableReadOnly(state, tabsTable);

	tabsMetaTable.set_function("getAll", &Plugins::TabsApi::getAll, tabsApi);
	tabsMetaTable.set_function("get", &Plugins::TabsApi::get, tabsApi);
	tabsMetaTable.set_function("create", &Plugins::TabsApi::create, tabsApi);
	tabsMetaTable.set_function("update", &Plugins::TabsApi::update, tabsApi);
	tabsMetaTable.set_function("refresh", &Plugins::TabsApi::refresh, tabsApi);
	tabsMetaTable.set_function("move", &Plugins::TabsApi::move, tabsApi);
	tabsMetaTable.set_function("close", &Plugins::TabsApi::close, tabsApi);

	std::shared_ptr<Plugins::TabCreated> tabCreated =
		std::make_shared<Plugins::TabCreated>(tabEvents);
	BindObserverMethods(state, tabsMetaTable, "onCreated", tabCreated);

	std::shared_ptr<Plugins::TabMoved> tabMoved = std::make_shared<Plugins::TabMoved>(tabEvents);
	BindObserverMethods(state, tabsMetaTable, "onMoved", tabMoved);

	std::shared_ptr<Plugins::TabUpdated> tabUpdated =
		std::make_shared<Plugins::TabUpdated>(tabEvents);
	BindObserverMethods(state, tabsMetaTable, "onUpdated", tabUpdated);

	std::shared_ptr<Plugins::TabRemoved> tabRemoved =
		std::make_shared<Plugins::TabRemoved>(tabEvents);
	BindObserverMethods(state, tabsMetaTable, "onRemoved", tabRemoved);

	// clang-format off
	tabsMetaTable.new_usertype<Plugins::TabsApi::FolderSettings>("FolderSettings",
		"viewMode", &Plugins::TabsApi::FolderSettings::viewMode,
		"__tostring", &Plugins::TabsApi::FolderSettings::toString);

	tabsMetaTable.new_usertype<Plugins::TabsApi::Tab>("Tab",
		"id", &Plugins::TabsApi::Tab::id,
		"location", &Plugins::TabsApi::Tab::location,
		"name", &Plugins::TabsApi::Tab::name,
		"locked", &Plugins::TabsApi::Tab::locked,
		"addressLocked", &Plugins::TabsApi::Tab::addressLocked,
		"folderSettings", &Plugins::TabsApi::Tab::folderSettings,
		"__tostring", &Plugins::TabsApi::Tab::toString);
	// clang-format on

	AddEnum<ViewMode>(state, tabsMetaTable, "ViewMode");
	AddEnum<SortMode>(state, tabsMetaTable, "SortMode");
}

void BindMenuApi(sol::state &state, Plugins::PluginMenuManager *pluginMenuManager)
{
	std::shared_ptr<Plugins::MenuApi> menuApi =
		std::make_shared<Plugins::MenuApi>(pluginMenuManager);

	sol::table menuTable = state.create_named_table("menu");
	sol::table metaTable = MarkTableReadOnly(state, menuTable);

	metaTable.set_function("create", &Plugins::MenuApi::create, menuApi);
	metaTable.set_function("remove", &Plugins::MenuApi::remove, menuApi);
}

void BindUiApi(sol::state &state, UiTheming *uiTheming)
{
	std::shared_ptr<Plugins::UiApi> uiApi = std::make_shared<Plugins::UiApi>(uiTheming);

	sol::table uiTable = state.create_named_table("ui");
	sol::table metaTable = MarkTableReadOnly(state, uiTable);

	metaTable.set_function("setListViewColors", &Plugins::UiApi::setListViewColors, uiApi);
	metaTable.set_function("setTreeViewColors", &Plugins::UiApi::setTreeViewColors, uiApi);
}

void BindCommandApi(int pluginId, sol::state &state,
	Plugins::PluginCommandManager *pluginCommandManager)
{
	sol::table commandsTable = state.create_named_table("commands");
	sol::table commandsMetaTable = MarkTableReadOnly(state, commandsTable);

	std::shared_ptr<Plugins::CommandInvoked> commandInvoked =
		std::make_shared<Plugins::CommandInvoked>(pluginCommandManager, pluginId);
	BindObserverMethods(state, commandsMetaTable, "onCommand", commandInvoked);
}

template <typename T>
void BindObserverMethods(sol::state &state, sol::table &parentTable,
	const std::string &observerTableName, const std::shared_ptr<T> &object)
{
	static_assert(std::is_base_of<Plugins::Event, T>::value, "T must inherit from Plugins::Event");

	sol::table observerTable = parentTable.create_named(observerTableName);
	sol::table observerMetaTable = MarkTableReadOnly(state, observerTable);

	observerMetaTable.set_function("addListener", &T::addObserver, object);
	observerMetaTable.set_function("removeListener", &T::removeObserver, object);
}

// This is used instead of the new_enum function provided by Sol, as
// that function is designed for compile-time enum value lists (there is
// a variant that accepts std::initializer_list, but that's designed for
// static lists of items as well). There's no way to specify a list of
// enum values that are created at runtime.
// The new_enum function is simple enough to emulate though, as all it
// does is create a (possibly read-only) table with the specified enum
// values. Note that this function is only designed to work with Better
// Enums (though there's currently no static type check).
template <typename T>
void AddEnum(sol::state &state, sol::table &parentTable, const std::string &name)
{
	sol::table enumTable = parentTable.create_named(name);
	sol::table enumMetaTable = MarkTableReadOnly(state, enumTable);

	for (auto item : T::_values())
	{
		enumMetaTable.set(item._to_string(), item._to_integral());
	}
}

sol::table MarkTableReadOnly(sol::state &state, sol::table &table)
{
	sol::table metaTable = state.create_table();

	metaTable[sol::meta_function::new_index] = deny;
	metaTable[sol::meta_function::index] = metaTable;

	table[sol::metatable_key] = metaTable;

	return metaTable;
}

int deny(lua_State *state)
{
	return luaL_error(state, "Attempt to modify read-only table");
}

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "APIBinding.h"
#include "TabsAPI.h"

void BindTabsAPI(sol::state &state, IExplorerplusplus *pexpp, TabInterface *ti);
int deny(lua_State *state);

void Plugins::BindAllApiMethods(sol::state &state, IExplorerplusplus *pexpp, TabInterface *ti)
{
	BindTabsAPI(state, pexpp, ti);
}

void BindTabsAPI(sol::state &state, IExplorerplusplus *pexpp, TabInterface *ti)
{
	// This parameter is likely to be used again when other API methods
	// are added.
	UNREFERENCED_PARAMETER(pexpp);

	std::shared_ptr<Plugins::TabsApi> tabsApi = std::make_shared<Plugins::TabsApi>(ti);

	sol::table tabsTable = state.create_named_table("tabs");

	sol::table metaTable = state.create_table();
	metaTable.set_function("getAll", &Plugins::TabsApi::getAll, tabsApi);
	metaTable.set_function("get", &Plugins::TabsApi::get, tabsApi);
	metaTable.set_function("create", &Plugins::TabsApi::create, tabsApi);
	metaTable.set_function("move", &Plugins::TabsApi::move, tabsApi);
	metaTable.set_function("close", &Plugins::TabsApi::close, tabsApi);

	metaTable.new_usertype<Plugins::TabsApi::Tab>("Tab",
		"id", &Plugins::TabsApi::Tab::id,
		"location", &Plugins::TabsApi::Tab::location,
		"viewMode", &Plugins::TabsApi::Tab::viewMode,
		"locked", &Plugins::TabsApi::Tab::locked,
		"addressLocked", &Plugins::TabsApi::Tab::addressLocked,
		"__tostring", &Plugins::TabsApi::Tab::toString);

	metaTable.new_enum("ViewMode",
		"details", ViewMode::VM_DETAILS,
		"extraLargeIcons", ViewMode::VM_EXTRALARGEICONS,
		"icons", ViewMode::VM_ICONS,
		"largeIcons", ViewMode::VM_LARGEICONS,
		"list", ViewMode::VM_LIST,
		"smallIcons", ViewMode::VM_SMALLICONS,
		"thumbnails", ViewMode::VM_THUMBNAILS,
		"tiles", ViewMode::VM_TILES);

	metaTable[sol::meta_function::new_index] = deny;
	metaTable[sol::meta_function::index] = metaTable;

	tabsTable[sol::metatable_key] = metaTable;
}

int deny(lua_State *state)
{
	return luaL_error(state, "Attempt to modify read-only table");
}
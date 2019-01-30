// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "APIBinding.h"
#include "TabsAPI.h"

void BindTabsAPI(sol::state &state, IExplorerplusplus *pexpp);
int deny(lua_State *state);

void Plugins::BindAllApiMethods(sol::state &state, IExplorerplusplus *pexpp)
{
	BindTabsAPI(state, pexpp);
}

void BindTabsAPI(sol::state &state, IExplorerplusplus *pexpp)
{
	std::shared_ptr<Plugins::TabsApi> tabsApi = std::make_shared<Plugins::TabsApi>(pexpp);

	sol::table tabsTable = state.create_named_table("tabs");

	sol::table metaTable = state.create_table();
	metaTable.set_function("create", &Plugins::TabsApi::create, tabsApi);
	metaTable.set_function("get", &Plugins::TabsApi::get, tabsApi);

	metaTable.new_usertype<Plugins::TabsApi::Tab>("Tab",
		"location", &Plugins::TabsApi::Tab::location,
		"__tostring", &Plugins::TabsApi::Tab::toString);

	metaTable[sol::meta_function::new_index] = deny;
	metaTable[sol::meta_function::index] = metaTable;

	tabsTable[sol::metatable_key] = metaTable;
}

int deny(lua_State *state)
{
	return luaL_error(state, "Attempt to modify read-only table");
}
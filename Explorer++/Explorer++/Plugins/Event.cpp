// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/Event.h"
#include "SolWrapper.h"

Plugins::Event::Event() :
	m_connectionIdCounter(1)
{

}

Plugins::Event::~Event()
{
	for (auto &item : m_connections)
	{
		item.second.disconnect();
	}
}

int Plugins::Event::addObserver(sol::protected_function observer, sol::this_state state)
{
	if (!observer)
	{
		return -1;
	}

	auto connection = connectObserver(observer, state);

	int id = m_connectionIdCounter++;
	m_connections.insert(std::make_pair(id, connection));

	return id;
}

void Plugins::Event::removeObserver(int id)
{
	auto itr = m_connections.find(id);

	if (itr == m_connections.end())
	{
		return;
	}

	itr->second.disconnect();

	m_connections.erase(itr);
}
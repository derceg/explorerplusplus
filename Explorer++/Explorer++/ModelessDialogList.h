// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <string>
#include <unordered_map>

// Maintains an unordered list of open modeless dialogs.
class ModelessDialogList
{
public:
	void AddDialog(std::wstring id, HWND dialog);
	void RemoveDialog(std::wstring id);

	concurrencpp::generator<HWND> GetList() const;
	HWND MaybeGetDialogById(std::wstring id) const;

private:
	std::unordered_map<std::wstring, HWND> m_dialogMap;
};

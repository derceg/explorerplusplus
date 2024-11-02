// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <functional>
#include <string>

class BaseDialog;
class ModelessDialogList;

void CreateOrSwitchToModelessDialog(ModelessDialogList *modelessDialogList, const std::wstring &id,
	std::function<BaseDialog *()> dialogCreator);

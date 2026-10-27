// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarViewBase.h"
#include <memory>
#include <vector>

// This represents a toolbar that can be customized externally (i.e. the customization is managed by
// the native control). In particular, this means that this class only owns the set of available
// buttons; the actual buttons shown to the user are almost fully managed by the native control. The
// only exception being the initial set of buttons, which are added via this class.
//
// That also means that this class provides no programmatic customization points. That is, it's not
// possible to add or remove buttons programmatically. The set of available buttons is fixed and the
// user can adjust the buttons that are shown by using the customization options built into the
// native control.
class CustomizableToolbarView : public ToolbarViewBase
{
public:
	CustomizableToolbarView(HWND parent, DWORD style, DWORD extendedStyle,
		MenuHelpTextHost *menuHelpTextHost, const Config *config);

	// This specifies the available buttons (i.e. the complete set of buttons that can be shown).
	// This method should be called before attempting to set the default/current items.
	void InitializeAvailableButtons(std::vector<std::unique_ptr<ToolbarButton>> buttons);

	// When the toolbar state is reset, using the native customization dialog, the buttons will
	// revert back the items provided here. Both the button passed in here, as well as the buttons
	// passed to SetItems(), must be part of the available set of buttons.
	void SetDefaultItems(const std::vector<ToolbarItem> &items);

	// This sets the current set of items. Any existing items will be removed first.
	void SetItems(const std::vector<ToolbarItem> &items);

	void StartCustomization();

private:
	// ToolbarViewBase
	std::optional<LRESULT> MaybeHandleNotification(NMHDR *notification) override;
	ToolbarButton *GetButtonById(int id) const override;

	bool OnGetButtonInfo(NMTOOLBAR *data);
	void OnReset();
	void OnButtonsChanged();
	void ClearExistingButtons();
	int GetButtonId(const ToolbarButton *button) const;

	// This represents the complete set of available buttons. Only a subset of the buttons may be
	// shown on the toolbar.
	std::vector<ButtonEntry> m_availableButtons;

	bool m_availableButtonsInitialized = false;

	std::vector<ToolbarItem> m_defaultItems;
};

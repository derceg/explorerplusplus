// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "ColorRule.h"
#include "../Helper/DialogSettings.h"
#include <memory>
#include <vector>

class ColorRuleEditorDialog;
class ColorRuleModel;
class WindowSubclass;

class ColorRuleEditorDialogPersistentSettings : public DialogSettings
{
public:
	static ColorRuleEditorDialogPersistentSettings &GetInstance();

private:
	friend ColorRuleEditorDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_CUSTOM_COLORS[];

	ColorRuleEditorDialogPersistentSettings();

	ColorRuleEditorDialogPersistentSettings(const ColorRuleEditorDialogPersistentSettings &);
	ColorRuleEditorDialogPersistentSettings &operator=(
		const ColorRuleEditorDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	COLORREF m_customColors[16];
};

class ColorRuleEditorDialog : public BaseDialog
{
public:
	class EditDetails
	{
	private:
		struct Token
		{
		private:
			Token() = default;
			friend EditDetails;
		};

	public:
		enum class Type
		{
			ExistingItem,
			NewItem
		};

		EditDetails(Type type, Token) : type(type)
		{
		}

		static std::unique_ptr<EditDetails> AddNewColorRule(std::unique_ptr<ColorRule> colorRule,
			std::optional<size_t> index = std::nullopt)
		{
			auto editDetails = std::make_unique<EditDetails>(Type::NewItem, Token());
			editDetails->newColorRule = std::move(colorRule);
			editDetails->index = index;
			return editDetails;
		}

		static std::unique_ptr<EditDetails> EditColorRule(ColorRule *colorRule)
		{
			auto editDetails = std::make_unique<EditDetails>(Type::ExistingItem, Token());
			editDetails->existingColorRule = colorRule;
			return editDetails;
		}

		const Type type;

		std::unique_ptr<ColorRule> newColorRule;
		std::optional<size_t> index;

		ColorRule *existingColorRule = nullptr;
	};

	ColorRuleEditorDialog(const ResourceLoader *resourceLoader, HWND parent, ColorRuleModel *model,
		std::unique_ptr<EditDetails> editDetails);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	void SaveState() override;

private:
	LRESULT StaticColorControlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void OnChangeColor();

	void OnOk();
	void OnCancel();

	void ApplyEdits(std::wstring newDescription, std::wstring newFilterPattern,
		bool newFilterPatternCaseInsensitive, DWORD newFilterAttributes, COLORREF newColor);

	ColorRuleModel *m_model;
	std::unique_ptr<EditDetails> m_editDetails;

	COLORREF m_currentColor;

	ColorRuleEditorDialogPersistentSettings *m_persistentSettings;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
};

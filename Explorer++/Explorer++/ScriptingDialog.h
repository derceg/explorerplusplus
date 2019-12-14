#pragma once

#include "LuaPlugin.h"
#include "PluginInterface.h"
#include "PluginMenuManager.h"
#include "UiTheming.h"
#include "../Helper/BaseDialog.h"

class ScriptingDialog : public CBaseDialog
{
public:

	ScriptingDialog(HINSTANCE hInstance, HWND hParent, PluginInterface *pluginInterface);

protected:

	INT_PTR OnInitDialog();
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	void OnRun();
	INT_PTR OnClose();
	INT_PTR	OnNcDestroy();

private:

	void GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);

	std::wstring FormatResult(const sol::protected_function_result &result);
	void AppendToLog(const std::wstring &command, const std::wstring &result);

	Plugins::LuaPlugin m_luaPlugin;
};
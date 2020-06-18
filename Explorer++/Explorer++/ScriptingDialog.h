#pragma once

#include "DarkModeDialogBase.h"
#include "PluginInterface.h"
#include "Plugins/LuaPlugin.h"

class ScriptingDialog : public DarkModeDialogBase
{
public:
	ScriptingDialog(HINSTANCE hInstance, HWND hParent, PluginInterface *pluginInterface);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	void OnRun();
	INT_PTR OnClose() override;
	INT_PTR OnNcDestroy() override;

private:
	void GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
		std::list<ResizableDialog::Control> &ControlList) override;

	std::wstring FormatResult(const sol::protected_function_result &result);
	void AppendToLog(const std::wstring &command, const std::wstring &result);

	Plugins::LuaPlugin m_luaPlugin;
};
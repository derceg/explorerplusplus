#pragma once

#include "PluginInterface.h"
#include "Plugins/LuaPlugin.h"
#include "ThemedDialog.h"

class ScriptingDialog : public ThemedDialog
{
public:
	ScriptingDialog(HINSTANCE resourceInstance, HWND hParent, ThemeManager *themeManager,
		PluginInterface *pluginInterface);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	void OnRun();
	INT_PTR OnClose() override;
	INT_PTR OnNcDestroy() override;

private:
	std::vector<ResizableDialogControl> GetResizableControls() override;

	std::wstring FormatResult(const sol::protected_function_result &result);
	void AppendToLog(const std::wstring &command, const std::wstring &result);

	Plugins::LuaPlugin m_luaPlugin;
};

#pragma once

#include "BaseDialog.h"
#include "Plugins/LuaPlugin.h"

struct Config;
class PluginInterface;

class ScriptingDialog : public BaseDialog
{
public:
	ScriptingDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, HWND hParent,
		PluginInterface *pluginInterface, const Config *config);

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

#pragma once

#include "Explorer++_internal.h"
#include "LuaPlugin.h"
#include "../Helper/BaseDialog.h"

class ScriptingDialog : public CBaseDialog
{
public:

	ScriptingDialog(HINSTANCE hInstance, int iResource, HWND hParent, IExplorerplusplus *pexpp);
	~ScriptingDialog();

protected:

	INT_PTR OnInitDialog();
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	void OnRun();
	INT_PTR OnClose();
	INT_PTR	OnNcDestroy();

private:

	void GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);

	void AppendToLog(const std::wstring &command, const std::wstring &result);

	Plugins::LuaPlugin m_luaPlugin;
};
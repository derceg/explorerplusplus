#ifndef APPLICATIONTOOLBARBUTTONDIALOG_INCLUDED
#define APPLICATIONTOOLBARBUTTONDIALOG_INCLUDED

#include "ApplicationToolbar.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

class CApplicationToolbarButtonDialog : public CBaseDialog
{
public:

	CApplicationToolbarButtonDialog(HINSTANCE hInstance,int iResource,HWND hParent,ApplicationButton_t *Button,bool IsNew);
	~CApplicationToolbarButtonDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

private:

	void	OnChooseFile();

	void	OnOk();
	void	OnCancel();

	ApplicationButton_t	*m_Button;
	bool				m_IsNew;
};

#endif
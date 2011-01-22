#ifndef RENAMETABDIALOG_INCLUDED
#define RENAMETABDIALOG_INCLUDED

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

class CRenameTabDialog;

class CRenameTabDialogPersistentSettings : public CDialogSettings
{
public:

	~CRenameTabDialogPersistentSettings();

	static CRenameTabDialogPersistentSettings &GetInstance();

private:

	friend CRenameTabDialog;

	static const TCHAR SETTINGS_KEY[];

	CRenameTabDialogPersistentSettings();

	CRenameTabDialogPersistentSettings(const CRenameTabDialogPersistentSettings &);
	CRenameTabDialogPersistentSettings & operator=(const CRenameTabDialogPersistentSettings &);
};

class CRenameTabDialog : public CBaseDialog
{
public:

	CRenameTabDialog(HINSTANCE hInstance,int iResource,HWND hParent);
	~CRenameTabDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnClose();
	BOOL	OnDestroy();

private:

	void	OnUseFolderName();
	void	OnUseCustomName();
	void	OnOk();
	void	OnCancel();

	void	SaveState();

	CRenameTabDialogPersistentSettings	*m_prtdps;

	TCHAR	m_szTabName[256];
};

#endif
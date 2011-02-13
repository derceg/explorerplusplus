#ifndef SETFILEATTRIBUTES_INCLUDED
#define SETFILEATTRIBUTES_INCLUDED

#include <list>
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

namespace NSetFileAttributesDialogExternal
{
	struct SetFileAttributesInfo_t
	{
		TCHAR			szFullFileName[MAX_PATH];
		WIN32_FIND_DATA	wfd;
	};
}

class CSetFileAttributesDialog;

class CSetFileAttributesDialogPersistentSettings : public CDialogSettings
{
public:

	~CSetFileAttributesDialogPersistentSettings();

	static CSetFileAttributesDialogPersistentSettings &GetInstance();

private:

	friend CSetFileAttributesDialog;

	static const TCHAR SETTINGS_KEY[];

	CSetFileAttributesDialogPersistentSettings();

	CSetFileAttributesDialogPersistentSettings(const CSetFileAttributesDialogPersistentSettings &);
	CSetFileAttributesDialogPersistentSettings & operator=(const CSetFileAttributesDialogPersistentSettings &);
};

class CSetFileAttributesDialog : public CBaseDialog
{
public:

	CSetFileAttributesDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList);
	~CSetFileAttributesDialog();

protected:

	BOOL	OnInitDialog();
	BOOL	OnCommand(WPARAM wParam,LPARAM lParam);
	BOOL	OnNotify(NMHDR *pnmhdr);
	BOOL	OnClose();

	void	SaveState();

private:

	typedef struct
	{
		DWORD Attribute;
		UINT uControlId;
		UINT uChecked;

		/* Set if the attribute state is on
		when the corresponding GUI element is
		'off'. */
		BOOL bReversed;
	} Attribute_t;

	enum DateTimeType_t
	{
		DATE_TIME_MODIFIED,
		DATE_TIME_CREATED,
		DATE_TIME_ACCESSED
	};

	void	InitializeAttributesStructure(void);

	void	ResetButtonState(HWND hwnd,BOOL bReset);
	void	SetAttributeCheckState(HWND hwnd,int nAttributes,int nSelected);

	void	InitializeDateFields();
	void	OnDateReset(DateTimeType_t DateTimeType);
	void	OnOk();
	void	OnCancel();

	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t>	m_FileList;
	std::list<Attribute_t>	m_AttributeList;

	CSetFileAttributesDialogPersistentSettings	*m_psfadps;

	SYSTEMTIME m_LocalWrite;
	SYSTEMTIME m_LocalCreation;
	SYSTEMTIME m_LocalAccess;

	BOOL	m_bModificationDateEnabled;
	BOOL	m_bCreationDateEnabled;
	BOOL	m_bAccessDateEnabled;
};

#endif
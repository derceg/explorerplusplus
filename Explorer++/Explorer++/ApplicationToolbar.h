#ifndef APPLICATIONTOOLBAR_INCLUDED
#define APPLICATIONTOOLBAR_INCLUDED

#include <vector>
#include "Explorer++_internal.h"
#include "ApplicationToolbarDropHandler.h"

#import <msxml3.dll> raw_interfaces_only

class CApplicationToolbar;

struct ApplicationButton_t
{
	std::wstring	Name;
	std::wstring	Command;
	BOOL			ShowNameOnToolbar;

	int				ID;
};

class CApplicationToolbarPersistentSettings
{
public:

	~CApplicationToolbarPersistentSettings();

	static CApplicationToolbarPersistentSettings &GetInstance();

	void	SaveRegistrySettings(HKEY hParentKey);
	void	LoadRegistrySettings(HKEY hParentKey);

	void	SaveXMLSettings(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe);
	void	LoadXMLSettings(MSXML2::IXMLDOMNode *pNode);

private:

	friend CApplicationToolbar;

	CApplicationToolbarPersistentSettings();

	CApplicationToolbarPersistentSettings(const CApplicationToolbarPersistentSettings &);
	CApplicationToolbarPersistentSettings & operator=(const CApplicationToolbarPersistentSettings &);

	void AddButton(std::wstring Name,std::wstring Command,BOOL ShowNameOnToolbar);

	std::vector<ApplicationButton_t> m_Buttons;
	int m_IDCounter;
};

class CApplicationToolbar
{
	friend LRESULT CALLBACK ParentProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	CApplicationToolbar(HWND hToolbar,UINT uIDStart,UINT uIDEnd,HINSTANCE hInstance,IExplorerplusplus *pexpp);
	~CApplicationToolbar();

private:

	static const UINT_PTR PARENT_SUBCLASS_ID = 1;

	struct ApplicationInfo_t
	{
		std::wstring Application;
		std::wstring Parameters;
	};

	LRESULT CALLBACK	ParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void				Initialize();

	void				NewItem();
	void				OpenItem(int iItem);
	void				ShowItemProperties(int iItem);
	void				DeleteItem(int iItem);

	void				AddButtonsToToolbar();
	void				AddButtonToToolbar(const ApplicationButton_t &Button);
	void				UpdateButton(int iItem);

	ApplicationInfo_t	ProcessCommand(const std::wstring &Command);
	ApplicationButton_t	*MapToolbarButtonToItem(int index);

	HWND				m_hToolbar;

	HINSTANCE			m_hInstance;

	UINT				m_uIDStart;
	UINT				m_uIDEnd;

	int					m_RightClickItem;

	IExplorerplusplus	*m_pexpp;

	CApplicationToolbarDropHandler	*m_patd;

	CApplicationToolbarPersistentSettings	*m_atps;
};

#endif
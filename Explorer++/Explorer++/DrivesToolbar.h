#ifndef DRIVESTOOLBAR_INCLUDED
#define DRIVESTOOLBAR_INCLUDED

#include <list>
#include <unordered_map>
#include <boost\serialization\strong_typedef.hpp>

class CDrivesToolbar
{
	friend LRESULT CALLBACK DrivesToolbarProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
	friend LRESULT CALLBACK DrivesToolbarParentProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	CDrivesToolbar(HWND hToolbar,UINT uIDStart,UINT uIDEnd,IExplorerplusplus *pexpp);
	~CDrivesToolbar();

private:

	BOOST_STRONG_TYPEDEF(UINT,IDCounter);

	struct DriveInformation_t
	{
		int			Position;
		IDCounter	ID;
	};

	static const UINT_PTR SUBCLASS_ID = 0;
	static CONST UINT_PTR PARENT_SUBCLASS_ID = 0;

	LRESULT CALLBACK DrivesToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	LRESULT CALLBACK DrivesToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void		InitializeToolbar();

	void		InsertDrives();
	void		InsertDrive(const std::wstring &DrivePath);
	void		RemoveDrive(const std::wstring &DrivePath);

	int			GetSortedPosition(const std::wstring &DrivePath);
	DriveInformation_t GetDrivePosition(const std::wstring &DrivePath);
	std::wstring GetDrivePath(int iIndex);

	void		UpdateDriveIcon(const std::wstring &DrivePath);

	HWND		m_hToolbar;

	UINT		m_uIDStart;
	UINT		m_uIDEnd;

	IExplorerplusplus *m_pexpp;

	std::unordered_map<IDCounter,std::wstring> m_mapID;
	IDCounter	m_IDCounter;
};

#endif
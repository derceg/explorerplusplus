#ifndef CUSTOMMENU_INCLUDED
#define CUSTOMMENU_INCLUDED

/* ALL custom menu item's must
have this data structure attached. */
typedef struct
{
	int			iImage;
	BOOL		bUseImage;
	ULONG_PTR	dwItemData;
} CustomMenuInfo_t;

class CCustomMenu
{
public:

	CCustomMenu(HWND hwnd,HMENU hMenu,HIMAGELIST himl);
	~CCustomMenu(void);

	void	OnInitMenu(WPARAM wParam);
	BOOL	OnMeasureItem(WPARAM wParam,LPARAM lParam);
	BOOL	OnDrawItem(WPARAM wParam,LPARAM lParam);

private:

	HWND m_hwnd;
	HMENU m_hMenu;
	HIMAGELIST m_himl;
};

#endif
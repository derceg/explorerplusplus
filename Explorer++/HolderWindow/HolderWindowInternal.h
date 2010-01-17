#ifndef HOLDERWINDOWINTERNAL_INCLUDED
#define HOLDERWINDOWINTERNAL_INCLUDED

/* Used for testing what version of Windows we're running on. */
#define WINDOWS_VISTA_MAJORVERSION	6
#define WINDOWS_XP_MAJORVERSION		5

class CHolderWindow
{
public:

	CHolderWindow(HWND hHolder);
	~CHolderWindow();

	LRESULT CALLBACK	HolderWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

private:

	void	OnHolderWindowPaint(HWND hwnd);
	void	OnHolderWindowLButtonDown(LPARAM lParam);
	void	OnHolderWindowLButtonUp(void);
	int		OnHolderWindowMouseMove(LPARAM lParam);


	HWND	m_hHolder;
	DWORD	m_dwMajorVersion;
	BOOL	m_bHolderResizing;
};

#endif
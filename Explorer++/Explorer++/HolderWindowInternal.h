#pragma once

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
	BOOL	m_bHolderResizing;
};
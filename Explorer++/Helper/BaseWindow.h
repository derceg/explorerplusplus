#ifndef BASEWINDOW_INCLUDED
#define BASEWINDOW_INCLUDED

#include "MessageForwarder.h"

class CBaseWindow : public CMessageForwarder
{
	friend LRESULT CALLBACK BaseWindowProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	CBaseWindow(HWND hwnd);
	~CBaseWindow();

protected:

	INT_PTR	GetDefaultReturnValue(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	LRESULT CALLBACK BaseWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

#endif
#pragma once

namespace NListView
{
	void	ListView_SelectItem(HWND hListView,int iItem,BOOL bSelect);
	void	ListView_SelectAllItems(HWND hListView,BOOL bSelect);
	int		ListView_InvertSelection(HWND hListView);
	void	ListView_FocusItem(HWND hListView,int iItem,BOOL bFocus);
	void	ListView_SetGridlines(HWND hListView,BOOL bEnableGridlines);
	BOOL	ListView_SetAutoArrange(HWND hListView,BOOL bAutoArrange);
	void	ListView_ActivateOneClickSelect(HWND hListView,BOOL bActivate,UINT uHoverTime);
	void	ListView_AddRemoveExtendedStyle(HWND hListView,DWORD dwStyle,BOOL bAdd);
	BOOL	ListView_SetBackgroundImage(HWND hListView,UINT uImage);
	BOOL	ListView_SwapItems(HWND hListView,int iItem1,int iItem2,BOOL bSwapLPARAM);
	void	ListView_PositionInsertMark(HWND hListView,const POINT *ppt);
}
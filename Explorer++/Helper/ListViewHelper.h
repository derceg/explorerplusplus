// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace ListViewHelper
{
	void	SelectItem(HWND hListView,int iItem,BOOL bSelect);
	void	SelectAllItems(HWND hListView,BOOL bSelect);
	int		InvertSelection(HWND hListView);
	void	FocusItem(HWND hListView,int iItem,BOOL bFocus);
	void	SetGridlines(HWND hListView,BOOL bEnableGridlines);
	BOOL	SetAutoArrange(HWND hListView,BOOL bAutoArrange);
	void	ActivateOneClickSelect(HWND hListView,BOOL bActivate,UINT uHoverTime);
	void	AddRemoveExtendedStyle(HWND hListView,DWORD dwStyle,BOOL bAdd);
	BOOL	SetBackgroundImage(HWND hListView,UINT uImage);
	BOOL	SwapItems(HWND hListView,int iItem1,int iItem2,BOOL bSwapLPARAM);
	void	PositionInsertMark(HWND hListView,const POINT *ppt);
}
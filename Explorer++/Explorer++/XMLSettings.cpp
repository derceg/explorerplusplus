// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Loads and saves all settings associated with
 * Explorer++ from/to an XML configuration file.
 *
 * Notes:
 *  - Attribute names and values must conform to
 *    the following rules:
 *     - No spaces
 *     - First character cannot be a number
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "ColumnXmlStorage.h"
#include "Config.h"
#include "CustomFontStorage.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "MainRebarStorage.h"
#include "MainRebarXmlStorage.h"
#include "MainToolbar.h"
#include "MainToolbarStorage.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "TabStorage.h"
#include "TabXmlStorage.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>
#include <wil/resource.h>

/* These represent the pre-hashed values of attribute
names. They are used to avoid string comparisons
on each attribute. If the hash function or any of
the attribute names change in any way, these values
will need to be changed correspondingly. */
#define HASH_ALWAYSOPENINNEWTAB 1123321600
#define HASH_AUTOARRANGEGLOBAL 151507311
#define HASH_CONFIRMCLOSETABS 2636757395
#define HASH_DISPLAYCENTRECOLOR 3404143227
#define HASH_DISPLAYFONT 362757714
#define HASH_DISPLAYSURROUNDCOLOR 1807564604
#define HASH_DISPLAYTEXTCOLOR 4212809823
#define HASH_DISPLAYWINDOWWIDTH 3332824435
#define HASH_DISPLAYWINDOWHEIGHT 2017415020
#define HASH_DISPLAYWINDOWVERTICAL 2262072301
#define HASH_LANGUAGE 3526403497
#define HASH_LASTSELECTEDTAB 1712438393
#define HASH_NEXTTOCURRENT 743165450
#define HASH_SHOWADDRESSBAR 3302864385
#define HASH_SHOWBOOKMARKSTOOLBAR 1216493954
#define HASH_SHOWDRIVESTOOLBAR 899091590
#define HASH_SHOWDISPLAYWINDOW 351410676
#define HASH_SHOWEXTENSIONS 3743594966
#define HASH_SHOWFOLDERS 948345109
#define HASH_SHOWFOLDERSIZES 3684676528
#define HASH_SHOWFRIENDLYDATES 467626964
#define HASH_SHOWFULLTITLEPATH 1871292168
#define HASH_SHOWGRIDLINESGLOBAL 1707929656
#define HASH_SHOWHIDDENGLOBAL 558199811
#define HASH_SHOWSTATUSBAR 3554629247
#define HASH_SHOWINFOTIPS 3018038962
#define HASH_SHOWTOOLBAR 1852868921
#define HASH_SORTASCENDINGGLOBAL 2605638058
#define HASH_STARTUPMODE 1344265373
#define HASH_TOOLBARSTATE 3436473849
#define HASH_TREEVIEWDELAYENABLED 2186637066
#define HASH_TREEVIEWWIDTH 4257779536
#define HASH_VIEWMODEGLOBAL 3743629718
#define HASH_POSITION 3300187802
#define HASH_LOCKTOOLBARS 3842965076
#define HASH_NEWTABDIRECTORY 3570078203
#define HASH_INFOTIPTYPE 3366492864
#define HASH_SHOWAPPLICATIONTOOLBAR 101571053
#define HASH_USEFULLROWSELECT 3780943197
#define HASH_SHOWINGROUPSGLOBAL 4239388334
#define HASH_EXTENDTABCONTROL 4097866437
#define HASH_SHOWFILEPREVIEWS 1834921243
#define HASH_REPLACEEXPLORERMODE 2422294263
#define HASH_SHOWUSERNAMETITLEBAR 2618183549
#define HASH_HIDESYSTEMFILESGLOBAL 1667356744
#define HASH_HIDELINKEXTENSIONGLOBAL 1073100667
#define HASH_ALLOWMULTIPLEINSTANCES 3463984536
#define HASH_ONECLICKACTIVATE 1118178238
#define HASH_ONECLICKACTIVATEHOVERTIME 3023373873
#define HASH_DOUBLECLICKTABCLOSE 1866215987
#define HASH_HANDLEZIPFILES 1074212343
#define HASH_INSERTSORTED 1109371947
#define HASH_SHOWPRIVILEGETITLEBAR 4071561587
#define HASH_DISABLEFOLDERSIZENETWORKREMOVABLE 2610679594
#define HASH_ALWAYSSHOWTABBAR 148004675
#define HASH_CHECKBOXSELECTION 456677010
#define HASH_FORCESIZE 1918861263
#define HASH_SIZEDISPLAYFOMRAT 3548127263
#define HASH_CLOSEMAINWINDOWONTABCLOSE 1151827266
#define HASH_SHOWTABBARATBOTTOM 4099029340
#define HASH_SHOWTASKBARTHUMBNAILS 2202555045
#define HASH_SYNCHRONIZETREEVIEW 1687787660
#define HASH_TVAUTOEXPAND 1228854897
#define HASH_OVERWRITEEXISTINGFILESCONFIRMATION 1625342835
#define HASH_LARGETOOLBARICONS 10895007
#define HASH_ICON_THEME 3998265761
#define HASH_CHECK_PINNED_TO_NAMESPACE_TREE_PROPERTY 145831142
#define HASH_SHOW_QUICK_ACCESS_IN_TREEVIEW 4287034967
#define HASH_THEME 237620728
#define HASH_ENABLE_DARK_MODE 1623404723
#define HASH_DISPLAY_MIXED_FILES_AND_FOLDERS 1168704423
#define HASH_USE_NATURAL_SORT_ORDER 528323501
#define HASH_OPEN_TABS_IN_FOREGROUND 2957281235
#define HASH_GROUP_SORT_DIRECTION_GLOBAL 790225996
#define HASH_GO_UP_ON_DOUBLE_CLICK 1809284638
#define HASH_MAIN_FONT 3006124449

struct ColumnXMLSaveData
{
	TCHAR szName[64];
	ColumnType type;
};

unsigned long hash_setting(unsigned char *str);

BOOL LoadWindowPositionFromXML(WINDOWPLACEMENT *pwndpl)
{
	wil::com_ptr_nothrow<IXMLDOMDocument> pXMLDom;
	pXMLDom.attach(NXMLSettings::DomFromCOM());

	if (!pXMLDom)
	{
		return FALSE;
	}

	TCHAR szConfigFile[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szConfigFile, SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	wil::unique_variant var(NXMLSettings::VariantString(szConfigFile));
	VARIANT_BOOL status;
	pXMLDom->load(var, &status);

	if (status != VARIANT_TRUE)
	{
		return FALSE;
	}

	wil::com_ptr_nothrow<IXMLDOMNodeList> pNodes;
	auto bstr = wil::make_bstr_nothrow(L"//WindowPosition/*");
	pXMLDom->selectNodes(bstr.get(), &pNodes);

	if (!pNodes)
	{
		return FALSE;
	}

	pwndpl->length = sizeof(WINDOWPLACEMENT);

	long length;
	pNodes->get_length(&length);

	/* There should only be one node
	under 'WindowPosition'. */
	if (length != 1)
	{
		return FALSE;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> pNode;
	pNodes->get_item(0, &pNode);

	wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
	HRESULT hr = pNode->get_attributes(&am);

	if (SUCCEEDED(hr))
	{
		long nChildNodes;
		am->get_length(&nChildNodes);

		for (long i = 1; i < nChildNodes; i++)
		{
			wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
			am->get_item(i, &pChildNode);

			wil::unique_bstr bstrName;
			pChildNode->get_nodeName(&bstrName);

			wil::unique_bstr bstrValue;
			pChildNode->get_text(&bstrValue);

			if (lstrcmp(bstrName.get(), _T("Flags")) == 0)
			{
				pwndpl->flags = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("ShowCmd")) == 0)
			{
				pwndpl->showCmd = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("MinPositionX")) == 0)
			{
				pwndpl->ptMinPosition.x = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("MinPositionY")) == 0)
			{
				pwndpl->ptMinPosition.y = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("MaxPositionX")) == 0)
			{
				pwndpl->ptMaxPosition.x = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("MaxPositionY")) == 0)
			{
				pwndpl->ptMaxPosition.y = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("NormalPositionLeft")) == 0)
			{
				pwndpl->rcNormalPosition.left = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("NormalPositionTop")) == 0)
			{
				pwndpl->rcNormalPosition.top = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("NormalPositionRight")) == 0)
			{
				pwndpl->rcNormalPosition.right = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), _T("NormalPositionBottom")) == 0)
			{
				pwndpl->rcNormalPosition.bottom = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
		}
	}

	return TRUE;
}

BOOL LoadAllowMultipleInstancesFromXML()
{
	BOOL bAllowMultipleInstances = TRUE;

	wil::com_ptr_nothrow<IXMLDOMDocument> pXMLDom;
	pXMLDom.attach(NXMLSettings::DomFromCOM());

	if (!pXMLDom)
	{
		return bAllowMultipleInstances;
	}

	TCHAR szConfigFile[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szConfigFile, SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	VARIANT_BOOL status;
	wil::unique_variant var(NXMLSettings::VariantString(szConfigFile));
	pXMLDom->load(var, &status);

	if (status != VARIANT_TRUE)
	{
		return bAllowMultipleInstances;
	}

	wil::com_ptr_nothrow<IXMLDOMNodeList> pNodes;
	auto bstr = wil::make_bstr_nothrow(L"//Settings/*");
	pXMLDom->selectNodes(bstr.get(), &pNodes);

	if (!pNodes)
	{
		return bAllowMultipleInstances;
	}

	long length;
	pNodes->get_length(&length);

	for (long i = 0; i < length; i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pNode;
		pNodes->get_item(i, &pNode);

		wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
		HRESULT hr = pNode->get_attributes(&am);

		if (SUCCEEDED(hr))
		{
			wil::com_ptr_nothrow<IXMLDOMNode> pNodeAttribute;
			hr = am->get_item(0, &pNodeAttribute);

			if (SUCCEEDED(hr))
			{
				wil::unique_bstr bstrName;
				pNodeAttribute->get_text(&bstrName);

				wil::unique_bstr bstrValue;
				pNode->get_text(&bstrValue);

				if (lstrcmp(bstrName.get(), _T("AllowMultipleInstances")) == 0)
				{
					bAllowMultipleInstances = NXMLSettings::DecodeBoolValue(bstrValue.get());
					break;
				}
			}
		}
	}

	return bAllowMultipleInstances;
}

void Explorerplusplus::LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom)
{
	if (!pXMLDom)
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNodeList> pNodes;
	auto bstr = wil::make_bstr_nothrow(L"//Settings/*");
	pXMLDom->selectNodes(bstr.get(), &pNodes);

	if (!pNodes)
	{
		return;
	}

	long length;
	pNodes->get_length(&length);

	for (long i = 0; i < length; i++)
	{
		wil::com_ptr_nothrow<IXMLDOMNode> pNode;
		pNodes->get_item(i, &pNode);

		wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
		HRESULT hr = pNode->get_attributes(&am);

		if (SUCCEEDED(hr))
		{
			wil::com_ptr_nothrow<IXMLDOMNode> pNodeAttribute;
			hr = am->get_item(0, &pNodeAttribute);

			if (SUCCEEDED(hr))
			{
				wil::unique_bstr bstrName;
				pNodeAttribute->get_text(&bstrName);

				wil::unique_bstr bstrValue;
				pNode->get_text(&bstrValue);

				/* Map the external attribute and value to an
				internal variable. */
				MapAttributeToValue(pNode.get(), bstrName.get(), bstrValue.get());
			}
		}
	}
}

void Explorerplusplus::SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot)
{
	wil::com_ptr_nothrow<IXMLDOMElement> pe;
	auto bstr = wil::make_bstr_nothrow(L"Settings");
	pXMLDom->createElement(bstr.get(), &pe);

	auto bstr_wsntt = wil::make_bstr_nothrow(L"\n\t\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("AllowMultipleInstances"),
		NXMLSettings::EncodeBoolValue(m_config->allowMultipleInstances));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("AlwaysOpenInNewTab"),
		NXMLSettings::EncodeBoolValue(m_config->alwaysOpenNewTab));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("AlwaysShowTabBar"),
		NXMLSettings::EncodeBoolValue(m_config->alwaysShowTabBar.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("AutoArrangeGlobal"),
		NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.autoArrange));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("CheckBoxSelection"),
		NXMLSettings::EncodeBoolValue(m_config->checkBoxSelection.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("CloseMainWindowOnTabClose"),
		NXMLSettings::EncodeBoolValue(m_config->closeMainWindowOnTabClose));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ConfirmCloseTabs"),
		NXMLSettings::EncodeBoolValue(m_config->confirmCloseTabs));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("DisableFolderSizesNetworkRemovable"),
		NXMLSettings::EncodeBoolValue(
			m_config->globalFolderSettings.disableFolderSizesNetworkRemovable));

	COLORREF centreColor;

	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayCentreColor"));
	centreColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETCENTRECOLOR, 0, 0);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		NXMLSettings::EncodeIntValue(GetRValue(centreColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		NXMLSettings::EncodeIntValue(GetGValue(centreColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		NXMLSettings::EncodeIntValue(GetBValue(centreColor)));

	HFONT hFont;
	LOGFONT fontInfo;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayFont"));
	SendMessage(m_hDisplayWindow, DWM_GETFONT, (WPARAM) &hFont, 0);
	GetObject(hFont, sizeof(LOGFONT), &fontInfo);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Height"),
		NXMLSettings::EncodeIntValue(fontInfo.lfHeight));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Width"),
		NXMLSettings::EncodeIntValue(fontInfo.lfWidth));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Weight"),
		NXMLSettings::EncodeIntValue(fontInfo.lfWeight));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Italic"),
		NXMLSettings::EncodeBoolValue(fontInfo.lfItalic));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Underline"),
		NXMLSettings::EncodeBoolValue(fontInfo.lfUnderline));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Strikeout"),
		NXMLSettings::EncodeBoolValue(fontInfo.lfStrikeOut));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Font"), fontInfo.lfFaceName);

	COLORREF surroundColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplaySurroundColor"));
	surroundColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETSURROUNDCOLOR, 0, 0);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		NXMLSettings::EncodeIntValue(GetRValue(surroundColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		NXMLSettings::EncodeIntValue(GetGValue(surroundColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		NXMLSettings::EncodeIntValue(GetBValue(surroundColor)));

	COLORREF textColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayTextColor"));
	textColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETTEXTCOLOR, 0, 0);
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		NXMLSettings::EncodeIntValue(GetRValue(textColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		NXMLSettings::EncodeIntValue(GetGValue(textColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		NXMLSettings::EncodeIntValue(GetBValue(textColor)));

	WCHAR szValue[32];
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->displayWindowWidth, szValue, SIZEOF_ARRAY(szValue), 10);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DisplayWindowWidth"),
		szValue);

	_itow_s(m_config->displayWindowHeight, szValue, SIZEOF_ARRAY(szValue), 10);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DisplayWindowHeight"),
		szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("DisplayWindowVertical"),
		NXMLSettings::EncodeBoolValue(m_config->displayWindowVertical));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DoubleClickTabClose"),
		NXMLSettings::EncodeBoolValue(m_config->doubleClickTabClose));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ExtendTabControl"),
		NXMLSettings::EncodeBoolValue(m_config->extendTabControl.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ForceSize"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.forceSize));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("HandleZipFiles"),
		NXMLSettings::EncodeBoolValue(m_config->handleZipFiles));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("HideLinkExtensionGlobal"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.hideLinkExtension));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("HideSystemFilesGlobal"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.hideSystemFiles));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("InfoTipType"),
		NXMLSettings::EncodeIntValue(m_config->infoTipType));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("InsertSorted"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.insertSorted));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->language, szValue, SIZEOF_ARRAY(szValue), 10);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("Language"), szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("LargeToolbarIcons"),
		NXMLSettings::EncodeBoolValue(m_config->useLargeToolbarIcons.get()));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_iLastSelectedTab, szValue, SIZEOF_ARRAY(szValue), 10);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("LastSelectedTab"),
		szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("LockToolbars"),
		NXMLSettings::EncodeBoolValue(m_config->lockToolbars));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("NextToCurrent"),
		NXMLSettings::EncodeBoolValue(m_config->openNewTabNextToCurrent));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("NewTabDirectory"),
		m_config->defaultTabDirectory.c_str());
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("OneClickActivate"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.oneClickActivate.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("OneClickActivateHoverTime"),
		NXMLSettings::EncodeIntValue(
			m_config->globalFolderSettings.oneClickActivateHoverTime.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("OverwriteExistingFilesConfirmation"),
		NXMLSettings::EncodeBoolValue(m_config->overwriteExistingFilesConfirmation));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ReplaceExplorerMode"),
		NXMLSettings::EncodeIntValue(m_config->replaceExplorerMode));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowAddressBar"),
		NXMLSettings::EncodeBoolValue(m_config->showAddressBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowApplicationToolbar"),
		NXMLSettings::EncodeBoolValue(m_config->showApplicationToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowBookmarksToolbar"),
		NXMLSettings::EncodeBoolValue(m_config->showBookmarksToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowDrivesToolbar"),
		NXMLSettings::EncodeBoolValue(m_config->showDrivesToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowDisplayWindow"),
		NXMLSettings::EncodeBoolValue(m_config->showDisplayWindow));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowExtensions"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showExtensions));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFilePreviews"),
		NXMLSettings::EncodeBoolValue(m_config->showFilePreviews));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFolders"),
		NXMLSettings::EncodeBoolValue(m_config->showFolders.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFolderSizes"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showFolderSizes));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFriendlyDates"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showFriendlyDates));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFullTitlePath"),
		NXMLSettings::EncodeBoolValue(m_config->showFullTitlePath.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowGridlinesGlobal"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showGridlines.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowHiddenGlobal"),
		NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.showHidden));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowInfoTips"),
		NXMLSettings::EncodeBoolValue(m_config->showInfoTips));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowInGroupsGlobal"),
		NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.showInGroups));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowPrivilegeLevelInTitleBar"),
		NXMLSettings::EncodeBoolValue(m_config->showPrivilegeLevelInTitleBar.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowStatusBar"),
		NXMLSettings::EncodeBoolValue(m_config->showStatusBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowTabBarAtBottom"),
		NXMLSettings::EncodeBoolValue(m_config->showTabBarAtBottom.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowTaskbarThumbnails"),
		NXMLSettings::EncodeBoolValue(m_config->showTaskbarThumbnails));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowToolbar"),
		NXMLSettings::EncodeBoolValue(m_config->showMainToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowUserNameTitleBar"),
		NXMLSettings::EncodeBoolValue(m_config->showUserNameInTitleBar.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("SizeDisplayFormat"),
		NXMLSettings::EncodeIntValue(m_config->globalFolderSettings.sizeDisplayFormat));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("SortAscendingGlobal"),
		NXMLSettings::EncodeBoolValue(
			m_config->defaultFolderSettings.sortDirection == +SortDirection::Ascending));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("StartupMode"),
		NXMLSettings::EncodeIntValue(m_config->startupMode));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("SynchronizeTreeview"),
		NXMLSettings::EncodeBoolValue(m_config->synchronizeTreeview.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("TVAutoExpandSelected"),
		NXMLSettings::EncodeBoolValue(m_config->treeViewAutoExpandSelected));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("UseFullRowSelect"),
		NXMLSettings::EncodeBoolValue(m_config->useFullRowSelect.get()));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("IconTheme"),
		NXMLSettings::EncodeIntValue(m_config->iconSet));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("ToolbarState"));
	MainToolbarStorage::SaveToXml(pXMLDom, pParentNode.get(),
		m_mainToolbar->GetButtonsForStorage());

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("TreeViewDelayEnabled"),
		NXMLSettings::EncodeBoolValue(m_config->treeViewDelayEnabled));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->treeViewWidth, szValue, SIZEOF_ARRAY(szValue), 10);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("TreeViewWidth"),
		szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->defaultFolderSettings.viewMode, szValue, SIZEOF_ARRAY(szValue), 10);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ViewModeGlobal"),
		szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("CheckPinnedToNamespaceTreeProperty"),
		NXMLSettings::EncodeBoolValue(m_config->checkPinnedToNamespaceTreeProperty));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowQuickAccessInTreeView"),
		NXMLSettings::EncodeBoolValue(m_config->showQuickAccessInTreeView.get()));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("Theme"),
		NXMLSettings::EncodeIntValue(m_config->theme.get()));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("DisplayMixedFilesAndFolders"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.displayMixedFilesAndFolders));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("UseNaturalSortOrder"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.useNaturalSortOrder));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("OpenTabsInForeground"),
		NXMLSettings::EncodeBoolValue(m_config->openTabsInForeground));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("GroupSortDirectionGlobal"),
		NXMLSettings::EncodeIntValue(m_config->defaultFolderSettings.groupSortDirection));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	NXMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("GoUpOnDoubleClick"),
		NXMLSettings::EncodeBoolValue(m_config->goUpOnDoubleClick));

	auto &mainFont = m_config->mainFont.get();

	if (mainFont)
	{
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
		NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
			_T("MainFont"));
		SaveCustomFontToXml(pXMLDom, pParentNode.get(), *mainFont);
	}

	auto bstr_wsnt = wil::make_bstr_nothrow(L"\n\t");
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pe.get());

	NXMLSettings::AppendChildToParent(pe.get(), pRoot);

	SaveWindowPositionToXML(pXMLDom, pRoot);
}

void Explorerplusplus::LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom)
{
	if (!pXMLDom)
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> tabsNode;
	auto bstr = wil::make_bstr_nothrow(L"//Tabs");
	pXMLDom->selectSingleNode(bstr.get(), &tabsNode);

	if (!tabsNode)
	{
		return;
	}

	m_loadedTabs = TabXmlStorage::Load(tabsNode.get());
}

void Explorerplusplus::SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot)
{
	wil::com_ptr_nothrow<IXMLDOMElement> tabsNode;
	auto bstr = wil::make_bstr_nothrow(L"Tabs");
	HRESULT hr = pXMLDom->createElement(bstr.get(), &tabsNode);

	if (FAILED(hr))
	{
		return;
	}

	TabXmlStorage::Save(pXMLDom, tabsNode.get(), GetTabListStorageData());

	NXMLSettings::AppendChildToParent(tabsNode.get(), pRoot);
}

void Explorerplusplus::LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom)
{
	if (!pXMLDom)
	{
		return;
	}

	wil::com_ptr_nothrow<IXMLDOMNode> defaultColumnsNode;
	auto bstr = wil::make_bstr_nothrow(L"//DefaultColumns");
	pXMLDom->selectSingleNode(bstr.get(), &defaultColumnsNode);

	if (!defaultColumnsNode)
	{
		return;
	}

	auto &folderColumns = m_config->globalFolderSettings.folderColumns;
	ColumnXmlStorage::LoadAllColumnSets(defaultColumnsNode.get(), folderColumns);
}

void Explorerplusplus::SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot)
{
	wil::com_ptr_nothrow<IXMLDOMElement> defaultColumnsNode;
	auto bstr = wil::make_bstr_nothrow(L"DefaultColumns");
	HRESULT hr = pXMLDom->createElement(bstr.get(), &defaultColumnsNode);

	if (FAILED(hr))
	{
		return;
	}

	const auto &folderColumns = m_config->globalFolderSettings.folderColumns;
	ColumnXmlStorage::SaveAllColumnSets(pXMLDom, defaultColumnsNode.get(), folderColumns);

	NXMLSettings::AppendChildToParent(defaultColumnsNode.get(), pRoot);
}

void Explorerplusplus::SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pRoot)
{
	wil::com_ptr_nothrow<IXMLDOMElement> pWndPosNode;
	auto bstr = wil::make_bstr_nothrow(L"WindowPosition");
	pXMLDom->createElement(bstr.get(), &pWndPosNode);

	SaveWindowPositionToXMLInternal(pXMLDom, pWndPosNode.get());

	auto bstr_wsnt = wil::make_bstr_nothrow(L"\n\t");
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pWndPosNode.get());
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pRoot);

	NXMLSettings::AppendChildToParent(pWndPosNode.get(), pRoot);
}

void Explorerplusplus::SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pWndPosNode)
{
	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(m_hContainer, &wndpl);

	auto bstr_wsntt = wil::make_bstr_nothrow(L"\n\t\t");
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pWndPosNode);

	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pWndPosNode, _T("Setting"),
		_T("Position"));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Flags"),
		NXMLSettings::EncodeIntValue(wndpl.flags));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("ShowCmd"),
		NXMLSettings::EncodeIntValue(wndpl.showCmd));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("MinPositionX"),
		NXMLSettings::EncodeIntValue(wndpl.ptMinPosition.x));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("MinPositionY"),
		NXMLSettings::EncodeIntValue(wndpl.ptMinPosition.y));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("MaxPositionX"),
		NXMLSettings::EncodeIntValue(wndpl.ptMaxPosition.x));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("MaxPositionY"),
		NXMLSettings::EncodeIntValue(wndpl.ptMaxPosition.y));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("NormalPositionLeft"),
		NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.left));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("NormalPositionTop"),
		NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.top));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("NormalPositionRight"),
		NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.right));
	NXMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("NormalPositionBottom"),
		NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.bottom));
}

void Explorerplusplus::LoadMainRebarInformationFromXML(IXMLDOMDocument *pXMLDom)
{
	m_loadedRebarStorageInfo = MainRebarXmlStorage::Load(pXMLDom);
}

void Explorerplusplus::SaveMainRebarInformationToXML(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pRoot)
{
	MainRebarXmlStorage::Save(pXMLDom, pRoot, GetMainRebarStorageInfo());
}

unsigned long hash_setting(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++) != '\0')
	{
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

/* Maps attribute name to their corresponding internal variable. */
void Explorerplusplus::MapAttributeToValue(IXMLDOMNode *pNode, WCHAR *wszName, WCHAR *wszValue)
{
	unsigned char szName[512];
	unsigned long uNameHash;

	WideCharToMultiByte(CP_ACP, 0, wszName, -1, (LPSTR) szName, SIZEOF_ARRAY(szName), nullptr,
		nullptr);

	uNameHash = hash_setting(szName);

	switch (uNameHash)
	{
	case HASH_ALLOWMULTIPLEINSTANCES:
		m_config->allowMultipleInstances = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ALWAYSOPENINNEWTAB:
		m_config->alwaysOpenNewTab = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ALWAYSSHOWTABBAR:
		m_config->alwaysShowTabBar.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_AUTOARRANGEGLOBAL:
		m_config->defaultFolderSettings.autoArrange = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CHECKBOXSELECTION:
		m_config->checkBoxSelection = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CLOSEMAINWINDOWONTABCLOSE:
		m_config->closeMainWindowOnTabClose = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CONFIRMCLOSETABS:
		m_config->confirmCloseTabs = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DISABLEFOLDERSIZENETWORKREMOVABLE:
		m_config->globalFolderSettings.disableFolderSizesNetworkRemovable =
			NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DISPLAYCENTRECOLOR:
		m_config->displayWindowCentreColor = NXMLSettings::ReadXMLColorData2(pNode);
		break;

	case HASH_DISPLAYFONT:
		m_config->displayWindowFont = NXMLSettings::ReadXMLFontData(pNode);
		break;

	case HASH_DISPLAYSURROUNDCOLOR:
		m_config->displayWindowSurroundColor = NXMLSettings::ReadXMLColorData2(pNode);
		break;

	case HASH_DISPLAYTEXTCOLOR:
		m_config->displayWindowTextColor = NXMLSettings::ReadXMLColorData(pNode);
		break;

	case HASH_DISPLAYWINDOWWIDTH:
		m_config->displayWindowWidth = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_DISPLAYWINDOWHEIGHT:
		m_config->displayWindowHeight = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_DISPLAYWINDOWVERTICAL:
		m_config->displayWindowVertical = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DOUBLECLICKTABCLOSE:
		m_config->doubleClickTabClose = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_EXTENDTABCONTROL:
		m_config->extendTabControl.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_FORCESIZE:
		m_config->globalFolderSettings.forceSize = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HANDLEZIPFILES:
		m_config->handleZipFiles = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HIDELINKEXTENSIONGLOBAL:
		m_config->globalFolderSettings.hideLinkExtension = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HIDESYSTEMFILESGLOBAL:
		m_config->globalFolderSettings.hideSystemFiles = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_INSERTSORTED:
		m_config->globalFolderSettings.insertSorted = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_LANGUAGE:
		m_config->language = NXMLSettings::DecodeIntValue(wszValue);
		m_bLanguageLoaded = true;
		break;

	case HASH_LARGETOOLBARICONS:
		m_config->useLargeToolbarIcons.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_LASTSELECTEDTAB:
		m_iLastSelectedTab = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_LOCKTOOLBARS:
		m_config->lockToolbars = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_NEXTTOCURRENT:
		m_config->openNewTabNextToCurrent = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ONECLICKACTIVATE:
		m_config->globalFolderSettings.oneClickActivate = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ONECLICKACTIVATEHOVERTIME:
		m_config->globalFolderSettings.oneClickActivateHoverTime =
			NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_OVERWRITEEXISTINGFILESCONFIRMATION:
		m_config->overwriteExistingFilesConfirmation = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_REPLACEEXPLORERMODE:
		m_config->replaceExplorerMode = DefaultFileManager::ReplaceExplorerMode::_from_integral(
			NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SHOWADDRESSBAR:
		m_config->showAddressBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWAPPLICATIONTOOLBAR:
		m_config->showApplicationToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWBOOKMARKSTOOLBAR:
		m_config->showBookmarksToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWDRIVESTOOLBAR:
		m_config->showDrivesToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWDISPLAYWINDOW:
		m_config->showDisplayWindow = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWEXTENSIONS:
		m_config->globalFolderSettings.showExtensions = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFILEPREVIEWS:
		m_config->showFilePreviews = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFOLDERS:
		m_config->showFolders = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFOLDERSIZES:
		m_config->globalFolderSettings.showFolderSizes = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFRIENDLYDATES:
		m_config->globalFolderSettings.showFriendlyDates = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFULLTITLEPATH:
		m_config->showFullTitlePath.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SHOWGRIDLINESGLOBAL:
		m_config->globalFolderSettings.showGridlines = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWHIDDENGLOBAL:
		m_config->defaultFolderSettings.showHidden = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWINFOTIPS:
		m_config->showInfoTips = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWINGROUPSGLOBAL:
		m_config->defaultFolderSettings.showInGroups = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWPRIVILEGETITLEBAR:
		m_config->showPrivilegeLevelInTitleBar.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SHOWSTATUSBAR:
		m_config->showStatusBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTABBARATBOTTOM:
		m_config->showTabBarAtBottom.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SHOWTASKBARTHUMBNAILS:
		m_config->showTaskbarThumbnails = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTOOLBAR:
		m_config->showMainToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWUSERNAMETITLEBAR:
		m_config->showUserNameInTitleBar.set(NXMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SIZEDISPLAYFOMRAT:
		m_config->globalFolderSettings.sizeDisplayFormat =
			SizeDisplayFormat::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SORTASCENDINGGLOBAL:
		m_config->defaultFolderSettings.sortDirection = NXMLSettings::DecodeBoolValue(wszValue)
			? SortDirection::Ascending
			: SortDirection::Descending;

		if (!m_groupSortDirectionGlobalLoadedFromXml)
		{
			m_config->defaultFolderSettings.groupSortDirection =
				NXMLSettings::DecodeBoolValue(wszValue) ? SortDirection::Ascending
														: SortDirection::Descending;
		}
		break;

	case HASH_STARTUPMODE:
		m_config->startupMode = StartupMode::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SYNCHRONIZETREEVIEW:
		m_config->synchronizeTreeview = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TVAUTOEXPAND:
		m_config->treeViewAutoExpandSelected = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_USEFULLROWSELECT:
		m_config->useFullRowSelect = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TOOLBARSTATE:
		m_loadedMainToolbarButtons = MainToolbarStorage::LoadFromXml(pNode);
		break;

	case HASH_TREEVIEWDELAYENABLED:
		m_config->treeViewDelayEnabled = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TREEVIEWWIDTH:
		m_config->treeViewWidth = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_VIEWMODEGLOBAL:
		m_config->defaultFolderSettings.viewMode =
			ViewMode::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_POSITION:
	{
		wil::com_ptr_nothrow<IXMLDOMNamedNodeMap> am;
		pNode->get_attributes(&am);

		WINDOWPLACEMENT wndpl;
		BOOL bMaximized = FALSE;

		long lChildNodes;
		am->get_length(&lChildNodes);

		for (long j = 1; j < lChildNodes; j++)
		{
			wil::com_ptr_nothrow<IXMLDOMNode> pChildNode;
			am->get_item(j, &pChildNode);

			wil::unique_bstr bstrName;
			pChildNode->get_nodeName(&bstrName);

			wil::unique_bstr bstrValue;
			pChildNode->get_text(&bstrValue);

			if (lstrcmp(bstrName.get(), L"Left") == 0)
			{
				wndpl.rcNormalPosition.left = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), L"Top") == 0)
			{
				wndpl.rcNormalPosition.top = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), L"Right") == 0)
			{
				wndpl.rcNormalPosition.right = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), L"Bottom") == 0)
			{
				wndpl.rcNormalPosition.bottom = NXMLSettings::DecodeIntValue(bstrValue.get());
			}
			else if (lstrcmp(bstrName.get(), L"Maximized") == 0)
			{
				bMaximized = NXMLSettings::DecodeBoolValue(bstrValue.get());
			}
		}

		wndpl.length = sizeof(WINDOWPLACEMENT);
		wndpl.showCmd = SW_HIDE;

		if (bMaximized)
		{
			wndpl.showCmd |= SW_MAXIMIZE;
		}

		SetWindowPlacement(m_hContainer, &wndpl);
	}
	break;

	case HASH_NEWTABDIRECTORY:
		m_config->defaultTabDirectory = wszValue;
		break;

	case HASH_INFOTIPTYPE:
		m_config->infoTipType = InfoTipType::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_ICON_THEME:
		m_config->iconSet = IconSet::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_CHECK_PINNED_TO_NAMESPACE_TREE_PROPERTY:
		m_config->checkPinnedToNamespaceTreeProperty = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOW_QUICK_ACCESS_IN_TREEVIEW:
		m_config->showQuickAccessInTreeView = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_THEME:
		m_config->theme = Theme::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		m_themeValueLoadedFromXml = true;
		break;

	case HASH_ENABLE_DARK_MODE:
		if (!m_themeValueLoadedFromXml)
		{
			bool enableDarkMode = NXMLSettings::DecodeBoolValue(wszValue);
			m_config->theme = enableDarkMode ? Theme::Dark : Theme::Light;
		}
		break;

	case HASH_DISPLAY_MIXED_FILES_AND_FOLDERS:
		m_config->globalFolderSettings.displayMixedFilesAndFolders =
			NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_USE_NATURAL_SORT_ORDER:
		m_config->globalFolderSettings.useNaturalSortOrder =
			NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_OPEN_TABS_IN_FOREGROUND:
		m_config->openTabsInForeground = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_GROUP_SORT_DIRECTION_GLOBAL:
		m_config->defaultFolderSettings.groupSortDirection =
			SortDirection::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		m_groupSortDirectionGlobalLoadedFromXml = true;
		break;

	case HASH_GO_UP_ON_DOUBLE_CLICK:
		m_config->goUpOnDoubleClick = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_MAIN_FONT:
	{
		auto mainFont = LoadCustomFontFromXml(pNode);

		if (mainFont)
		{
			m_config->mainFont = *mainFont;
		}
	}
	break;
	}
}

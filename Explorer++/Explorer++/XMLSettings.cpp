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
#include "Config.h"
#include "CustomFontStorage.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Storage.h"
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
#define HASH_TREEVIEWDELAYENABLED 2186637066
#define HASH_TREEVIEWWIDTH 4257779536
#define HASH_VIEWMODEGLOBAL 3743629718
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

unsigned long hash_setting(unsigned char *str);

BOOL LoadAllowMultipleInstancesFromXML()
{
	BOOL bAllowMultipleInstances = TRUE;

	auto pXMLDom = XMLSettings::CreateXmlDocument();

	if (!pXMLDom)
	{
		return bAllowMultipleInstances;
	}

	TCHAR szConfigFile[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szConfigFile, SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, Storage::CONFIG_FILE_FILENAME);

	VARIANT_BOOL status;
	wil::unique_variant var(XMLSettings::VariantString(szConfigFile));
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
					bAllowMultipleInstances = XMLSettings::DecodeBoolValue(bstrValue.get());
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

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("AllowMultipleInstances"),
		XMLSettings::EncodeBoolValue(m_config->allowMultipleInstances));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("AlwaysOpenInNewTab"),
		XMLSettings::EncodeBoolValue(m_config->alwaysOpenNewTab));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("AlwaysShowTabBar"),
		XMLSettings::EncodeBoolValue(m_config->alwaysShowTabBar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("AutoArrangeGlobal"),
		XMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.autoArrange));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("CheckBoxSelection"),
		XMLSettings::EncodeBoolValue(m_config->checkBoxSelection.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("CloseMainWindowOnTabClose"),
		XMLSettings::EncodeBoolValue(m_config->closeMainWindowOnTabClose));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ConfirmCloseTabs"),
		XMLSettings::EncodeBoolValue(m_config->confirmCloseTabs));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("DisableFolderSizesNetworkRemovable"),
		XMLSettings::EncodeBoolValue(
			m_config->globalFolderSettings.disableFolderSizesNetworkRemovable));

	COLORREF centreColor;

	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayCentreColor"));
	centreColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETCENTRECOLOR, 0, 0);
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		XMLSettings::EncodeIntValue(GetRValue(centreColor)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		XMLSettings::EncodeIntValue(GetGValue(centreColor)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		XMLSettings::EncodeIntValue(GetBValue(centreColor)));

	HFONT hFont;
	LOGFONT fontInfo;

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayFont"));
	SendMessage(m_hDisplayWindow, DWM_GETFONT, (WPARAM) &hFont, 0);
	GetObject(hFont, sizeof(LOGFONT), &fontInfo);
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Height"),
		XMLSettings::EncodeIntValue(fontInfo.lfHeight));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Width"),
		XMLSettings::EncodeIntValue(fontInfo.lfWidth));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Weight"),
		XMLSettings::EncodeIntValue(fontInfo.lfWeight));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Italic"),
		XMLSettings::EncodeBoolValue(fontInfo.lfItalic));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Underline"),
		XMLSettings::EncodeBoolValue(fontInfo.lfUnderline));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Strikeout"),
		XMLSettings::EncodeBoolValue(fontInfo.lfStrikeOut));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Font"), fontInfo.lfFaceName);

	COLORREF surroundColor;

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplaySurroundColor"));
	surroundColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETSURROUNDCOLOR, 0, 0);
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		XMLSettings::EncodeIntValue(GetRValue(surroundColor)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		XMLSettings::EncodeIntValue(GetGValue(surroundColor)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		XMLSettings::EncodeIntValue(GetBValue(surroundColor)));

	COLORREF textColor;

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayTextColor"));
	textColor = (COLORREF) SendMessage(m_hDisplayWindow, DWM_GETTEXTCOLOR, 0, 0);
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		XMLSettings::EncodeIntValue(GetRValue(textColor)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		XMLSettings::EncodeIntValue(GetGValue(textColor)));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		XMLSettings::EncodeIntValue(GetBValue(textColor)));

	WCHAR szValue[32];
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->displayWindowWidth, szValue, SIZEOF_ARRAY(szValue), 10);
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DisplayWindowWidth"),
		szValue);

	_itow_s(m_config->displayWindowHeight, szValue, SIZEOF_ARRAY(szValue), 10);
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DisplayWindowHeight"),
		szValue);

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DisplayWindowVertical"),
		XMLSettings::EncodeBoolValue(m_config->displayWindowVertical));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("DoubleClickTabClose"),
		XMLSettings::EncodeBoolValue(m_config->doubleClickTabClose));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ExtendTabControl"),
		XMLSettings::EncodeBoolValue(m_config->extendTabControl.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ForceSize"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.forceSize));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("HandleZipFiles"),
		XMLSettings::EncodeBoolValue(m_config->handleZipFiles));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("HideLinkExtensionGlobal"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.hideLinkExtension));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("HideSystemFilesGlobal"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.hideSystemFiles));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("InfoTipType"),
		XMLSettings::EncodeIntValue(m_config->infoTipType));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("InsertSorted"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.insertSorted));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->language, szValue, SIZEOF_ARRAY(szValue), 10);
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("Language"), szValue);

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("LargeToolbarIcons"),
		XMLSettings::EncodeBoolValue(m_config->useLargeToolbarIcons.get()));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("LockToolbars"),
		XMLSettings::EncodeBoolValue(m_config->lockToolbars.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("NextToCurrent"),
		XMLSettings::EncodeBoolValue(m_config->openNewTabNextToCurrent));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("NewTabDirectory"),
		m_config->defaultTabDirectory.c_str());
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("OneClickActivate"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.oneClickActivate.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("OneClickActivateHoverTime"),
		XMLSettings::EncodeIntValue(
			m_config->globalFolderSettings.oneClickActivateHoverTime.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("OverwriteExistingFilesConfirmation"),
		XMLSettings::EncodeBoolValue(m_config->overwriteExistingFilesConfirmation));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ReplaceExplorerMode"),
		XMLSettings::EncodeIntValue(m_config->replaceExplorerMode));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowAddressBar"),
		XMLSettings::EncodeBoolValue(m_config->showAddressBar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowApplicationToolbar"),
		XMLSettings::EncodeBoolValue(m_config->showApplicationToolbar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowBookmarksToolbar"),
		XMLSettings::EncodeBoolValue(m_config->showBookmarksToolbar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowDrivesToolbar"),
		XMLSettings::EncodeBoolValue(m_config->showDrivesToolbar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowDisplayWindow"),
		XMLSettings::EncodeBoolValue(m_config->showDisplayWindow));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowExtensions"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showExtensions));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFilePreviews"),
		XMLSettings::EncodeBoolValue(m_config->showFilePreviews));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFolders"),
		XMLSettings::EncodeBoolValue(m_config->showFolders.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFolderSizes"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showFolderSizes));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFriendlyDates"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showFriendlyDates));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowFullTitlePath"),
		XMLSettings::EncodeBoolValue(m_config->showFullTitlePath.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowGridlinesGlobal"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showGridlines.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowHiddenGlobal"),
		XMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.showHidden));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowInfoTips"),
		XMLSettings::EncodeBoolValue(m_config->showInfoTips));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowInGroupsGlobal"),
		XMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.showInGroups));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowPrivilegeLevelInTitleBar"),
		XMLSettings::EncodeBoolValue(m_config->showPrivilegeLevelInTitleBar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowStatusBar"),
		XMLSettings::EncodeBoolValue(m_config->showStatusBar));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowTabBarAtBottom"),
		XMLSettings::EncodeBoolValue(m_config->showTabBarAtBottom.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowTaskbarThumbnails"),
		XMLSettings::EncodeBoolValue(m_config->showTaskbarThumbnails));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowToolbar"),
		XMLSettings::EncodeBoolValue(m_config->showMainToolbar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ShowUserNameTitleBar"),
		XMLSettings::EncodeBoolValue(m_config->showUserNameInTitleBar.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("SizeDisplayFormat"),
		XMLSettings::EncodeIntValue(m_config->globalFolderSettings.sizeDisplayFormat));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("SortAscendingGlobal"),
		XMLSettings::EncodeBoolValue(
			m_config->defaultFolderSettings.sortDirection == +SortDirection::Ascending));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("StartupMode"),
		XMLSettings::EncodeIntValue(m_config->startupMode));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("SynchronizeTreeview"),
		XMLSettings::EncodeBoolValue(m_config->synchronizeTreeview.get()));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("TVAutoExpandSelected"),
		XMLSettings::EncodeBoolValue(m_config->treeViewAutoExpandSelected));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("UseFullRowSelect"),
		XMLSettings::EncodeBoolValue(m_config->useFullRowSelect.get()));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("IconTheme"),
		XMLSettings::EncodeIntValue(m_config->iconSet));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("TreeViewDelayEnabled"),
		XMLSettings::EncodeBoolValue(m_config->treeViewDelayEnabled));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->treeViewWidth, szValue, SIZEOF_ARRAY(szValue), 10);
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("TreeViewWidth"),
		szValue);

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	_itow_s(m_config->defaultFolderSettings.viewMode, szValue, SIZEOF_ARRAY(szValue), 10);
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("ViewModeGlobal"),
		szValue);

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("CheckPinnedToNamespaceTreeProperty"),
		XMLSettings::EncodeBoolValue(m_config->checkPinnedToNamespaceTreeProperty));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("ShowQuickAccessInTreeView"),
		XMLSettings::EncodeBoolValue(m_config->showQuickAccessInTreeView.get()));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("Theme"),
		XMLSettings::EncodeIntValue(m_config->theme.get()));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("DisplayMixedFilesAndFolders"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.displayMixedFilesAndFolders));
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("UseNaturalSortOrder"),
		XMLSettings::EncodeBoolValue(m_config->globalFolderSettings.useNaturalSortOrder));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("OpenTabsInForeground"),
		XMLSettings::EncodeBoolValue(m_config->openTabsInForeground));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"),
		_T("GroupSortDirectionGlobal"),
		XMLSettings::EncodeIntValue(m_config->defaultFolderSettings.groupSortDirection));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::WriteStandardSetting(pXMLDom, pe.get(), _T("Setting"), _T("GoUpOnDoubleClick"),
		XMLSettings::EncodeBoolValue(m_config->goUpOnDoubleClick));

	auto &mainFont = m_config->mainFont.get();

	if (mainFont)
	{
		XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
		XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
			_T("MainFont"));
		SaveCustomFontToXml(pXMLDom, pParentNode.get(), *mainFont);
	}

	auto bstr_wsnt = wil::make_bstr_nothrow(L"\n\t");
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pe.get());

	XMLSettings::AppendChildToParent(pe.get(), pRoot);
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
		m_config->allowMultipleInstances = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ALWAYSOPENINNEWTAB:
		m_config->alwaysOpenNewTab = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ALWAYSSHOWTABBAR:
		m_config->alwaysShowTabBar.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_AUTOARRANGEGLOBAL:
		m_config->defaultFolderSettings.autoArrange = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CHECKBOXSELECTION:
		m_config->checkBoxSelection = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CLOSEMAINWINDOWONTABCLOSE:
		m_config->closeMainWindowOnTabClose = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CONFIRMCLOSETABS:
		m_config->confirmCloseTabs = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DISABLEFOLDERSIZENETWORKREMOVABLE:
		m_config->globalFolderSettings.disableFolderSizesNetworkRemovable =
			XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DISPLAYCENTRECOLOR:
		m_config->displayWindowCentreColor = XMLSettings::ReadXMLColorData2(pNode);
		break;

	case HASH_DISPLAYFONT:
		m_config->displayWindowFont = XMLSettings::ReadXMLFontData(pNode);
		break;

	case HASH_DISPLAYSURROUNDCOLOR:
		m_config->displayWindowSurroundColor = XMLSettings::ReadXMLColorData2(pNode);
		break;

	case HASH_DISPLAYTEXTCOLOR:
		m_config->displayWindowTextColor = XMLSettings::ReadXMLColorData(pNode);
		break;

	case HASH_DISPLAYWINDOWWIDTH:
		m_config->displayWindowWidth = XMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_DISPLAYWINDOWHEIGHT:
		m_config->displayWindowHeight = XMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_DISPLAYWINDOWVERTICAL:
		m_config->displayWindowVertical = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DOUBLECLICKTABCLOSE:
		m_config->doubleClickTabClose = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_EXTENDTABCONTROL:
		m_config->extendTabControl.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_FORCESIZE:
		m_config->globalFolderSettings.forceSize = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HANDLEZIPFILES:
		m_config->handleZipFiles = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HIDELINKEXTENSIONGLOBAL:
		m_config->globalFolderSettings.hideLinkExtension = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HIDESYSTEMFILESGLOBAL:
		m_config->globalFolderSettings.hideSystemFiles = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_INSERTSORTED:
		m_config->globalFolderSettings.insertSorted = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_LANGUAGE:
		m_config->language = XMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_LARGETOOLBARICONS:
		m_config->useLargeToolbarIcons.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_LOCKTOOLBARS:
		m_config->lockToolbars = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_NEXTTOCURRENT:
		m_config->openNewTabNextToCurrent = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ONECLICKACTIVATE:
		m_config->globalFolderSettings.oneClickActivate = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ONECLICKACTIVATEHOVERTIME:
		m_config->globalFolderSettings.oneClickActivateHoverTime =
			XMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_OVERWRITEEXISTINGFILESCONFIRMATION:
		m_config->overwriteExistingFilesConfirmation = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_REPLACEEXPLORERMODE:
		m_config->replaceExplorerMode = DefaultFileManager::ReplaceExplorerMode::_from_integral(
			XMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SHOWADDRESSBAR:
		m_config->showAddressBar = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWAPPLICATIONTOOLBAR:
		m_config->showApplicationToolbar = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWBOOKMARKSTOOLBAR:
		m_config->showBookmarksToolbar = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWDRIVESTOOLBAR:
		m_config->showDrivesToolbar = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWDISPLAYWINDOW:
		m_config->showDisplayWindow = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWEXTENSIONS:
		m_config->globalFolderSettings.showExtensions = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFILEPREVIEWS:
		m_config->showFilePreviews = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFOLDERS:
		m_config->showFolders = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFOLDERSIZES:
		m_config->globalFolderSettings.showFolderSizes = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFRIENDLYDATES:
		m_config->globalFolderSettings.showFriendlyDates = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFULLTITLEPATH:
		m_config->showFullTitlePath.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SHOWGRIDLINESGLOBAL:
		m_config->globalFolderSettings.showGridlines = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWHIDDENGLOBAL:
		m_config->defaultFolderSettings.showHidden = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWINFOTIPS:
		m_config->showInfoTips = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWINGROUPSGLOBAL:
		m_config->defaultFolderSettings.showInGroups = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWPRIVILEGETITLEBAR:
		m_config->showPrivilegeLevelInTitleBar.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SHOWSTATUSBAR:
		m_config->showStatusBar = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTABBARATBOTTOM:
		m_config->showTabBarAtBottom.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SHOWTASKBARTHUMBNAILS:
		m_config->showTaskbarThumbnails = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTOOLBAR:
		m_config->showMainToolbar = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWUSERNAMETITLEBAR:
		m_config->showUserNameInTitleBar.set(XMLSettings::DecodeBoolValue(wszValue));
		break;

	case HASH_SIZEDISPLAYFOMRAT:
		m_config->globalFolderSettings.sizeDisplayFormat =
			SizeDisplayFormat::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SORTASCENDINGGLOBAL:
		m_config->defaultFolderSettings.sortDirection = XMLSettings::DecodeBoolValue(wszValue)
			? SortDirection::Ascending
			: SortDirection::Descending;

		if (!m_groupSortDirectionGlobalLoadedFromXml)
		{
			m_config->defaultFolderSettings.groupSortDirection =
				XMLSettings::DecodeBoolValue(wszValue) ? SortDirection::Ascending
													   : SortDirection::Descending;
		}
		break;

	case HASH_STARTUPMODE:
		m_config->startupMode = StartupMode::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SYNCHRONIZETREEVIEW:
		m_config->synchronizeTreeview = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TVAUTOEXPAND:
		m_config->treeViewAutoExpandSelected = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_USEFULLROWSELECT:
		m_config->useFullRowSelect = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TREEVIEWDELAYENABLED:
		m_config->treeViewDelayEnabled = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TREEVIEWWIDTH:
		m_config->treeViewWidth = XMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_VIEWMODEGLOBAL:
		m_config->defaultFolderSettings.viewMode =
			ViewMode::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_NEWTABDIRECTORY:
		m_config->defaultTabDirectory = wszValue;
		break;

	case HASH_INFOTIPTYPE:
		m_config->infoTipType = InfoTipType::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_ICON_THEME:
		m_config->iconSet = IconSet::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_CHECK_PINNED_TO_NAMESPACE_TREE_PROPERTY:
		m_config->checkPinnedToNamespaceTreeProperty = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOW_QUICK_ACCESS_IN_TREEVIEW:
		m_config->showQuickAccessInTreeView = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_THEME:
		m_config->theme = Theme::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		m_themeValueLoadedFromXml = true;
		break;

	case HASH_ENABLE_DARK_MODE:
		if (!m_themeValueLoadedFromXml)
		{
			bool enableDarkMode = XMLSettings::DecodeBoolValue(wszValue);
			m_config->theme = enableDarkMode ? Theme::Dark : Theme::Light;
		}
		break;

	case HASH_DISPLAY_MIXED_FILES_AND_FOLDERS:
		m_config->globalFolderSettings.displayMixedFilesAndFolders =
			XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_USE_NATURAL_SORT_ORDER:
		m_config->globalFolderSettings.useNaturalSortOrder = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_OPEN_TABS_IN_FOREGROUND:
		m_config->openTabsInForeground = XMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_GROUP_SORT_DIRECTION_GLOBAL:
		m_config->defaultFolderSettings.groupSortDirection =
			SortDirection::_from_integral(XMLSettings::DecodeIntValue(wszValue));
		m_groupSortDirectionGlobalLoadedFromXml = true;
		break;

	case HASH_GO_UP_ON_DOUBLE_CLICK:
		m_config->goUpOnDoubleClick = XMLSettings::DecodeBoolValue(wszValue);
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

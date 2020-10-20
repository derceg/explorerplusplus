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
#include "ApplicationToolbar.h"
#include "Bookmarks/BookmarkXmlStorage.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "MainToolbar.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/XMLSettings.h"
#include <boost/range/adaptor/map.hpp>

#define COLUMN_TYPE_GENERIC			0
#define COLUMN_TYPE_MYCOMPUTER		1
#define COLUMN_TYPE_CONTROLPANEL	2
#define COLUMN_TYPE_RECYCLEBIN		3
#define COLUMN_TYPE_PRINTERS		4
#define COLUMN_TYPE_NETWORK			5
#define COLUMN_TYPE_NETWORKPLACES	6

/* These represent the pre-hashed values of attribute
names. They are used to avoid string comparisons
on each attribute. If the hash function or any of
the attribute names change in any way, these values
will need to be changed correspondingly. */
#define HASH_ALWAYSOPENINNEWTAB		1123321600
#define HASH_AUTOARRANGEGLOBAL		151507311
#define HASH_CONFIRMCLOSETABS		2636757395
#define HASH_DISPLAYCENTRECOLOR		3404143227
#define HASH_DISPLAYFONT			362757714
#define HASH_DISPLAYSURROUNDCOLOR	1807564604
#define HASH_DISPLAYTEXTCOLOR		4212809823
#define HASH_DISPLAYWINDOWWIDTH		3332824435
#define HASH_DISPLAYWINDOWHEIGHT	2017415020
#define HASH_DISPLAYWINDOWVERTICAL	2262072301
#define HASH_LANGUAGE				3526403497
#define HASH_LASTSELECTEDTAB		1712438393
#define HASH_NEXTTOCURRENT			743165450
#define HASH_SHOWADDRESSBAR			3302864385
#define HASH_SHOWBOOKMARKSTOOLBAR	1216493954
#define HASH_SHOWDRIVESTOOLBAR		899091590
#define HASH_SHOWDISPLAYWINDOW		351410676
#define HASH_SHOWEXTENSIONS			3743594966
#define HASH_SHOWFOLDERS			948345109
#define HASH_SHOWFOLDERSIZES		3684676528
#define HASH_SHOWFRIENDLYDATES		467626964
#define HASH_SHOWFULLTITLEPATH		1871292168
#define HASH_SHOWGRIDLINESGLOBAL	1707929656
#define HASH_SHOWHIDDENGLOBAL		558199811
#define HASH_SHOWSTATUSBAR			3554629247
#define HASH_SHOWINFOTIPS			3018038962
#define HASH_SHOWTOOLBAR			1852868921
#define HASH_SORTASCENDINGGLOBAL	2605638058
#define HASH_STARTUPMODE			1344265373
#define HASH_TOOLBARSTATE			3436473849
#define HASH_TREEVIEWDELAYENABLED	2186637066
#define HASH_TREEVIEWWIDTH			4257779536
#define HASH_VIEWMODEGLOBAL			3743629718
#define HASH_POSITION				3300187802
#define HASH_LOCKTOOLBARS			3842965076
#define HASH_NEWTABDIRECTORY		3570078203
#define HASH_INFOTIPTYPE			3366492864
#define HASH_SHOWAPPLICATIONTOOLBAR	101571053
#define HASH_USEFULLROWSELECT		3780943197
#define HASH_SHOWINGROUPSGLOBAL		4239388334
#define HASH_EXTENDTABCONTROL		4097866437
#define HASH_SHOWFILEPREVIEWS		1834921243
#define HASH_REPLACEEXPLORERMODE	2422294263
#define HASH_SHOWUSERNAMETITLEBAR	2618183549
#define HASH_HIDESYSTEMFILESGLOBAL	1667356744
#define HASH_HIDELINKEXTENSIONGLOBAL	1073100667
#define HASH_ALLOWMULTIPLEINSTANCES	3463984536
#define HASH_ONECLICKACTIVATE		1118178238
#define HASH_ONECLICKACTIVATEHOVERTIME	3023373873
#define HASH_FORCESAMETABWIDTH		2315576081
#define HASH_DOUBLECLICKTABCLOSE	1866215987
#define HASH_HANDLEZIPFILES			1074212343
#define HASH_INSERTSORTED			1109371947
#define HASH_SHOWPRIVILEGETITLEBAR	4071561587
#define HASH_DISABLEFOLDERSIZENETWORKREMOVABLE	2610679594
#define HASH_ALWAYSSHOWTABBAR		148004675
#define HASH_CHECKBOXSELECTION		456677010
#define HASH_FORCESIZE				1918861263
#define HASH_SIZEDISPLAYFOMRAT		3548127263
#define HASH_CLOSEMAINWINDOWONTABCLOSE	1151827266
#define HASH_SHOWTABBARATBOTTOM		4099029340
#define HASH_SHOWTASKBARTHUMBNAILS	2202555045
#define HASH_SYNCHRONIZETREEVIEW	1687787660
#define HASH_TVAUTOEXPAND			1228854897
#define HASH_OVERWRITEEXISTINGFILESCONFIRMATION	1625342835
#define HASH_LARGETOOLBARICONS		10895007
#define HASH_PLAYNAVIGATIONSOUND	1987363412
#define HASH_ICON_THEME				3998265761

struct ColumnXMLSaveData
{
	TCHAR szName[64];
	ColumnType type;
};

/* Maps column save names to id's. */
static ColumnXMLSaveData ColumnData[] =
{{_T("Name"),ColumnType::Name},{_T("Type"),ColumnType::Type},
{_T("Size"),ColumnType::Size},{_T("DateModified"),ColumnType::DateModified},
{_T("Attributes"),ColumnType::Attributes},{_T("SizeOnDisk"),ColumnType::RealSize},
{_T("ShortName"),ColumnType::ShortName},{_T("Owner"),ColumnType::Owner},
{_T("ProductName"),ColumnType::ProductName},{_T("Company"),ColumnType::Company},
{_T("Description"),ColumnType::Description},{_T("FileVersion"),ColumnType::FileVersion},
{_T("ProductVersion"),ColumnType::ProductVersion},{_T("ShortcutTo"),ColumnType::ShortcutTo},
{_T("HardLinks"),ColumnType::HardLinks},{_T("Extension"),ColumnType::Extension},
{_T("Created"),ColumnType::Created},{_T("Accessed"),ColumnType::Accessed},
{_T("Title"),ColumnType::Title},{_T("Subject"),ColumnType::Subject},
{_T("Author"),ColumnType::Authors},{_T("Keywords"),ColumnType::Keywords},
{_T("Comment"),ColumnType::Comment},{_T("CameraModel"),ColumnType::CameraModel},
{_T("DateTaken"),ColumnType::DateTaken},{_T("Width"),ColumnType::Width},
{_T("Height"),ColumnType::Height},{_T("TotalSize"),ColumnType::TotalSize},
{_T("FreeSpace"),ColumnType::FreeSpace},{_T("FileSystem"),ColumnType::FileSystem},
{_T("OriginalLocation"),ColumnType::OriginalLocation},{_T("DateDeleted"),ColumnType::DateDeleted},
{_T("Documents"),ColumnType::PrinterNumDocuments},{_T("Status"),ColumnType::PrinterStatus}};

unsigned long hash_setting(unsigned char *str);

BOOL LoadWindowPositionFromXML(WINDOWPLACEMENT *pwndpl)
{
	IXMLDOMDocument *pXMLDom;
	TCHAR szConfigFile[MAX_PATH];
	IXMLDOMNodeList		*pNodes = nullptr;
	IXMLDOMNode			*pNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	IXMLDOMNode			*pChildNode = nullptr;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = nullptr;
	VARIANT_BOOL				status;
	VARIANT						var;
	HRESULT						hr;
	long						length;
	long						nChildNodes;
	int							i = 0;

	pXMLDom = NXMLSettings::DomFromCOM();

	if(!pXMLDom)
		goto clean;

	GetProcessImageName(GetCurrentProcessId(),szConfigFile, SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	var = NXMLSettings::VariantString(szConfigFile);
	pXMLDom->load(var, &status);

	if(status != VARIANT_TRUE)
		goto clean;

	bstr = SysAllocString(L"//WindowPosition/*");
	pXMLDom->selectNodes(bstr, &pNodes);

	pwndpl->length = sizeof(WINDOWPLACEMENT);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		/* There should only be one node
		under 'WindowPosition'. */
		if(length == 1)
		{
			pNodes->get_item(0, &pNode);

			hr = pNode->get_attributes(&am);

			if(SUCCEEDED(hr))
			{
				am->get_length(&nChildNodes);

				for(i = 1; i < nChildNodes; i++)
				{
					am->get_item(i, &pChildNode);

					/* Element name. */
					pChildNode->get_nodeName(&bstrName);

					/* Element value. */
					pChildNode->get_text(&bstrValue);

					if(lstrcmp(bstrName, _T("Flags")) == 0)
						pwndpl->flags = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("ShowCmd")) == 0)
						pwndpl->showCmd = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("MinPositionX")) == 0)
						pwndpl->ptMinPosition.x = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("MinPositionY")) == 0)
						pwndpl->ptMinPosition.y = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("MaxPositionX")) == 0)
						pwndpl->ptMaxPosition.x = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("MaxPositionY")) == 0)
						pwndpl->ptMaxPosition.y = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("NormalPositionLeft")) == 0)
						pwndpl->rcNormalPosition.left = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("NormalPositionTop")) == 0)
						pwndpl->rcNormalPosition.top = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("NormalPositionRight")) == 0)
						pwndpl->rcNormalPosition.right = NXMLSettings::DecodeIntValue(bstrValue);
					else if(lstrcmp(bstrName, _T("NormalPositionBottom")) == 0)
						pwndpl->rcNormalPosition.bottom = NXMLSettings::DecodeIntValue(bstrValue);
				}
			}

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	if(&var) VariantClear(&var);
	if(bstr) SysFreeString(bstr);
	if(pNodes) pNodes->Release();
	if(pNode) pNode->Release();

	return TRUE;
}

BOOL LoadAllowMultipleInstancesFromXML()
{
	IXMLDOMDocument *pXMLDom;
	TCHAR szConfigFile[MAX_PATH];
	VARIANT_BOOL status;
	VARIANT var;
	BOOL bAllowMultipleInstances = TRUE;
	BSTR bstr = nullptr;
	IXMLDOMNodeList *pNodes = nullptr;
	IXMLDOMNode *pNode = nullptr;
	IXMLDOMNamedNodeMap *am = nullptr;
	IXMLDOMNode *pNodeAttribute = nullptr;
	BSTR bstrName;
	BSTR bstrValue;
	HRESULT hr;
	BOOL bFound = FALSE;
	long length;

	pXMLDom = NXMLSettings::DomFromCOM();

	if(!pXMLDom)
		goto clean;

	GetProcessImageName(GetCurrentProcessId(),szConfigFile, SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile, NExplorerplusplus::XML_FILENAME);

	var = NXMLSettings::VariantString(szConfigFile);
	pXMLDom->load(var, &status);

	if(status != VARIANT_TRUE)
		goto clean;

	bstr = SysAllocString(L"//Settings/*");
	pXMLDom->selectNodes(bstr, &pNodes);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		for(long i = 0; i < length; i++)
		{
			if(bFound)
				break;

			pNodes->get_item(i, &pNode);

			hr = pNode->get_attributes(&am);

			if(SUCCEEDED(hr))
			{
				hr = am->get_item(0, &pNodeAttribute);

				if(SUCCEEDED(hr))
				{
					/* Element name. */
					pNodeAttribute->get_text(&bstrName);

					/* Element value. */
					pNode->get_text(&bstrValue);

					if(lstrcmp(bstrName, _T("AllowMultipleInstances")) == 0)
					{
						bAllowMultipleInstances = NXMLSettings::DecodeBoolValue(bstrValue);

						bFound = TRUE;
					}

					pNodeAttribute->Release();
					pNodeAttribute = nullptr;
				}
			}

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	if(&var) VariantClear(&var);

	return bAllowMultipleInstances;
}

void Explorerplusplus::LoadGenericSettingsFromXML(IXMLDOMDocument *pXMLDom)
{
	BSTR						bstr = nullptr;
	IXMLDOMNodeList		*pNodes = nullptr;
	IXMLDOMNode			*pNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	IXMLDOMNode			*pNodeAttribute = nullptr;
	BSTR						bstrName;
	BSTR						bstrValue;
	HRESULT						hr;
	long						length;

	if(pXMLDom == nullptr)
		goto clean;

	bstr = SysAllocString(L"//Settings/*");
	pXMLDom->selectNodes(bstr,&pNodes);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		for(long i = 0;i < length;i++)
		{
			pNodes->get_item(i,&pNode);

			hr = pNode->get_attributes(&am);

			if(SUCCEEDED(hr))
			{
				hr = am->get_item(0,&pNodeAttribute);

				if(SUCCEEDED(hr))
				{
					/* Element name. */
					pNodeAttribute->get_text(&bstrName);

					/* Element value. */
					pNode->get_text(&bstrValue);

					/* Map the external attribute and value to an
					internal variable. */
					MapAttributeToValue(pNode,bstrName,bstrValue);

					pNodeAttribute->Release();
					pNodeAttribute = nullptr;
				}
			}

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	return;
}

void Explorerplusplus::SaveGenericSettingsToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement					*pe = nullptr;
	IXMLDOMElement					*pParentNode = nullptr;
	BSTR									bstr = nullptr;
	BSTR									bstr_wsnt= SysAllocString(L"\n\t");
	BSTR									bstr_wsntt = SysAllocString(L"\n\t\t");
	WCHAR									szValue[32];

	bstr = SysAllocString(L"Settings");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = nullptr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AllowMultipleInstances"),NXMLSettings::EncodeBoolValue(m_config->allowMultipleInstances));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AlwaysOpenInNewTab"),NXMLSettings::EncodeBoolValue(m_config->alwaysOpenNewTab));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AlwaysShowTabBar"),NXMLSettings::EncodeBoolValue(m_config->alwaysShowTabBar.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AutoArrangeGlobal"),NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.autoArrange));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("CheckBoxSelection"),NXMLSettings::EncodeBoolValue(m_config->checkBoxSelection));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("CloseMainWindowOnTabClose"),NXMLSettings::EncodeBoolValue(m_config->closeMainWindowOnTabClose));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ConfirmCloseTabs"),NXMLSettings::EncodeBoolValue(m_config->confirmCloseTabs));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DisableFolderSizesNetworkRemovable"),
		NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.disableFolderSizesNetworkRemovable));

	COLORREF centreColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplayCentreColor"));
	centreColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETCENTRECOLOR,0,0);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(centreColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(centreColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(centreColor)));
	pParentNode->Release();
	pParentNode = nullptr;

	HFONT hFont;
	LOGFONT fontInfo;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplayFont"));
	SendMessage(m_hDisplayWindow,DWM_GETFONT,(WPARAM)&hFont,0);
	GetObject(hFont,sizeof(LOGFONT),&fontInfo);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Height"),NXMLSettings::EncodeIntValue(fontInfo.lfHeight));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Width"),NXMLSettings::EncodeIntValue(fontInfo.lfWidth));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Weight"),NXMLSettings::EncodeIntValue(fontInfo.lfWeight));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Italic"),NXMLSettings::EncodeBoolValue(fontInfo.lfItalic));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Underline"),NXMLSettings::EncodeBoolValue(fontInfo.lfUnderline));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Strikeout"),NXMLSettings::EncodeBoolValue(fontInfo.lfStrikeOut));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Font"),fontInfo.lfFaceName);

	COLORREF surroundColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplaySurroundColor"));
	surroundColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETSURROUNDCOLOR,0,0);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(surroundColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(surroundColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(surroundColor)));
	pParentNode->Release();
	pParentNode = nullptr;

	COLORREF textColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplayTextColor"));
	textColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETTEXTCOLOR,0,0);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(textColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(textColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(textColor)));
	pParentNode->Release();
	pParentNode = nullptr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_config->displayWindowWidth,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DisplayWindowWidth"),szValue);

	_itow_s(m_config->displayWindowHeight,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DisplayWindowHeight"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DisplayWindowVertical"),NXMLSettings::EncodeBoolValue(m_config->displayWindowVertical));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DoubleClickTabClose"),NXMLSettings::EncodeBoolValue(m_config->doubleClickTabClose));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ExtendTabControl"),NXMLSettings::EncodeBoolValue(m_config->extendTabControl));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ForceSameTabWidth"),NXMLSettings::EncodeBoolValue(m_config->forceSameTabWidth.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ForceSize"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.forceSize));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("HandleZipFiles"),NXMLSettings::EncodeBoolValue(m_config->handleZipFiles));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("HideLinkExtensionGlobal"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.hideLinkExtension));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("HideSystemFilesGlobal"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.hideSystemFiles));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("InfoTipType"),NXMLSettings::EncodeIntValue(static_cast<int>(m_config->infoTipType)));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("InsertSorted"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.insertSorted));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_config->language,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("Language"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("LargeToolbarIcons"),NXMLSettings::EncodeBoolValue(m_config->useLargeToolbarIcons.get()));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_iLastSelectedTab,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("LastSelectedTab"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("LockToolbars"),NXMLSettings::EncodeBoolValue(m_config->lockToolbars));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("NextToCurrent"),NXMLSettings::EncodeBoolValue(m_config->openNewTabNextToCurrent));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("NewTabDirectory"),m_config->defaultTabDirectory.c_str());
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("OneClickActivate"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.oneClickActivate));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("OneClickActivateHoverTime"),NXMLSettings::EncodeIntValue(m_config->globalFolderSettings.oneClickActivateHoverTime));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("OverwriteExistingFilesConfirmation"),NXMLSettings::EncodeBoolValue(m_config->overwriteExistingFilesConfirmation));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("PlayNavigationSound"),NXMLSettings::EncodeBoolValue(m_config->playNavigationSound));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ReplaceExplorerMode"),NXMLSettings::EncodeIntValue(static_cast<int>(m_config->replaceExplorerMode)));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowAddressBar"),NXMLSettings::EncodeBoolValue(m_config->showAddressBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowApplicationToolbar"),NXMLSettings::EncodeBoolValue(m_config->showApplicationToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowBookmarksToolbar"),NXMLSettings::EncodeBoolValue(m_config->showBookmarksToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowDrivesToolbar"),NXMLSettings::EncodeBoolValue(m_config->showDrivesToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowDisplayWindow"),NXMLSettings::EncodeBoolValue(m_config->showDisplayWindow));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowExtensions"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showExtensions));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFilePreviews"),NXMLSettings::EncodeBoolValue(m_config->showFilePreviews));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFolders"),NXMLSettings::EncodeBoolValue(m_config->showFolders));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFolderSizes"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showFolderSizes));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFriendlyDates"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showFriendlyDates));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFullTitlePath"),NXMLSettings::EncodeBoolValue(m_config->showFullTitlePath.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowGridlinesGlobal"),NXMLSettings::EncodeBoolValue(m_config->globalFolderSettings.showGridlines));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowHiddenGlobal"),NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.showHidden));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowInfoTips"),NXMLSettings::EncodeBoolValue(m_config->showInfoTips));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowInGroupsGlobal"),NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.showInGroups));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowPrivilegeLevelInTitleBar"),NXMLSettings::EncodeBoolValue(m_config->showPrivilegeLevelInTitleBar.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowStatusBar"),NXMLSettings::EncodeBoolValue(m_config->showStatusBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowTabBarAtBottom"),NXMLSettings::EncodeBoolValue(m_config->showTabBarAtBottom));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowTaskbarThumbnails"),NXMLSettings::EncodeBoolValue(m_config->showTaskbarThumbnails));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowToolbar"),NXMLSettings::EncodeBoolValue(m_config->showMainToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowUserNameTitleBar"),NXMLSettings::EncodeBoolValue(m_config->showUserNameInTitleBar.get()));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("SizeDisplayFormat"),NXMLSettings::EncodeIntValue(static_cast<int>(m_config->globalFolderSettings.sizeDisplayFormat)));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("SortAscendingGlobal"),NXMLSettings::EncodeBoolValue(m_config->defaultFolderSettings.sortAscending));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("StartupMode"),NXMLSettings::EncodeIntValue(static_cast<int>(m_config->startupMode)));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("SynchronizeTreeview"),NXMLSettings::EncodeBoolValue(m_config->synchronizeTreeview));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("TVAutoExpandSelected"),NXMLSettings::EncodeBoolValue(m_config->treeViewAutoExpandSelected));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("UseFullRowSelect"),NXMLSettings::EncodeBoolValue(m_config->useFullRowSelect));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt, pe);
	NXMLSettings::WriteStandardSetting(pXMLDom, pe, _T("Setting"), _T("IconTheme"), NXMLSettings::EncodeIntValue(m_config->iconTheme));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt, pe);
	NXMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe, _T("Setting"), _T("ToolbarState"));

	MainToolbarPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom, pParentNode);

	pParentNode->Release();
	pParentNode = nullptr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("TreeViewDelayEnabled"),NXMLSettings::EncodeBoolValue(m_config->treeViewDelayEnabled));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_config->treeViewWidth,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("TreeViewWidth"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_config->defaultFolderSettings.viewMode,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ViewModeGlobal"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = nullptr;

	SaveWindowPositionToXML(pXMLDom,pRoot);
}

int Explorerplusplus::LoadTabSettingsFromXML(IXMLDOMDocument *pXMLDom)
{
	IXMLDOMNodeList		*pNodes = nullptr;
	IXMLDOMNode			*pNode = nullptr;
	IXMLDOMNode			*pColumnsNode = nullptr;
	IXMLDOMNode			*pColumnNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	IXMLDOMNode			*pChildNode = nullptr;
	BSTR				bstrName;
	BSTR				bstrValue;
	BSTR				bstr = nullptr;
	HRESULT				hr;
	TCHAR				szDirectory[MAX_PATH];
	FolderColumns		initialColumns;
	long				length;
	long				lChildNodes;
	long				j = 0;
	int					nTabsCreated = 0;

	if(pXMLDom == nullptr)
		goto clean;

	bstr = SysAllocString(L"//Tabs/*");
	pXMLDom->selectNodes(bstr,&pNodes);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		for(long i = 0;i < length;i++)
		{
			/* This should never fail, as the number
			of nodes has already been counted (so
			they must exist). */
			hr = pNodes->get_item(i,&pNode);

			TabSettings tabSettings;
			FolderSettings folderSettings;

			initialColumns = {};

			if(SUCCEEDED(hr))
			{
				hr = pNode->get_attributes(&am);

				if(SUCCEEDED(hr))
				{
					tabSettings.index = i;
					tabSettings.selected = true;

					/* Retrieve the total number of attributes
					attached to this node. */
					am->get_length(&lChildNodes);

					/* For each tab, the first attribute will just be
					a tab number (0,1,2...). This number can be safely
					ignored. */
					for(j = 1;j < lChildNodes;j++)
					{
						am->get_item(j,&pChildNode);

						/* Element name. */
						pChildNode->get_nodeName(&bstrName);

						/* Element value. */
						pChildNode->get_text(&bstrValue);

						if(lstrcmp(bstrName,L"Directory") == 0)
							StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),bstrValue);
						else
							MapTabAttributeValue(bstrName,bstrValue,tabSettings,folderSettings);
					}

					hr = pNode->get_firstChild(&pColumnsNode);

					if(hr == S_OK)
					{
						hr = pColumnsNode->get_nextSibling(&pColumnsNode);

						if(hr == S_OK)
						{
							hr = pColumnsNode->get_firstChild(&pColumnNode);

							if(hr == S_OK)
							{
								pColumnNode->get_nextSibling(&pColumnNode);

								std::vector<Column_t> column;
								int iColumnType;

								while(pColumnNode != nullptr)
								{
									iColumnType = LoadColumnFromXML(pColumnNode,column);

									switch(iColumnType)
									{
									case COLUMN_TYPE_GENERIC:
										initialColumns.realFolderColumns = column;
										break;

									case COLUMN_TYPE_MYCOMPUTER:
										initialColumns.myComputerColumns = column;
										break;

									case COLUMN_TYPE_CONTROLPANEL:
										initialColumns.controlPanelColumns = column;
										break;

									case COLUMN_TYPE_RECYCLEBIN:
										initialColumns.recycleBinColumns = column;
										break;

									case COLUMN_TYPE_PRINTERS:
										initialColumns.printersColumns = column;
										break;

									case COLUMN_TYPE_NETWORK:
										initialColumns.networkConnectionsColumns = column;
										break;

									case COLUMN_TYPE_NETWORKPLACES:
										initialColumns.myNetworkPlacesColumns = column;
										break;
									}

									pColumnNode->get_nextSibling(&pColumnNode);
									pColumnNode->get_nextSibling(&pColumnNode);
								}
							}
						}
					}

					ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS, initialColumns.realFolderColumns);
					ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS, initialColumns.controlPanelColumns);
					ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS, initialColumns.myComputerColumns);
					ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS, initialColumns.recycleBinColumns);
					ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS, initialColumns.printersColumns);
					ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS, initialColumns.networkConnectionsColumns);
					ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS, initialColumns.myNetworkPlacesColumns);
				}
			}

			hr = m_tabContainer->CreateNewTab(szDirectory, tabSettings, &folderSettings, initialColumns);

			if(hr == S_OK)
				nTabsCreated++;

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();

	return nTabsCreated;
}

void Explorerplusplus::SaveTabSettingsToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement	*pe = nullptr;
	BSTR					bstr = nullptr;
	BSTR					bstr_wsnt= SysAllocString(L"\n\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"Tabs");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = nullptr;

	SaveTabSettingsToXMLnternal(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = nullptr;
}

void Explorerplusplus::SaveTabSettingsToXMLnternal(IXMLDOMDocument *pXMLDom,IXMLDOMElement *pe)
{
	IXMLDOMElement	*pParentNode = nullptr;
	IXMLDOMElement	*pColumnsNode = nullptr;
	BSTR					bstr_wsntt = SysAllocString(L"\n\t\t");
	BSTR					bstr_wsnttt = SysAllocString(L"\n\t\t\t");
	BSTR					bstr = nullptr;
	TCHAR					szNodeName[32];
	UINT					sortMode;
	UINT					viewMode;
	int						tabNum = 0;

	for (auto tabRef : m_tabContainer->GetAllTabsInOrder())
	{
		auto &tab = tabRef.get();

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

		StringCchPrintf(szNodeName, SIZEOF_ARRAY(szNodeName), _T("%d"), tabNum);
		NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Tab"),szNodeName);

		std::wstring tabDirectory = tab.GetShellBrowser()->GetDirectory();
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Directory"), tabDirectory.c_str());

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ApplyFilter"),
			NXMLSettings::EncodeBoolValue(tab.GetShellBrowser()->GetFilterStatus()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("AutoArrange"),
			NXMLSettings::EncodeBoolValue(tab.GetShellBrowser()->GetAutoArrange()));

		std::wstring filter = tab.GetShellBrowser()->GetFilter();
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Filter"),filter.c_str());

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("FilterCaseSensitive"),
			NXMLSettings::EncodeBoolValue(tab.GetShellBrowser()->GetFilterCaseSensitive()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowHidden"),
			NXMLSettings::EncodeBoolValue(tab.GetShellBrowser()->GetShowHidden()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowInGroups"),
			NXMLSettings::EncodeBoolValue(tab.GetShellBrowser()->GetShowInGroups()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SortAscending"),
			NXMLSettings::EncodeBoolValue(tab.GetShellBrowser()->GetSortAscending()));

		sortMode = tab.GetShellBrowser()->GetSortMode();
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SortMode"),NXMLSettings::EncodeIntValue(sortMode));

		viewMode = tab.GetShellBrowser()->GetViewMode();
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ViewMode"),NXMLSettings::EncodeIntValue(viewMode));

		bstr = SysAllocString(L"Columns");
		pXMLDom->createElement(bstr,&pColumnsNode);
		SysFreeString(bstr);
		bstr = nullptr;

		auto folderColumns = tab.GetShellBrowser()->ExportAllColumns();

		int TAB_INDENT = 4;

		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.realFolderColumns, _T("Generic"), TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.myComputerColumns, _T("MyComputer"), TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.controlPanelColumns, _T("ControlPanel"), TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.recycleBinColumns, _T("RecycleBin"), TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.printersColumns, _T("Printers"), TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.networkConnectionsColumns, _T("Network"), TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.myNetworkPlacesColumns, _T("NetworkPlaces"), TAB_INDENT);

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnttt,pColumnsNode);

		/* High-level settings. */
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Locked"),
			NXMLSettings::EncodeBoolValue(tab.GetLockState() == Tab::LockState::Locked));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("AddressLocked"),
			NXMLSettings::EncodeBoolValue(tab.GetLockState() == Tab::LockState::AddressLocked));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("UseCustomName"),
			NXMLSettings::EncodeBoolValue(tab.GetUseCustomName()));

		if(tab.GetUseCustomName())
			NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CustomName"), tab.GetName().c_str());
		else
			NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CustomName"), EMPTY_STRING);

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnttt,pParentNode);

		NXMLSettings::AppendChildToParent(pColumnsNode,pParentNode);
		pColumnsNode->Release();
		pColumnsNode = nullptr;

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pParentNode);

		pParentNode->Release();
		pParentNode = nullptr;

		tabNum++;
	}

	SysFreeString(bstr_wsntt);
	SysFreeString(bstr_wsnttt);
}

int Explorerplusplus::LoadColumnFromXML(IXMLDOMNode *pNode, std::vector<Column_t> &outputColumns)
{
	IXMLDOMNamedNodeMap	*am = nullptr;
	IXMLDOMNode			*pAttributeNode = nullptr;
	Column_t					column;
	BSTR						bstrName;
	BSTR						bstrValue;
	TCHAR						szWidth[32];
	HRESULT						hr;
	long						nAttributeNodes;
	int							iColumnType = -1;
	long						i = 0;

	outputColumns.clear();

	hr = pNode->get_attributes(&am);

	if(FAILED(hr))
		return -1;

	/* Retrieve the total number of attributes
	attached to this node. */
	am->get_length(&nAttributeNodes);

	for(i = 0;i < nAttributeNodes;i++)
	{
		am->get_item(i, &pAttributeNode);

		/* Element name. */
		pAttributeNode->get_nodeName(&bstrName);

		/* Element value. */
		pAttributeNode->get_text(&bstrValue);

		if(lstrcmp(bstrName,_T("name")) == 0)
		{
			if(lstrcmp(bstrValue,_T("Generic")) == 0)
				iColumnType = COLUMN_TYPE_GENERIC;
			else if(lstrcmp(bstrValue,_T("MyComputer")) == 0)
				iColumnType = COLUMN_TYPE_MYCOMPUTER;
			else if(lstrcmp(bstrValue,_T("ControlPanel")) == 0)
				iColumnType = COLUMN_TYPE_CONTROLPANEL;
			else if(lstrcmp(bstrValue,_T("RecycleBin")) == 0)
				iColumnType = COLUMN_TYPE_RECYCLEBIN;
			else if(lstrcmp(bstrValue,_T("Printers")) == 0)
				iColumnType = COLUMN_TYPE_PRINTERS;
			else if(lstrcmp(bstrValue,_T("Network")) == 0)
				iColumnType = COLUMN_TYPE_NETWORK;
			else if(lstrcmp(bstrValue,_T("NetworkPlaces")) == 0)
				iColumnType = COLUMN_TYPE_NETWORKPLACES;
		}
		else
		{
			int j = 0;

			for(j = 0;j < sizeof(ColumnData) / sizeof(ColumnData[0]);j++)
			{
				StringCchPrintf(szWidth,SIZEOF_ARRAY(szWidth),_T("%s_Width"),ColumnData[j].szName);

				if(lstrcmp(bstrName,ColumnData[j].szName) == 0)
				{
					column.type = ColumnData[j].type;

					column.bChecked	= NXMLSettings::DecodeBoolValue(bstrValue);

					outputColumns.push_back(column);
					break;
				}
				else if(lstrcmp(bstrName,szWidth) == 0)
				{
					if(!outputColumns.empty())
					{
						outputColumns.back().iWidth = NXMLSettings::DecodeIntValue(bstrValue);
					}

					break;
				}
			}
		}
	}

	return iColumnType;
}

void Explorerplusplus::LoadBookmarksFromXML(IXMLDOMDocument *pXMLDom)
{
	BookmarkXmlStorage::Load(pXMLDom, &m_bookmarkTree);
}

void Explorerplusplus::SaveBookmarksToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	BookmarkXmlStorage::Save(pXMLDom, pRoot, &m_bookmarkTree, 1);
}

int Explorerplusplus::LoadDefaultColumnsFromXML(IXMLDOMDocument *pXMLDom)
{
	IXMLDOMNodeList	*pNodes = nullptr;
	IXMLDOMNode		*pNode = nullptr;
	BSTR			bstr = nullptr;
	std::vector<Column_t>	columnSet;
	long			length;
	int				iColumnType;

	if(!pXMLDom)
		goto clean;

	bstr = SysAllocString(L"//DefaultColumns/*");
	pXMLDom->selectNodes(bstr,&pNodes);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		auto &folderColumns = m_config->globalFolderSettings.folderColumns;

		for(long i = 0; i < length; i++)
		{
			pNodes->get_item(i, &pNode);

			iColumnType = LoadColumnFromXML(pNode,columnSet);

			switch(iColumnType)
			{
			case COLUMN_TYPE_GENERIC:
				folderColumns.realFolderColumns = columnSet;
				break;

			case COLUMN_TYPE_MYCOMPUTER:
				folderColumns.myComputerColumns = columnSet;
				break;

			case COLUMN_TYPE_CONTROLPANEL:
				folderColumns.controlPanelColumns = columnSet;
				break;

			case COLUMN_TYPE_RECYCLEBIN:
				folderColumns.recycleBinColumns = columnSet;
				break;

			case COLUMN_TYPE_PRINTERS:
				folderColumns.printersColumns = columnSet;
				break;

			case COLUMN_TYPE_NETWORK:
				folderColumns.networkConnectionsColumns = columnSet;
				break;

			case COLUMN_TYPE_NETWORKPLACES:
				folderColumns.myNetworkPlacesColumns = columnSet;
				break;
			}

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();

	return 0;
}

void Explorerplusplus::SaveDefaultColumnsToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement	*pColumnsNode = nullptr;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");
	BSTR					bstr;

	bstr = SysAllocString(L"DefaultColumns");
	pXMLDom->createElement(bstr,&pColumnsNode);
	SysFreeString(bstr);
	bstr = nullptr;

	SaveDefaultColumnsToXMLInternal(pXMLDom,pColumnsNode);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pColumnsNode);
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	NXMLSettings::AppendChildToParent(pColumnsNode,pRoot);
	pColumnsNode->Release();
	pColumnsNode = nullptr;
}

void Explorerplusplus::SaveDefaultColumnsToXMLInternal(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pColumnsNode)
{
	int DEFAULT_INDENT = 2;

	const auto &folderColumns = m_config->globalFolderSettings.folderColumns;

	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.realFolderColumns, _T("Generic"), DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.myComputerColumns, _T("MyComputer"), DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.controlPanelColumns, _T("ControlPanel"), DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.recycleBinColumns, _T("RecycleBin"), DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.printersColumns, _T("Printers"), DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.networkConnectionsColumns, _T("Network"), DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode, folderColumns.myNetworkPlacesColumns, _T("NetworkPlaces"), DEFAULT_INDENT);
}

void Explorerplusplus::SaveColumnToXML(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pColumnsNode, const std::vector<Column_t> &columns,
	const TCHAR *szColumnSet, int iIndent)
{
	IXMLDOMElement	*pColumnNode = nullptr;
	TCHAR			*pszColumnSaveName = nullptr;
	WCHAR			wszIndent[128];
	TCHAR			szWidth[32];
	BSTR			bstr_indent = SysAllocString(L"\n\t\t\t\t");
	int				i = 0;

	StringCchPrintf(wszIndent,SIZEOF_ARRAY(wszIndent),L"\n");

	for(i = 0;i < iIndent;i++)
		StringCchCat(wszIndent,SIZEOF_ARRAY(wszIndent),L"\t");

	bstr_indent = SysAllocString(wszIndent);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_indent,pColumnsNode);
	NXMLSettings::CreateElementNode(pXMLDom,&pColumnNode,pColumnsNode,_T("Column"),szColumnSet);

	for(auto itr = columns.begin();itr != columns.end();itr++)
	{
		for(i = 0;i < sizeof(ColumnData) / sizeof(ColumnData[0]);i++)
		{
			if(ColumnData[i].type == itr->type)
			{
				pszColumnSaveName = ColumnData[i].szName;
				break;
			}
		}

		NXMLSettings::AddAttributeToNode(pXMLDom,pColumnNode,pszColumnSaveName,NXMLSettings::EncodeBoolValue(itr->bChecked));

		StringCchPrintf(szWidth,SIZEOF_ARRAY(szWidth),_T("%s_Width"),pszColumnSaveName);
		NXMLSettings::AddAttributeToNode(pXMLDom,pColumnNode,szWidth,NXMLSettings::EncodeIntValue(itr->iWidth));
	}

	SysFreeString(bstr_indent);
}

void Explorerplusplus::SaveWindowPositionToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement	*pWndPosNode = nullptr;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");
	BSTR					bstr;

	bstr = SysAllocString(L"WindowPosition");
	pXMLDom->createElement(bstr,&pWndPosNode);
	SysFreeString(bstr);
	bstr = nullptr;

	SaveWindowPositionToXMLInternal(pXMLDom,pWndPosNode);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pWndPosNode);
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	NXMLSettings::AppendChildToParent(pWndPosNode,pRoot);
	pWndPosNode->Release();
	pWndPosNode = nullptr;
}

void Explorerplusplus::SaveWindowPositionToXMLInternal(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pWndPosNode)
{
	IXMLDOMElement	*pParentNode = nullptr;
	WINDOWPLACEMENT			wndpl;
	BSTR					bstr_wsntt = SysAllocString(L"\n\t\t");

	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(m_hContainer,&wndpl);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pWndPosNode);

	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pWndPosNode,_T("Setting"),_T("Position"));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Flags"),NXMLSettings::EncodeIntValue(wndpl.flags));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowCmd"),NXMLSettings::EncodeIntValue(wndpl.showCmd));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("MinPositionX"),NXMLSettings::EncodeIntValue(wndpl.ptMinPosition.x));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("MinPositionY"),NXMLSettings::EncodeIntValue(wndpl.ptMinPosition.y));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("MaxPositionX"),NXMLSettings::EncodeIntValue(wndpl.ptMaxPosition.x));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("MaxPositionY"),NXMLSettings::EncodeIntValue(wndpl.ptMaxPosition.y));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("NormalPositionLeft"),NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.left));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("NormalPositionTop"),NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.top));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("NormalPositionRight"),NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.right));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("NormalPositionBottom"),NXMLSettings::EncodeIntValue(wndpl.rcNormalPosition.bottom));

	pParentNode->Release();
	pParentNode = nullptr;
}

void Explorerplusplus::LoadToolbarInformationFromXML(IXMLDOMDocument *pXMLDom)
{
	IXMLDOMNodeList		*pNodes = nullptr;
	IXMLDOMNode			*pNode = nullptr;
	IXMLDOMNamedNodeMap	*am = nullptr;
	IXMLDOMNode			*pChildNode = nullptr;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = nullptr;
	HRESULT						hr;
	long						length;
	long						lChildNodes;
	long						j = 0;

	if(pXMLDom == nullptr)
		goto clean;

	bstr = SysAllocString(L"//Toolbars/*");
	pXMLDom->selectNodes(bstr,&pNodes);

	if(!pNodes)
	{
		goto clean;
	}
	else
	{
		pNodes->get_length(&length);

		for(long i = 0;i < length;i++)
		{
			/* This should never fail, as the number
			of nodes has already been counted (so
			they must exist). */
			hr = pNodes->get_item(i,&pNode);

			if(SUCCEEDED(hr))
			{
				hr = pNode->get_attributes(&am);

				if(SUCCEEDED(hr))
				{
					BOOL bUseChevron = FALSE;

					if(m_ToolbarInformation[i].fStyle & RBBS_USECHEVRON)
						bUseChevron = TRUE;

					/* Retrieve the total number of attributes
					attached to this node. */
					am->get_length(&lChildNodes);

					/* For each tab, the first attribute will just be
					a toolbar number (0,1,2...). This number can be safely
					ignored. */
					for(j = 1;j < lChildNodes;j++)
					{
						am->get_item(j,&pChildNode);

						/* Element name. */
						pChildNode->get_nodeName(&bstrName);

						/* Element value. */
						pChildNode->get_text(&bstrValue);

						if(lstrcmp(bstrName,L"id") == 0)
							m_ToolbarInformation[i].wID = NXMLSettings::DecodeIntValue(bstrValue);
						else if(lstrcmp(bstrName,L"Style") == 0)
							m_ToolbarInformation[i].fStyle = NXMLSettings::DecodeIntValue(bstrValue);
						else if(lstrcmp(bstrName,L"Length") == 0)
							m_ToolbarInformation[i].cx = NXMLSettings::DecodeIntValue(bstrValue);
					}

					if(bUseChevron)
						m_ToolbarInformation[i].fStyle |= RBBS_USECHEVRON;
				}
			}

			pNode->Release();
			pNode = nullptr;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();
}

void Explorerplusplus::SaveToolbarInformationToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement	*pe = nullptr;
	BSTR					bstr = nullptr;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"Toolbars");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = nullptr;

	SaveToolbarInformationToXMLnternal(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = nullptr;
}

void Explorerplusplus::SaveToolbarInformationToXMLnternal(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pe)
{
	IXMLDOMElement	*pParentNode = nullptr;
	BSTR					bstr_wsntt = SysAllocString(L"\n\t\t");
	BSTR					bstr_wsnttt = SysAllocString(L"\n\t\t\t");
	REBARBANDINFO			rbi;
	TCHAR					szNodeName[32];
	int						nBands;
	int						i = 0;

	nBands = (int)SendMessage(m_hMainRebar,RB_GETBANDCOUNT,0,0);

	for(i = 0;i < nBands;i++)
	{
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

		rbi.cbSize = sizeof(rbi);
		rbi.fMask = RBBIM_ID|RBBIM_CHILD|RBBIM_SIZE|RBBIM_STYLE;
		SendMessage(m_hMainRebar,RB_GETBANDINFO,i,(LPARAM)&rbi);

		StringCchPrintf(szNodeName, SIZEOF_ARRAY(szNodeName), _T("%d"), i);
		NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Toolbar"),szNodeName);

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("id"),
			NXMLSettings::EncodeIntValue(rbi.wID));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Style"),
			NXMLSettings::EncodeIntValue(rbi.fStyle));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Length"),
			NXMLSettings::EncodeIntValue(rbi.cx));

		pParentNode->Release();
		pParentNode = nullptr;
	}

	SysFreeString(bstr_wsntt);
	SysFreeString(bstr_wsnttt);
}

void Explorerplusplus::LoadApplicationToolbarFromXML(IXMLDOMDocument *pXMLDom)
{
	IXMLDOMNodeList		*pNodes = nullptr;
	IXMLDOMNode			*pNode = nullptr;
	BSTR						bstr = nullptr;
	HRESULT						hr;

	if(!pXMLDom)
		goto clean;

	bstr = SysAllocString(L"//ApplicationButton");
	hr = pXMLDom->selectSingleNode(bstr,&pNode);

	if(hr == S_OK)
	{
		ApplicationToolbarPersistentSettings::GetInstance().LoadXMLSettings(pNode);
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();
}

void Explorerplusplus::SaveApplicationToolbarToXML(IXMLDOMDocument *pXMLDom,
IXMLDOMElement *pRoot)
{
	IXMLDOMElement		*pe = nullptr;
	BSTR						bstr_wsnt = SysAllocString(L"\n\t");
	BSTR						bstr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"ApplicationToolbar");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = nullptr;

	ApplicationToolbarPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe, pRoot);
	pe->Release();
	pe = nullptr;

	SysFreeString(bstr_wsnt);
}

unsigned long hash_setting(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while((c = *str++) != '\0')
		hash = ((hash << 5) + hash) + c;

	return hash;
}

/* Maps attribute name to their corresponding internal variable. */
void Explorerplusplus::MapAttributeToValue(IXMLDOMNode *pNode,
WCHAR *wszName,WCHAR *wszValue)
{
	unsigned char	szName[512];
	unsigned long	uNameHash;

	WideCharToMultiByte(CP_ACP,0,wszName,-1,(LPSTR)szName,
		SIZEOF_ARRAY(szName), nullptr, nullptr);

	uNameHash = hash_setting(szName);

	switch(uNameHash)
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
		m_config->globalFolderSettings.disableFolderSizesNetworkRemovable = NXMLSettings::DecodeBoolValue(wszValue);
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
		m_config->extendTabControl = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_FORCESAMETABWIDTH:
		m_config->forceSameTabWidth.set(NXMLSettings::DecodeBoolValue(wszValue));
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
		m_config->globalFolderSettings.oneClickActivateHoverTime = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_OVERWRITEEXISTINGFILESCONFIRMATION:
		m_config->overwriteExistingFilesConfirmation = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_PLAYNAVIGATIONSOUND:
		m_config->playNavigationSound = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_REPLACEEXPLORERMODE:
		m_config->replaceExplorerMode = static_cast<DefaultFileManager::ReplaceExplorerMode>(NXMLSettings::DecodeIntValue(wszValue));
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
		m_config->showTabBarAtBottom = NXMLSettings::DecodeBoolValue(wszValue);
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
		m_config->globalFolderSettings.sizeDisplayFormat = static_cast<SizeDisplayFormat>(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SORTASCENDINGGLOBAL:
		m_config->defaultFolderSettings.sortAscending = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_STARTUPMODE:
		m_config->startupMode = static_cast<StartupMode>(NXMLSettings::DecodeIntValue(wszValue));
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
		MainToolbarPersistentSettings::GetInstance().LoadXMLSettings(pNode);
		break;

	case HASH_TREEVIEWDELAYENABLED:
		m_config->treeViewDelayEnabled = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TREEVIEWWIDTH:
		m_config->treeViewWidth = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_VIEWMODEGLOBAL:
		m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_POSITION:
		{
			IXMLDOMNode	*pChildNode = nullptr;
			IXMLDOMNamedNodeMap	*am = nullptr;
			WINDOWPLACEMENT wndpl;
			BSTR		bstrName;
			BSTR		bstrValue;
			BOOL		bMaximized = FALSE;

			pNode->get_attributes(&am);

			long lChildNodes;
			long j = 0;

			/* Retrieve the total number of attributes
			attached to this node. */
			am->get_length(&lChildNodes);

			for(j = 1;j < lChildNodes;j++)
			{
				am->get_item(j, &pChildNode);

				/* Element name. */
				pChildNode->get_nodeName(&bstrName);

				/* Element value. */
				pChildNode->get_text(&bstrValue);

				if(lstrcmp(bstrName,L"Left") == 0)
					wndpl.rcNormalPosition.left = NXMLSettings::DecodeIntValue(bstrValue);
				else if(lstrcmp(bstrName,L"Top") == 0)
					wndpl.rcNormalPosition.top = NXMLSettings::DecodeIntValue(bstrValue);
				else if(lstrcmp(bstrName,L"Right") == 0)
					wndpl.rcNormalPosition.right = NXMLSettings::DecodeIntValue(bstrValue);
				else if(lstrcmp(bstrName,L"Bottom") == 0)
					wndpl.rcNormalPosition.bottom = NXMLSettings::DecodeIntValue(bstrValue);
				else if(lstrcmp(bstrName,L"Maximized") == 0)
					bMaximized = NXMLSettings::DecodeBoolValue(bstrValue);
			}

			wndpl.length	= sizeof(WINDOWPLACEMENT);
			wndpl.showCmd	= SW_HIDE;

			if(bMaximized)
				wndpl.showCmd |= SW_MAXIMIZE;

			SetWindowPlacement(m_hContainer,&wndpl);
		}
		break;

	case HASH_NEWTABDIRECTORY:
		m_config->defaultTabDirectory = wszValue;
		break;

	case HASH_INFOTIPTYPE:
		m_config->infoTipType = static_cast<InfoTipType>(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_ICON_THEME:
		m_config->iconTheme = IconTheme::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
		break;
	}
}

void Explorerplusplus::MapTabAttributeValue(WCHAR *wszName,WCHAR *wszValue,
	TabSettings &tabSettings, FolderSettings &folderSettings)
{
	if(lstrcmp(wszName,L"ApplyFilter") == 0)
	{
		folderSettings.applyFilter = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"AutoArrange") == 0)
	{
		folderSettings.autoArrange = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"Filter") == 0)
	{
		folderSettings.filter = wszValue;
	}
	else if(lstrcmp(wszName,L"FilterCaseSensitive") == 0)
	{
		folderSettings.filterCaseSensitive = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"ShowHidden") == 0)
	{
		folderSettings.showHidden = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"ShowInGroups") == 0)
	{
		folderSettings.showInGroups = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"SortAscending") == 0)
	{
		folderSettings.sortAscending = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"SortMode") == 0)
	{
		folderSettings.sortMode = SortMode::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
	}
	else if(lstrcmp(wszName,L"ViewMode") == 0)
	{
		folderSettings.viewMode = ViewMode::_from_integral(NXMLSettings::DecodeIntValue(wszValue));
	}
	else if(lstrcmp(wszName,L"Locked") == 0)
	{
		BOOL locked = NXMLSettings::DecodeBoolValue(wszValue);

		if (locked)
		{
			tabSettings.lockState = Tab::LockState::Locked;
		}
	}
	else if(lstrcmp(wszName,L"AddressLocked") == 0)
	{
		BOOL addressLocked = NXMLSettings::DecodeBoolValue(wszValue);

		if (addressLocked)
		{
			tabSettings.lockState = Tab::LockState::AddressLocked;
		}
	}
	else if(lstrcmp(wszName,L"CustomName") == 0)
	{
		tabSettings.name = wszValue;
	}
}
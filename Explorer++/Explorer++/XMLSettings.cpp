/******************************************************************
 *
 * Project: Explorer++
 * File: XMLSettings.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Loads and saves all settings associated with
 * Explorer++ from/to an XML configuration file.
 *
 * Notes:
 *  - Attribute names and values must conform to
 *    the following rules:
 *     - No spaces
 *     - First character cannot be a number
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/XMLSettings.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/Macros.h"

#import <msxml3.dll> raw_interfaces_only


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
#define HASH_ALWAYSSHOWSIZESINBYTES	1986148483
#define HASH_AUTOARRANGEGLOBAL		151507311
#define HASH_CONFIRMCLOSETABS		2636757395
#define HASH_DISPLAYCENTRECOLOR		3404143227
#define HASH_DISPLAYFONT			362757714
#define HASH_DISPLAYSURROUNDCOLOR	1807564604
#define HASH_DISPLAYTEXTCOLOR		4212809823
#define HASH_DISPLAYWINDOWHEIGHT	2017415020
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

struct ColumnXMLSaveData
{
	TCHAR			szName[64];
	unsigned int	id;
};

/* Maps column save names to id's. */
static ColumnXMLSaveData ColumnData[] =
{{_T("Name"),CM_NAME},{_T("Type"),CM_TYPE},
{_T("Size"),CM_SIZE},{_T("DateModified"),CM_DATEMODIFIED},
{_T("Attributes"),CM_ATTRIBUTES},{_T("SizeOnDisk"),CM_REALSIZE},
{_T("ShortName"),CM_SHORTNAME},{_T("Owner"),CM_OWNER},
{_T("ProductName"),CM_PRODUCTNAME},{_T("Company"),CM_COMPANY},
{_T("Description"),CM_DESCRIPTION},{_T("FileVersion"),CM_FILEVERSION},
{_T("ProductVersion"),CM_PRODUCTVERSION},{_T("ShortcutTo"),CM_SHORTCUTTO},
{_T("HardLinks"),CM_HARDLINKS},{_T("Extension"),CM_EXTENSION},
{_T("Created"),CM_CREATED},{_T("Accessed"),CM_ACCESSED},
{_T("Title"),CM_TITLE},{_T("Subject"),CM_SUBJECT},
{_T("Author"),CM_AUTHOR},{_T("Keywords"),CM_KEYWORDS},
{_T("Comment"),CM_COMMENT},{_T("CameraModel"),CM_CAMERAMODEL},
{_T("DateTaken"),CM_DATETAKEN},{_T("Width"),CM_WIDTH},
{_T("Height"),CM_HEIGHT},{_T("TotalSize"),CM_TOTALSIZE},
{_T("FreeSpace"),CM_FREESPACE},{_T("FileSystem"),CM_FILESYSTEM},
{_T("OriginalLocation"),CM_ORIGINALLOCATION},{_T("DateDeleted"),CM_DATEDELETED},
{_T("Documents"),CM_NUMPRINTERDOCUMENTS},{_T("Status"),CM_PRINTERSTATUS}};

unsigned long hash_setting(unsigned char *str);

BOOL LoadWindowPositionFromXML(WINDOWPLACEMENT *pwndpl)
{
	MSXML2::IXMLDOMDocument *pXMLDom;
	TCHAR szConfigFile[MAX_PATH];
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pChildNode = NULL;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = NULL;
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
			pNode = NULL;
		}
	}

clean:
	if(&var) VariantClear(&var);
	if(bstr) SysFreeString(bstr);
	if(pNodes) pNodes->Release();
	if(pNode) pNode->Release();

	return TRUE;
}

BOOL LoadAllowMultipleInstancesFromXML(void)
{
	MSXML2::IXMLDOMDocument *pXMLDom;
	TCHAR szConfigFile[MAX_PATH];
	VARIANT_BOOL status;
	VARIANT var;
	BOOL bAllowMultipleInstances = TRUE;

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

	BSTR						bstr = NULL;
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pNodeAttribute = NULL;
	BSTR						bstrName;
	BSTR						bstrValue;
	HRESULT						hr;
	BOOL						bFound = FALSE;
	long						length;

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
					pNodeAttribute = NULL;
				}
			}

			pNode->Release();
			pNode = NULL;
		}
	}

clean:
	if(&var) VariantClear(&var);

	return bAllowMultipleInstances;
}

void Explorerplusplus::LoadGenericSettingsFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	BSTR						bstr = NULL;
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pNodeAttribute = NULL;
	BSTR						bstrName;
	BSTR						bstrValue;
	HRESULT						hr;
	long						length;

	if(pXMLDom == NULL)
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
					pNodeAttribute = NULL;
				}
			}

			pNode->Release();
			pNode = NULL;
		}
	}

clean:
	return;
}

void Explorerplusplus::SaveGenericSettingsToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement					*pe = NULL;
	MSXML2::IXMLDOMElement					*pParentNode = NULL;
	BSTR									bstr = NULL;
	BSTR									bstr_wsnt= SysAllocString(L"\n\t");
	BSTR									bstr_wsntt = SysAllocString(L"\n\t\t");
	WCHAR									szValue[32];

	bstr = SysAllocString(L"Settings");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = NULL;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AllowMultipleInstances"),NXMLSettings::EncodeBoolValue(m_bAllowMultipleInstances));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AlwaysOpenInNewTab"),NXMLSettings::EncodeBoolValue(m_bAlwaysOpenNewTab));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AlwaysShowTabBar"),NXMLSettings::EncodeBoolValue(m_bAlwaysShowTabBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("AutoArrangeGlobal"),NXMLSettings::EncodeBoolValue(m_bAutoArrangeGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("CheckBoxSelection"),NXMLSettings::EncodeBoolValue(m_bCheckBoxSelection));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("CloseMainWindowOnTabClose"),NXMLSettings::EncodeBoolValue(m_bCloseMainWindowOnTabClose));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ConfirmCloseTabs"),NXMLSettings::EncodeBoolValue(m_bConfirmCloseTabs));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DisableFolderSizesNetworkRemovable"),NXMLSettings::EncodeBoolValue(m_bDisableFolderSizesNetworkRemovable));

	COLORREF CentreColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplayCentreColor"));
	CentreColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETCENTRECOLOR,0,0);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(CentreColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(CentreColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(CentreColor)));
	pParentNode->Release();
	pParentNode = NULL;

	HFONT hFont;
	LOGFONT FontInfo;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplayFont"));
	SendMessage(m_hDisplayWindow,DWM_GETFONT,(WPARAM)&hFont,0);
	GetObject(hFont,sizeof(LOGFONT),&FontInfo);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Height"),NXMLSettings::EncodeIntValue(FontInfo.lfHeight));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Width"),NXMLSettings::EncodeIntValue(FontInfo.lfWidth));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Weight"),NXMLSettings::EncodeIntValue(FontInfo.lfWeight));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Italic"),NXMLSettings::EncodeBoolValue(FontInfo.lfItalic));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Underline"),NXMLSettings::EncodeBoolValue(FontInfo.lfUnderline));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Strikeout"),NXMLSettings::EncodeBoolValue(FontInfo.lfStrikeOut));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Font"),FontInfo.lfFaceName);

	COLORREF SurroundColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplaySurroundColor"));
	SurroundColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETSURROUNDCOLOR,0,0);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(SurroundColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(SurroundColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(SurroundColor)));
	pParentNode->Release();
	pParentNode = NULL;

	COLORREF TextColor;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("DisplayTextColor"));
	TextColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETTEXTCOLOR,0,0);
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("r"),NXMLSettings::EncodeIntValue(GetRValue(TextColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("g"),NXMLSettings::EncodeIntValue(GetGValue(TextColor)));
	NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("b"),NXMLSettings::EncodeIntValue(GetBValue(TextColor)));
	pParentNode->Release();
	pParentNode = NULL;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_DisplayWindowHeight,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DisplayWindowHeight"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("DoubleClickTabClose"),NXMLSettings::EncodeBoolValue(m_bDoubleClickTabClose));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ExtendTabControl"),NXMLSettings::EncodeBoolValue(m_bExtendTabControl));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ForceSameTabWidth"),NXMLSettings::EncodeBoolValue(m_bForceSameTabWidth));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ForceSize"),NXMLSettings::EncodeBoolValue(m_bForceSize));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("HandleZipFiles"),NXMLSettings::EncodeBoolValue(m_bHandleZipFiles));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("HideLinkExtensionGlobal"),NXMLSettings::EncodeBoolValue(m_bHideLinkExtensionGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("HideSystemFilesGlobal"),NXMLSettings::EncodeBoolValue(m_bHideSystemFilesGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("InfoTipType"),NXMLSettings::EncodeIntValue(m_InfoTipType));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("InsertSorted"),NXMLSettings::EncodeBoolValue(m_bInsertSorted));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_Language,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("Language"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("LargeToolbarIcons"),NXMLSettings::EncodeBoolValue(m_bLargeToolbarIcons));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_iLastSelectedTab,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("LastSelectedTab"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("LockToolbars"),NXMLSettings::EncodeBoolValue(m_bLockToolbars));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("NextToCurrent"),NXMLSettings::EncodeBoolValue(m_bOpenNewTabNextToCurrent));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("NewTabDirectory"),m_DefaultTabDirectory);
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("OneClickActivate"),NXMLSettings::EncodeBoolValue(m_bOneClickActivate));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("OneClickActivateHoverTime"),NXMLSettings::EncodeIntValue(m_OneClickActivateHoverTime));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("OverwriteExistingFilesConfirmation"),NXMLSettings::EncodeBoolValue(m_bOverwriteExistingFilesConfirmation));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("PlayNavigationSound"),NXMLSettings::EncodeBoolValue(m_bPlayNavigationSound));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_ReplaceExplorerMode,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ReplaceExplorerMode"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowAddressBar"),NXMLSettings::EncodeBoolValue(m_bShowAddressBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowApplicationToolbar"),NXMLSettings::EncodeBoolValue(m_bShowApplicationToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowBookmarksToolbar"),NXMLSettings::EncodeBoolValue(m_bShowBookmarksToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowDrivesToolbar"),NXMLSettings::EncodeBoolValue(m_bShowDrivesToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowDisplayWindow"),NXMLSettings::EncodeBoolValue(m_bShowDisplayWindow));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowExtensions"),NXMLSettings::EncodeBoolValue(m_bShowExtensionsGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFilePreviews"),NXMLSettings::EncodeBoolValue(m_bShowFilePreviews));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFolders"),NXMLSettings::EncodeBoolValue(m_bShowFolders));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFolderSizes"),NXMLSettings::EncodeBoolValue(m_bShowFolderSizes));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFriendlyDates"),NXMLSettings::EncodeBoolValue(m_bShowFriendlyDatesGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowFullTitlePath"),NXMLSettings::EncodeBoolValue(m_bShowFullTitlePath));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowGridlinesGlobal"),NXMLSettings::EncodeBoolValue(m_bShowGridlinesGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowHiddenGlobal"),NXMLSettings::EncodeBoolValue(m_bShowHiddenGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowInfoTips"),NXMLSettings::EncodeBoolValue(m_bShowInfoTips));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowInGroupsGlobal"),NXMLSettings::EncodeBoolValue(m_bShowInGroupsGlobal));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowPrivilegeLevelInTitleBar"),NXMLSettings::EncodeBoolValue(m_bShowPrivilegeLevelInTitleBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowStatusBar"),NXMLSettings::EncodeBoolValue(m_bShowStatusBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowTabBarAtBottom"),NXMLSettings::EncodeBoolValue(m_bShowTabBarAtBottom));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowTaskbarThumbnails"),NXMLSettings::EncodeBoolValue(m_bShowTaskbarThumbnails));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowToolbar"),NXMLSettings::EncodeBoolValue(m_bShowMainToolbar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ShowUserNameTitleBar"),NXMLSettings::EncodeBoolValue(m_bShowUserNameInTitleBar));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("SizeDisplayFormat"),NXMLSettings::EncodeIntValue(m_SizeDisplayFormat));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("SortAscendingGlobal"),NXMLSettings::EncodeBoolValue(m_bSortAscendingGlobal));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_StartupMode,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("StartupMode"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("SynchronizeTreeview"),NXMLSettings::EncodeBoolValue(m_bSynchronizeTreeview));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("TVAutoExpandSelected"),NXMLSettings::EncodeBoolValue(m_bTVAutoExpandSelected));
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("UseFullRowSelect"),NXMLSettings::EncodeBoolValue(m_bUseFullRowSelect));

	TBBUTTON tbButton;
	TCHAR szButtonAttributeName[32];
	TCHAR szButtonName[256];
	int nButtons;
	int idCommand;
	int i = 0;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Setting"),_T("ToolbarState"));

	nButtons = (int)SendMessage(m_hMainToolbar,TB_BUTTONCOUNT,0,0);

	for(i = 0;i < nButtons;i++)
	{
		SendMessage(m_hMainToolbar,TB_GETBUTTON,i,(LPARAM)&tbButton);

		StringCchPrintf(szButtonAttributeName,SIZEOF_ARRAY(szButtonAttributeName),_T("Button%d"),i);

		if(tbButton.idCommand == 0)
			idCommand = TOOLBAR_SEPARATOR;
		else
			idCommand = tbButton.idCommand;

		/* ALL settings are saved in English. */
		LoadString(GetModuleHandle(0),LookupToolbarButtonTextID(idCommand),
			szButtonName,SIZEOF_ARRAY(szButtonName));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,szButtonAttributeName,szButtonName);
	}

	pParentNode->Release();
	pParentNode = NULL;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("TreeViewDelayEnabled"),NXMLSettings::EncodeBoolValue(m_bTreeViewDelayEnabled));

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_TreeViewWidth,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("TreeViewWidth"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);
	_itow_s(m_ViewModeGlobal,szValue,SIZEOF_ARRAY(szValue),10);
	NXMLSettings::WriteStandardSetting(pXMLDom,pe,_T("Setting"),_T("ViewModeGlobal"),szValue);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = NULL;

	SaveWindowPositionToXML(pXMLDom,pRoot);
}

int Explorerplusplus::LoadTabSettingsFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	MSXML2::IXMLDOMNode			*pColumnsNode = NULL;
	MSXML2::IXMLDOMNode			*pColumnNode = NULL;
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pChildNode = NULL;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = NULL;
	HRESULT						hr;
	InitialSettings_t			*pSettings = NULL;
	TabInfo_t					*pTabInfo = NULL;
	TCHAR						szDirectory[MAX_PATH];
	std::list<Column_t>			RealFolderColumnList;
	std::list<Column_t>			MyComputerColumnList;
	std::list<Column_t>			ControlPanelColumnList;
	std::list<Column_t>			RecycleBinColumnList;
	std::list<Column_t>			PrintersColumnList;
	std::list<Column_t>			NetworkConnectionsColumnList;
	std::list<Column_t>			MyNetworkPlacesColumnList;
	long						length;
	long						lChildNodes;
	long						j = 0;
	int							nTabsCreated = 0;

	if(pXMLDom == NULL)
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

		pSettings = (InitialSettings_t *)malloc(sizeof(InitialSettings_t) * length);
		pTabInfo = (TabInfo_t *)malloc(sizeof(TabInfo_t) * length);

		for(long i = 0;i < length;i++)
		{
			/* This should never fail, as the number
			of nodes has already been counted (so
			they must exist). */
			hr = pNodes->get_item(i,&pNode);

			RealFolderColumnList.clear();
			MyComputerColumnList.clear();
			ControlPanelColumnList.clear();
			RecycleBinColumnList.clear();
			PrintersColumnList.clear();
			NetworkConnectionsColumnList.clear();
			MyNetworkPlacesColumnList.clear();

			if(SUCCEEDED(hr))
			{
				hr = pNode->get_attributes(&am);

				if(SUCCEEDED(hr))
				{
					/* Retrieve the total number of attributes
					attached to this node. */
					am->get_length(&lChildNodes);

					SetDefaultTabSettings(&pTabInfo[i]);

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
							MapTabAttributeValue(bstrName,bstrValue,&pSettings[i],&pTabInfo[i]);
					}

					pSettings[i].bShowFolderSizes = m_bShowFolderSizes;
					pSettings[i].bDisableFolderSizesNetworkRemovable = m_bDisableFolderSizesNetworkRemovable;

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

								std::list<Column_t> Column;
								int iColumnType;

								while(pColumnNode != NULL)
								{
									iColumnType = LoadColumnFromXML(pColumnNode,&Column);

									switch(iColumnType)
									{
									case COLUMN_TYPE_GENERIC:
										RealFolderColumnList = Column;
										break;

									case COLUMN_TYPE_MYCOMPUTER:
										MyComputerColumnList = Column;
										break;

									case COLUMN_TYPE_CONTROLPANEL:
										ControlPanelColumnList = Column;
										break;

									case COLUMN_TYPE_RECYCLEBIN:
										RecycleBinColumnList = Column;
										break;

									case COLUMN_TYPE_PRINTERS:
										PrintersColumnList = Column;
										break;

									case COLUMN_TYPE_NETWORK:
										NetworkConnectionsColumnList = Column;
										break;

									case COLUMN_TYPE_NETWORKPLACES:
										MyNetworkPlacesColumnList = Column;
										break;
									}

									pColumnNode->get_nextSibling(&pColumnNode);
									pColumnNode->get_nextSibling(&pColumnNode);
								}
							}
						}
					}

					pSettings[i].pRealFolderColumnList = &RealFolderColumnList;
					pSettings[i].pMyComputerColumnList = &MyComputerColumnList;
					pSettings[i].pControlPanelColumnList = &ControlPanelColumnList;
					pSettings[i].pRecycleBinColumnList = &RecycleBinColumnList;
					pSettings[i].pPrintersColumnList = &PrintersColumnList;
					pSettings[i].pNetworkConnectionsColumnList = &NetworkConnectionsColumnList;
					pSettings[i].pMyNetworkPlacesColumnList = &MyNetworkPlacesColumnList;

					ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS,&(*pSettings[i].pRealFolderColumnList));
					ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS,&(*pSettings[i].pControlPanelColumnList));
					ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS,&(*pSettings[i].pMyComputerColumnList));
					ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS,&(*pSettings[i].pRecycleBinColumnList));
					ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS,&(*pSettings[i].pPrintersColumnList));
					ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS,&(*pSettings[i].pNetworkConnectionsColumnList));
					ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS,&(*pSettings[i].pMyNetworkPlacesColumnList));
				}
			}

			hr = CreateNewTab(szDirectory,&pSettings[i],&pTabInfo[i],TRUE,NULL);

			if(hr == S_OK)
				nTabsCreated++;

			pNode->Release();
			pNode = NULL;
		}

		free(pTabInfo);
		free(pSettings);
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();

	return nTabsCreated;
}

void Explorerplusplus::SaveTabSettingsToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement	*pe = NULL;
	BSTR					bstr = NULL;
	BSTR					bstr_wsnt= SysAllocString(L"\n\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"Tabs");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = NULL;

	SaveTabSettingsToXMLnternal(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = NULL;
}

void Explorerplusplus::SaveTabSettingsToXMLnternal(MSXML2::IXMLDOMDocument *pXMLDom,MSXML2::IXMLDOMElement *pe)
{
	MSXML2::IXMLDOMElement	*pParentNode = NULL;
	MSXML2::IXMLDOMElement	*pColumnsNode = NULL;
	TCITEM					tcItem;
	BSTR					bstr_wsntt = SysAllocString(L"\n\t\t");
	BSTR					bstr_wsnttt = SysAllocString(L"\n\t\t\t");
	BSTR					bstr = NULL;
	TCHAR					szNodeName[32];
	TCHAR					szTabDirectory[MAX_PATH];
	UINT					SortMode;
	UINT					ViewMode;
	int						nTabs;
	int						i = 0;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < nTabs;i++)
	{
		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pe);

		tcItem.mask	= TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		StringCchPrintf(szNodeName, SIZEOF_ARRAY(szNodeName), _T("%d"), i);
		NXMLSettings::CreateElementNode(pXMLDom,&pParentNode,pe,_T("Tab"),szNodeName);

		m_pShellBrowser[(int) tcItem.lParam]->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory), szTabDirectory);
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Directory"),szTabDirectory);

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ApplyFilter"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->GetFilterStatus()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("AutoArrange"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->GetAutoArrange()));

		TCHAR szFilter[512];

		m_pShellBrowser[(int)tcItem.lParam]->GetFilter(szFilter,SIZEOF_ARRAY(szFilter));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Filter"),szFilter);

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("FilterCaseSensitive"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->GetFilterCaseSensitive()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowGridlines"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->QueryGridlinesActive()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowHidden"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->GetShowHidden()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ShowInGroups"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->IsGroupViewEnabled()));

		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SortAscending"),
			NXMLSettings::EncodeBoolValue(m_pShellBrowser[(int)tcItem.lParam]->GetSortAscending()));

		SortMode = m_pShellBrowser[(int) tcItem.lParam]->GetSortMode();
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("SortMode"),NXMLSettings::EncodeIntValue(SortMode));

		ViewMode = m_pShellBrowser[(int) tcItem.lParam]->GetCurrentViewMode();
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("ViewMode"),NXMLSettings::EncodeIntValue(ViewMode));

		bstr = SysAllocString(L"Columns");
		pXMLDom->createElement(bstr,&pColumnsNode);
		SysFreeString(bstr);
		bstr = NULL;

		ColumnExport_t ce;

		m_pShellBrowser[(int)tcItem.lParam]->ExportAllColumns(&ce);

		int TAB_INDENT = 4;

		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.RealFolderColumnList,_T("Generic"),TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.MyComputerColumnList,_T("MyComputer"),TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.ControlPanelColumnList,_T("ControlPanel"),TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.RecycleBinColumnList,_T("RecycleBin"),TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.PrintersColumnList,_T("Printers"),TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.NetworkConnectionsColumnList,_T("Network"),TAB_INDENT);
		SaveColumnToXML(pXMLDom,pColumnsNode,&ce.MyNetworkPlacesColumnList,_T("NetworkPlaces"),TAB_INDENT);

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnttt,pColumnsNode);

		/* High-level settings. */
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("Locked"),
			NXMLSettings::EncodeBoolValue(m_TabInfo.at((int)tcItem.lParam).bLocked));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("AddressLocked"),
			NXMLSettings::EncodeBoolValue(m_TabInfo.at((int)tcItem.lParam).bAddressLocked));
		NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("UseCustomName"),
			NXMLSettings::EncodeBoolValue(m_TabInfo.at((int)tcItem.lParam).bUseCustomName));

		if(m_TabInfo.at((int)tcItem.lParam).bUseCustomName)
			NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CustomName"),
			m_TabInfo.at((int)tcItem.lParam).szName);
		else
			NXMLSettings::AddAttributeToNode(pXMLDom,pParentNode,_T("CustomName"),
			EMPTY_STRING);

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnttt,pParentNode);

		NXMLSettings::AppendChildToParent(pColumnsNode,pParentNode);
		pColumnsNode->Release();
		pColumnsNode = NULL;

		NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsntt,pParentNode);

		pParentNode->Release();
		pParentNode = NULL;
	}

	SysFreeString(bstr_wsntt);
	SysFreeString(bstr_wsnttt);
}

int Explorerplusplus::LoadColumnFromXML(MSXML2::IXMLDOMNode *pNode,std::list<Column_t> *pColumns)
{
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pAttributeNode = NULL;
	Column_t					Column;
	BSTR						bstrName;
	BSTR						bstrValue;
	TCHAR						szWidth[32];
	HRESULT						hr;
	long						nAttributeNodes;
	int							iColumnType = -1;
	long						i = 0;

	pColumns->clear();

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
					Column.id = ColumnData[j].id;

					Column.bChecked	= NXMLSettings::DecodeBoolValue(bstrValue);

					pColumns->push_back(Column);
					break;
				}
				else if(lstrcmp(bstrName,szWidth) == 0)
				{
					if(pColumns->size() > 0)
					{
						pColumns->back().iWidth = NXMLSettings::DecodeIntValue(bstrValue);
					}

					break;
				}
			}
		}
	}

	return iColumnType;
}

int Explorerplusplus::LoadBookmarksFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	BSTR						bstr = NULL;
	HRESULT						hr;

	if(!pXMLDom)
		goto clean;

	bstr = SysAllocString(L"//Bookmark");
	hr = pXMLDom->selectSingleNode(bstr,&pNode);

	if(hr == S_OK)
	{
		/* TODO: Load bookmarks. */
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();

	return 0;
}

void Explorerplusplus::SaveBookmarksToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement		*pe = NULL;
	BSTR						bstr_wsnt = SysAllocString(L"\n\t");
	BSTR						bstr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"Bookmarks");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = NULL;

	/* TODO: Save bookmarks. */

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe, pRoot);
	pe->Release();
	pe = NULL;

	SysFreeString(bstr_wsnt);
}

int Explorerplusplus::LoadDefaultColumnsFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	BSTR						bstr = NULL;
	std::list<Column_t>			ColumnSet;
	long						length;
	int							iColumnType;

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

		for(long i = 0; i < length; i++)
		{
			pNodes->get_item(i, &pNode);

			iColumnType = LoadColumnFromXML(pNode,&ColumnSet);

			switch(iColumnType)
			{
			case COLUMN_TYPE_GENERIC:
				m_RealFolderColumnList = ColumnSet;
				break;

			case COLUMN_TYPE_MYCOMPUTER:
				m_MyComputerColumnList = ColumnSet;
				break;

			case COLUMN_TYPE_CONTROLPANEL:
				m_ControlPanelColumnList = ColumnSet;
				break;

			case COLUMN_TYPE_RECYCLEBIN:
				m_RecycleBinColumnList = ColumnSet;
				break;

			case COLUMN_TYPE_PRINTERS:
				m_PrintersColumnList = ColumnSet;
				break;

			case COLUMN_TYPE_NETWORK:
				m_NetworkConnectionsColumnList = ColumnSet;
				break;

			case COLUMN_TYPE_NETWORKPLACES:
				m_MyNetworkPlacesColumnList = ColumnSet;
				break;
			}

			pNode->Release();
			pNode = NULL;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();

	return 0;
}

void Explorerplusplus::SaveDefaultColumnsToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement	*pColumnsNode = NULL;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");
	BSTR					bstr;

	bstr = SysAllocString(L"DefaultColumns");
	pXMLDom->createElement(bstr,&pColumnsNode);
	SysFreeString(bstr);
	bstr = NULL;

	SaveDefaultColumnsToXMLInternal(pXMLDom,pColumnsNode);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pColumnsNode);
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	NXMLSettings::AppendChildToParent(pColumnsNode,pRoot);
	pColumnsNode->Release();
	pColumnsNode = NULL;
}

void Explorerplusplus::SaveDefaultColumnsToXMLInternal(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pColumnsNode)
{
	int DEFAULT_INDENT = 2;

	SaveColumnToXML(pXMLDom,pColumnsNode,&m_RealFolderColumnList,_T("Generic"),DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode,&m_MyComputerColumnList,_T("MyComputer"),DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode,&m_ControlPanelColumnList,_T("ControlPanel"),DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode,&m_RecycleBinColumnList,_T("RecycleBin"),DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode,&m_PrintersColumnList,_T("Printers"),DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode,&m_NetworkConnectionsColumnList,_T("Network"),DEFAULT_INDENT);
	SaveColumnToXML(pXMLDom,pColumnsNode,&m_MyNetworkPlacesColumnList,_T("NetworkPlaces"),DEFAULT_INDENT);
}

void Explorerplusplus::SaveColumnToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pColumnsNode, std::list<Column_t> *pColumns,
const TCHAR *szColumnSet, int iIndent)
{
	MSXML2::IXMLDOMElement		*pColumnNode = NULL;
	std::list<Column_t>::iterator	itr;
	TCHAR						*pszColumnSaveName = NULL;
	WCHAR						wszIndent[128];
	TCHAR						szWidth[32];
	BSTR						bstr_indent = SysAllocString(L"\n\t\t\t\t");
	int							i = 0;

	StringCchPrintf(wszIndent,SIZEOF_ARRAY(wszIndent),L"\n");

	for(i = 0;i < iIndent;i++)
		StringCchCat(wszIndent,SIZEOF_ARRAY(wszIndent),L"\t");

	bstr_indent = SysAllocString(wszIndent);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_indent,pColumnsNode);
	NXMLSettings::CreateElementNode(pXMLDom,&pColumnNode,pColumnsNode,_T("Column"),szColumnSet);

	for(itr = pColumns->begin();itr != pColumns->end();itr++)
	{
		for(i = 0;i < sizeof(ColumnData) / sizeof(ColumnData[0]);i++)
		{
			if(ColumnData[i].id == itr->id)
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

void Explorerplusplus::SaveWindowPositionToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement	*pWndPosNode = NULL;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");
	BSTR					bstr;

	bstr = SysAllocString(L"WindowPosition");
	pXMLDom->createElement(bstr,&pWndPosNode);
	SysFreeString(bstr);
	bstr = NULL;

	SaveWindowPositionToXMLInternal(pXMLDom,pWndPosNode);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pWndPosNode);
	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	NXMLSettings::AppendChildToParent(pWndPosNode,pRoot);
	pWndPosNode->Release();
	pWndPosNode = NULL;
}

void Explorerplusplus::SaveWindowPositionToXMLInternal(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pWndPosNode)
{
	MSXML2::IXMLDOMElement	*pParentNode = NULL;
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
	pParentNode = NULL;
}

void Explorerplusplus::LoadToolbarInformationFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
	MSXML2::IXMLDOMNode			*pChildNode = NULL;
	BSTR						bstrName;
	BSTR						bstrValue;
	BSTR						bstr = NULL;
	HRESULT						hr;
	long						length;
	long						lChildNodes;
	long						j = 0;

	if(pXMLDom == NULL)
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
			pNode = NULL;
		}
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();
}

void Explorerplusplus::SaveToolbarInformationToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement	*pe = NULL;
	BSTR					bstr = NULL;
	BSTR					bstr_wsnt = SysAllocString(L"\n\t");

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"Toolbars");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = NULL;

	SaveToolbarInformationToXMLnternal(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe,pRoot);
	pe->Release();
	pe = NULL;
}

void Explorerplusplus::SaveToolbarInformationToXMLnternal(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pe)
{
	MSXML2::IXMLDOMElement	*pParentNode = NULL;
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
		pParentNode = NULL;
	}

	SysFreeString(bstr_wsntt);
	SysFreeString(bstr_wsnttt);
}

void Explorerplusplus::LoadApplicationToolbarFromXML(MSXML2::IXMLDOMDocument *pXMLDom)
{
	MSXML2::IXMLDOMNodeList		*pNodes = NULL;
	MSXML2::IXMLDOMNode			*pNode = NULL;
	BSTR						bstr = NULL;
	HRESULT						hr;

	if(!pXMLDom)
		goto clean;

	bstr = SysAllocString(L"//ApplicationButton");
	hr = pXMLDom->selectSingleNode(bstr,&pNode);

	if(hr == S_OK)
	{
		CApplicationToolbarPersistentSettings::GetInstance().LoadXMLSettings(pNode);
	}

clean:
	if (bstr) SysFreeString(bstr);
	if (pNodes) pNodes->Release();
	if (pNode) pNode->Release();

	return;
}

void Explorerplusplus::SaveApplicationToolbarToXML(MSXML2::IXMLDOMDocument *pXMLDom,
MSXML2::IXMLDOMElement *pRoot)
{
	MSXML2::IXMLDOMElement		*pe = NULL;
	BSTR						bstr_wsnt = SysAllocString(L"\n\t");
	BSTR						bstr;

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pRoot);

	bstr = SysAllocString(L"ApplicationToolbar");
	pXMLDom->createElement(bstr,&pe);
	SysFreeString(bstr);
	bstr = NULL;

	CApplicationToolbarPersistentSettings::GetInstance().SaveXMLSettings(pXMLDom,pe);

	NXMLSettings::AddWhiteSpaceToNode(pXMLDom,bstr_wsnt,pe);

	NXMLSettings::AppendChildToParent(pe, pRoot);
	pe->Release();
	pe = NULL;

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
void Explorerplusplus::MapAttributeToValue(MSXML2::IXMLDOMNode *pNode,
WCHAR *wszName,WCHAR *wszValue)
{
	unsigned char	szName[512];
	unsigned long	uNameHash;

	WideCharToMultiByte(CP_ACP,0,wszName,-1,(LPSTR)szName,
		SIZEOF_ARRAY(szName),NULL,NULL);

	uNameHash = hash_setting(szName);

	switch(uNameHash)
	{
	case HASH_ALLOWMULTIPLEINSTANCES:
		m_bAllowMultipleInstances = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ALWAYSOPENINNEWTAB:
		m_bAlwaysOpenNewTab = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ALWAYSSHOWTABBAR:
		m_bAlwaysShowTabBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_AUTOARRANGEGLOBAL:
		m_bAutoArrangeGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CHECKBOXSELECTION:
		m_bCheckBoxSelection = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CLOSEMAINWINDOWONTABCLOSE:
		m_bCloseMainWindowOnTabClose = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_CONFIRMCLOSETABS:
		m_bConfirmCloseTabs = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DISABLEFOLDERSIZENETWORKREMOVABLE:
		m_bDisableFolderSizesNetworkRemovable = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_DISPLAYCENTRECOLOR:
		m_DisplayWindowCentreColor = NXMLSettings::ReadXMLColorData2(pNode);
		m_DisplayWindowCentreColor = NXMLSettings::ReadXMLColorData2(pNode);
		break;

	case HASH_DISPLAYFONT:
		m_DisplayWindowFont = NXMLSettings::ReadXMLFontData(pNode);
		break;

	case HASH_DISPLAYSURROUNDCOLOR:
		m_DisplayWindowSurroundColor = NXMLSettings::ReadXMLColorData2(pNode);
		break;

	case HASH_DISPLAYTEXTCOLOR:
		m_DisplayWindowTextColor = NXMLSettings::ReadXMLColorData(pNode);
		break;

	case HASH_DISPLAYWINDOWHEIGHT:
		m_DisplayWindowHeight = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_DOUBLECLICKTABCLOSE:
		m_bDoubleClickTabClose = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_EXTENDTABCONTROL:
		m_bExtendTabControl = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_FORCESAMETABWIDTH:
		m_bForceSameTabWidth = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_FORCESIZE:
		m_bForceSize = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HANDLEZIPFILES:
		m_bHandleZipFiles = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HIDELINKEXTENSIONGLOBAL:
		m_bHideLinkExtensionGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_HIDESYSTEMFILESGLOBAL:
		m_bHideSystemFilesGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_INSERTSORTED:
		m_bInsertSorted = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_LANGUAGE:
		m_Language = NXMLSettings::DecodeIntValue(wszValue);
		m_bLanguageLoaded = TRUE;
		break;

	case HASH_LARGETOOLBARICONS:
		m_bLargeToolbarIcons = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_LASTSELECTEDTAB:
		m_iLastSelectedTab = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_LOCKTOOLBARS:
		m_bLockToolbars = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_NEXTTOCURRENT:
		m_bOpenNewTabNextToCurrent = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ONECLICKACTIVATE:
		m_bOneClickActivate = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_ONECLICKACTIVATEHOVERTIME:
		m_OneClickActivateHoverTime = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_OVERWRITEEXISTINGFILESCONFIRMATION:
		m_bOverwriteExistingFilesConfirmation = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_PLAYNAVIGATIONSOUND:
		m_bPlayNavigationSound = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_REPLACEEXPLORERMODE:
		m_ReplaceExplorerMode = static_cast<NDefaultFileManager::ReplaceExplorerModes_t>(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SHOWADDRESSBAR:
		m_bShowAddressBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWAPPLICATIONTOOLBAR:
		m_bShowApplicationToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWBOOKMARKSTOOLBAR:
		m_bShowBookmarksToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWDRIVESTOOLBAR:
		m_bShowDrivesToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWDISPLAYWINDOW:
		m_bShowDisplayWindow = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWEXTENSIONS:
		m_bShowExtensionsGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFILEPREVIEWS:
		m_bShowFilePreviews = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFOLDERS:
		m_bShowFolders = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFOLDERSIZES:
		m_bShowFolderSizes = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFRIENDLYDATES:
		m_bShowFriendlyDatesGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWFULLTITLEPATH:
		m_bShowFullTitlePath = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWGRIDLINESGLOBAL:
		m_bShowGridlinesGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWHIDDENGLOBAL:
		m_bShowHiddenGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWINFOTIPS:
		m_bShowInfoTips = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWINGROUPSGLOBAL:
		m_bShowInGroupsGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWPRIVILEGETITLEBAR:
		m_bShowPrivilegeLevelInTitleBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWSTATUSBAR:
		m_bShowStatusBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTABBARATBOTTOM:
		m_bShowTabBarAtBottom = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTASKBARTHUMBNAILS:
		m_bShowTaskbarThumbnails = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWTOOLBAR:
		m_bShowMainToolbar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SHOWUSERNAMETITLEBAR:
		m_bShowUserNameInTitleBar = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_SIZEDISPLAYFOMRAT:
		m_SizeDisplayFormat = static_cast<SizeDisplayFormat_t>(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SORTASCENDINGGLOBAL:
		m_bSortAscendingGlobal = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_STARTUPMODE:
		m_StartupMode = static_cast<StartupMode_t>(NXMLSettings::DecodeIntValue(wszValue));
		break;

	case HASH_SYNCHRONIZETREEVIEW:
		m_bSynchronizeTreeview = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TVAUTOEXPAND:
		m_bTVAutoExpandSelected = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_USEFULLROWSELECT:
		m_bUseFullRowSelect = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TOOLBARSTATE:
		{
			MSXML2::IXMLDOMNode	*pChildNode = NULL;
			MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
			ToolbarButton_t	tb;
			BSTR		bstrName;
			BSTR		bstrValue;

			m_tbInitial.clear();

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

				/* TODO: Replace hardcoded strings. */
				if(lstrcmpi(bstrValue,L"Separator") == 0)
					tb.iItemID = TOOLBAR_SEPARATOR;
				else if(lstrcmpi(bstrValue,L"Back") == 0)
					tb.iItemID = TOOLBAR_BACK;
				else if(lstrcmpi(bstrValue,L"Forward") == 0)
					tb.iItemID = TOOLBAR_FORWARD;
				else if(lstrcmpi(bstrValue,L"Up") == 0)
					tb.iItemID = TOOLBAR_UP;
				else if(lstrcmpi(bstrValue,L"Folders") == 0)
					tb.iItemID = TOOLBAR_FOLDERS;
				else if(lstrcmpi(bstrValue,L"Copy To") == 0)
					tb.iItemID = TOOLBAR_COPYTO;
				else if(lstrcmpi(bstrValue,L"Move To") == 0)
					tb.iItemID = TOOLBAR_MOVETO;
				else if(lstrcmpi(bstrValue,L"New Folder") == 0)
					tb.iItemID = TOOLBAR_NEWFOLDER;
				else if(lstrcmpi(bstrValue,L"Copy") == 0)
					tb.iItemID = TOOLBAR_COPY;
				else if(lstrcmpi(bstrValue,L"Cut") == 0)
					tb.iItemID = TOOLBAR_CUT;
				else if(lstrcmpi(bstrValue,L"Paste") == 0)
					tb.iItemID = TOOLBAR_PASTE;
				else if(lstrcmpi(bstrValue,L"Delete") == 0)
					tb.iItemID = TOOLBAR_DELETE;
				else if(lstrcmpi(bstrValue,L"Delete Permanently") == 0)
					tb.iItemID = TOOLBAR_DELETEPERMANENTLY;
				else if(lstrcmpi(bstrValue,L"Views") == 0)
					tb.iItemID = TOOLBAR_VIEWS;
				else if(lstrcmpi(bstrValue,L"Search") == 0)
					tb.iItemID = TOOLBAR_SEARCH;
				else if(lstrcmpi(bstrValue,L"Properties") == 0)
					tb.iItemID = TOOLBAR_PROPERTIES;
				else if(lstrcmpi(bstrValue,L"Refresh") == 0)
					tb.iItemID = TOOLBAR_REFRESH;
				else if(lstrcmpi(bstrValue,L"Bookmark the current tab") == 0)
					tb.iItemID = TOOLBAR_ADDBOOKMARK;
				else if(lstrcmpi(bstrValue,L"Organize Bookmarks") == 0)
					tb.iItemID = TOOLBAR_ORGANIZEBOOKMARKS;
				else if(lstrcmpi(bstrValue,L"Create a new tab") == 0)
					tb.iItemID = TOOLBAR_NEWTAB;
				else if(lstrcmpi(bstrValue,L"Open Command Prompt") == 0)
					tb.iItemID = TOOLBAR_OPENCOMMANDPROMPT;

				m_tbInitial.push_back(tb);
			}
		}
		break;

	case HASH_TREEVIEWDELAYENABLED:
		m_bTreeViewDelayEnabled = NXMLSettings::DecodeBoolValue(wszValue);
		break;

	case HASH_TREEVIEWWIDTH:
		m_TreeViewWidth = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_VIEWMODEGLOBAL:
		m_ViewModeGlobal = NXMLSettings::DecodeIntValue(wszValue);
		break;

	case HASH_POSITION:
		{
			MSXML2::IXMLDOMNode	*pChildNode = NULL;
			MSXML2::IXMLDOMNamedNodeMap	*am = NULL;
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
		StringCchCopy(m_DefaultTabDirectory,SIZEOF_ARRAY(m_DefaultTabDirectory),wszValue);
		break;

	case HASH_INFOTIPTYPE:
		m_InfoTipType = static_cast<InfoTipType_t>(NXMLSettings::DecodeIntValue(wszValue));
		break;
	}
}

void Explorerplusplus::MapTabAttributeValue(WCHAR *wszName,WCHAR *wszValue,
InitialSettings_t *pSettings,TabInfo_t *pTabInfo)
{
	if(lstrcmp(wszName,L"ApplyFilter") == 0)
	{
		pSettings->bApplyFilter = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"AutoArrange") == 0)
	{
		pSettings->bAutoArrange = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"Filter") == 0)
	{
		StringCchCopy(pSettings->szFilter,SIZEOF_ARRAY(pSettings->szFilter),
			wszValue);
	}
	else if(lstrcmp(wszName,L"FilterCaseSensitive") == 0)
	{
		pSettings->bFilterCaseSensitive = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"ShowGridlines") == 0)
	{
		pSettings->bGridlinesActive = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"ShowHidden") == 0)
	{
		pSettings->bShowHidden = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"ShowInGroups") == 0)
	{
		pSettings->bShowInGroups = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"SortAscending") == 0)
	{
		pSettings->bSortAscending = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"SortMode") == 0)
	{
		pSettings->SortMode = NXMLSettings::DecodeIntValue(wszValue);
	}
	else if(lstrcmp(wszName,L"ViewMode") == 0)
	{
		pSettings->ViewMode = NXMLSettings::DecodeIntValue(wszValue);
	}
	else if(lstrcmp(wszName,L"Locked") == 0)
	{
		pTabInfo->bLocked = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"AddressLocked") == 0)
	{
		pTabInfo->bAddressLocked = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"UseCustomName") == 0)
	{
		pTabInfo->bUseCustomName = NXMLSettings::DecodeBoolValue(wszValue);
	}
	else if(lstrcmp(wszName,L"CustomName") == 0)
	{
		StringCchCopy(pTabInfo->szName,
			SIZEOF_ARRAY(pTabInfo->szName),wszValue);
	}
}

Explorerplusplus::CLoadSaveXML::CLoadSaveXML(Explorerplusplus *pContainer,BOOL bLoad) :
m_pContainer(pContainer),
m_bLoad(bLoad)
{
	if(bLoad)
	{
		/* Initialize the load environment (namely,
		load the configuration file). */
		InitializeLoadEnvironment();
	}
	else
	{
		/* Initialize the save environment. */
		InitializeSaveEnvironment();
	}
}

Explorerplusplus::CLoadSaveXML::~CLoadSaveXML()
{
	if(m_bLoad)
		ReleaseLoadEnvironment();
	else
		ReleaseSaveEnvironment();
}

void Explorerplusplus::CLoadSaveXML::InitializeLoadEnvironment()
{
	TCHAR szConfigFile[MAX_PATH];
	VARIANT_BOOL status;
	VARIANT var;

	m_bLoadedCorrectly = FALSE;

	m_pXMLDom = NXMLSettings::DomFromCOM();

	if(!m_pXMLDom)
		goto clean;

	GetProcessImageName(GetCurrentProcessId(),szConfigFile,SIZEOF_ARRAY(szConfigFile));
	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile,NExplorerplusplus::XML_FILENAME);

	var = NXMLSettings::VariantString(NExplorerplusplus::XML_FILENAME);
	m_pXMLDom->load(var,&status);

	if(status != VARIANT_TRUE)
		goto clean;

	m_bLoadedCorrectly = TRUE;

clean:
	if(&var) VariantClear(&var);

	return;
}

void Explorerplusplus::CLoadSaveXML::ReleaseLoadEnvironment()
{
	if(m_bLoadedCorrectly)
	{
		m_pXMLDom->Release();
		m_pXMLDom = NULL;
	}
}

void Explorerplusplus::CLoadSaveXML::InitializeSaveEnvironment()
{
	MSXML2::IXMLDOMProcessingInstruction	*pi = NULL;
	MSXML2::IXMLDOMComment					*pc = NULL;
	BSTR									bstr = NULL;
	BSTR									bstr1 = NULL;
	BSTR									bstr_wsnt = SysAllocString(L"\n\t");

	m_pXMLDom = NXMLSettings::DomFromCOM();

	if(!m_pXMLDom)
		goto clean;

	/* Insert the XML header. */
	bstr = SysAllocString(L"xml");
	bstr1 = SysAllocString(L"version='1.0'");
	m_pXMLDom->createProcessingInstruction(bstr,bstr1, &pi);
	NXMLSettings::AppendChildToParent(pi, m_pXMLDom);

	pi->Release();
	pi = NULL;
	SysFreeString(bstr);
	bstr = NULL;
	SysFreeString(bstr1);
	bstr1 = NULL;

	/* Short header comment, explaining file purpose. */
	bstr = SysAllocString(L" Preference file for Explorer++ ");
	m_pXMLDom->createComment(bstr, &pc);
	NXMLSettings::AppendChildToParent(pc, m_pXMLDom);
	SysFreeString(bstr);
	bstr = NULL;
	pc->Release();
	pc = NULL;

	/* Create the root element. CANNOT use '+' signs
	within the element name. */
	bstr = SysAllocString(L"ExplorerPlusPlus");
	m_pXMLDom->createElement(bstr,&m_pRoot);
	SysFreeString(bstr);
	bstr = NULL;

	NXMLSettings::AppendChildToParent(m_pRoot,m_pXMLDom);

	NXMLSettings::AddWhiteSpaceToNode(m_pXMLDom, bstr_wsnt, m_pRoot);

clean:
	if (bstr) SysFreeString(bstr);
	if (bstr1) SysFreeString(bstr1);

	if (pi) pi->Release();
	if (pc) pc->Release();
}

void Explorerplusplus::CLoadSaveXML::ReleaseSaveEnvironment()
{
	HANDLE	hProcess;
	TCHAR	szConfigFile[MAX_PATH];
	DWORD	dwProcessId;
	BSTR	bstr_wsn = SysAllocString(L"\n");
	BSTR	bstr = NULL;
	VARIANT	var;

	NXMLSettings::AddWhiteSpaceToNode(m_pXMLDom,bstr_wsn,m_pRoot);

	m_pXMLDom->get_xml(&bstr);

	/* To ensure the configuration file is saved to the same directory
	as the executable, determine the fully qualified path of the executable,
	then save the configuration file in that directory. */
	dwProcessId = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,dwProcessId);
	GetModuleFileNameEx(hProcess,NULL,szConfigFile,SIZEOF_ARRAY(szConfigFile));
	CloseHandle(hProcess);

	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile,NExplorerplusplus::XML_FILENAME);

	var = NXMLSettings::VariantString(szConfigFile);
	m_pXMLDom->save(var);

	m_pRoot->Release();
	m_pRoot = NULL;

	m_pXMLDom->Release();
	m_pXMLDom = NULL;
}

void Explorerplusplus::CLoadSaveXML::LoadGenericSettings()
{
	m_pContainer->LoadGenericSettingsFromXML(m_pXMLDom);
}

void Explorerplusplus::CLoadSaveXML::LoadBookmarks()
{
	m_pContainer->LoadBookmarksFromXML(m_pXMLDom);
}

int Explorerplusplus::CLoadSaveXML::LoadPreviousTabs()
{
	return m_pContainer->LoadTabSettingsFromXML(m_pXMLDom);
}

void Explorerplusplus::CLoadSaveXML::LoadDefaultColumns()
{
	m_pContainer->LoadDefaultColumnsFromXML(m_pXMLDom);
}

void Explorerplusplus::CLoadSaveXML::LoadApplicationToolbar()
{
	m_pContainer->LoadApplicationToolbarFromXML(m_pXMLDom);
}

void Explorerplusplus::CLoadSaveXML::LoadToolbarInformation()
{
	m_pContainer->LoadToolbarInformationFromXML(m_pXMLDom);
}

void Explorerplusplus::CLoadSaveXML::LoadColorRules()
{
	NColorRuleHelper::LoadColorRulesFromXML(m_pXMLDom,m_pContainer->m_ColorRules);
}

void Explorerplusplus::CLoadSaveXML::LoadDialogStates()
{
	m_pContainer->LoadDialogStatesFromXML(m_pXMLDom);
}

void Explorerplusplus::CLoadSaveXML::SaveGenericSettings()
{
	m_pContainer->SaveGenericSettingsToXML(m_pXMLDom,m_pRoot);
}

void Explorerplusplus::CLoadSaveXML::SaveBookmarks()
{
	m_pContainer->SaveBookmarksToXML(m_pXMLDom,m_pRoot);
}

void Explorerplusplus::CLoadSaveXML::SaveTabs()
{
	m_pContainer->SaveTabSettingsToXML(m_pXMLDom,m_pRoot);
}

void Explorerplusplus::CLoadSaveXML::SaveDefaultColumns()
{
	m_pContainer->SaveDefaultColumnsToXML(m_pXMLDom,m_pRoot);
}

void Explorerplusplus::CLoadSaveXML::SaveApplicationToolbar()
{
	m_pContainer->SaveApplicationToolbarToXML(m_pXMLDom,m_pRoot);
}

void Explorerplusplus::CLoadSaveXML::SaveToolbarInformation()
{
	m_pContainer->SaveToolbarInformationToXML(m_pXMLDom,m_pRoot);
}

void Explorerplusplus::CLoadSaveXML::SaveColorRules()
{
	NColorRuleHelper::SaveColorRulesToXML(m_pXMLDom,m_pRoot,m_pContainer->m_ColorRules);
}

void Explorerplusplus::CLoadSaveXML::SaveDialogStates()
{
	m_pContainer->SaveDialogStatesToXML(m_pXMLDom,m_pRoot);
}
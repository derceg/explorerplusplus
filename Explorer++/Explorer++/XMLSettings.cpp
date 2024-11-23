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
#include "../Helper/Macros.h"
#include "../Helper/XMLSettings.h"
#include <wil/com.h>
#include <wil/resource.h>

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

	wil::com_ptr_nothrow<IXMLDOMElement> pParentNode;
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayCentreColor"));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		XMLSettings::EncodeIntValue(GetRValue(m_config->displayWindowCentreColor.get())));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		XMLSettings::EncodeIntValue(GetGValue(m_config->displayWindowCentreColor.get())));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		XMLSettings::EncodeIntValue(GetBValue(m_config->displayWindowCentreColor.get())));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayFont"));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Height"),
		XMLSettings::EncodeIntValue(m_config->displayWindowFont.get().lfHeight));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Width"),
		XMLSettings::EncodeIntValue(m_config->displayWindowFont.get().lfWidth));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Weight"),
		XMLSettings::EncodeIntValue(m_config->displayWindowFont.get().lfWeight));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Italic"),
		XMLSettings::EncodeBoolValue(m_config->displayWindowFont.get().lfItalic));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Underline"),
		XMLSettings::EncodeBoolValue(m_config->displayWindowFont.get().lfUnderline));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Strikeout"),
		XMLSettings::EncodeBoolValue(m_config->displayWindowFont.get().lfStrikeOut));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("Font"),
		m_config->displayWindowFont.get().lfFaceName);

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplaySurroundColor"));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		XMLSettings::EncodeIntValue(GetRValue(m_config->displayWindowSurroundColor.get())));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		XMLSettings::EncodeIntValue(GetGValue(m_config->displayWindowSurroundColor.get())));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		XMLSettings::EncodeIntValue(GetBValue(m_config->displayWindowSurroundColor.get())));

	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsntt.get(), pe.get());
	XMLSettings::CreateElementNode(pXMLDom, &pParentNode, pe.get(), _T("Setting"),
		_T("DisplayTextColor"));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("r"),
		XMLSettings::EncodeIntValue(GetRValue(m_config->displayWindowTextColor.get())));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("g"),
		XMLSettings::EncodeIntValue(GetGValue(m_config->displayWindowTextColor.get())));
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode.get(), _T("b"),
		XMLSettings::EncodeIntValue(GetBValue(m_config->displayWindowTextColor.get())));

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
		XMLSettings::EncodeBoolValue(m_config->showDisplayWindow.get()));
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
		CustomFontStorage::SaveToXml(pXMLDom, pParentNode.get(), *mainFont);
	}

	auto bstr_wsnt = wil::make_bstr_nothrow(L"\n\t");
	XMLSettings::AddWhiteSpaceToNode(pXMLDom, bstr_wsnt.get(), pe.get());

	XMLSettings::AppendChildToParent(pe.get(), pRoot);
}

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceHelper.h"
#include <glog/logging.h>
#include <wil/common.h>
#include <cstring>

namespace
{

// As per the documentation for the DLGTEMPLATEEX struct, this signature indicates that a dialog
// template is an extended template.
const WORD DIALOG_BOX_EXTENDED_SIGNATURE = 0xFFFF;

}

// Note that the type parameter isn't necessarily a string. For example, it may be set to RT_DIALOG,
// which is simply an integer with a cast. So, it's not valid to change that parameter to a const
// std::wstring&.
std::optional<std::vector<std::byte>> CopyResource(HINSTANCE resourceInstance, UINT resourceId,
	const WCHAR *type)
{
	HRSRC resourceInformationHandle =
		FindResource(resourceInstance, MAKEINTRESOURCE(resourceId), type);

	if (!resourceInformationHandle)
	{
		return std::nullopt;
	}

	DWORD resourceSize = SizeofResource(resourceInstance, resourceInformationHandle);

	if (resourceSize == 0)
	{
		return std::nullopt;
	}

	HGLOBAL resourceHandle = LoadResource(resourceInstance, resourceInformationHandle);

	if (!resourceHandle)
	{
		return std::nullopt;
	}

	const void *resourceData = LockResource(resourceHandle);

	if (!resourceData)
	{
		return std::nullopt;
	}

	std::vector<std::byte> copiedData(resourceSize);
	std::memcpy(copiedData.data(), resourceData, resourceSize);

	return copiedData;
}

UniqueDlgTemplateEx MakeRTLDialogTemplate(HINSTANCE resourceInstance, UINT dialogId)
{
	auto resourceData = CopyResource(resourceInstance, dialogId, RT_DIALOG);

	if (!resourceData)
	{
		return nullptr;
	}

	// The resource size should always be greater than the base size of the struct, since
	// additional, variable-length data will always be included. If this isn't the case, something
	// has gone wrong and it's not safe to access the base members of the struct.
	CHECK_GT(resourceData->size(), sizeof(DLGTEMPLATEEX));

	// The only reason this struct is allocated and the data is copied into it is to make it
	// possible to return typed data, rather than an untyped set of bytes. This struct is of
	// variable size, which is the reason for the use of ::operator new.
	UniqueDlgTemplateEx dialogTemplateEx(
		static_cast<DLGTEMPLATEEX *>(::operator new(resourceData->size(), std::nothrow)));

	if (!dialogTemplateEx)
	{
		return nullptr;
	}

	std::memcpy(dialogTemplateEx.get(), resourceData->data(), resourceData->size());

	// All dialog resources are currently of the type DIALOGEX. Therefore, the signature here should
	// always indicate that this dialog is an extended dialog box template.
	CHECK_EQ(dialogTemplateEx->signature, DIALOG_BOX_EXTENDED_SIGNATURE);

	WI_SetFlag(dialogTemplateEx->exStyle, WS_EX_LAYOUTRTL);

	return dialogTemplateEx;
}

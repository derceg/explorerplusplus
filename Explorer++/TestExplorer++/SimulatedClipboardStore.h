// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ClipboardStore.h"
#include "../Helper/WinRTBaseWrapper.h"

class SimulatedClipboardDataObject;

// Using the system clipboard in a unit test isn't ideal, for a few reasons:
//
// - Other applications can sometimes have the clipboard open, causing the test to intermittently
//   fail.
// - It can be hard to ensure that the clipboard remains open for the duration of the test. The code
//   being tested might open the clipboard, write data, then close the clipboard, making it hard for
//   the unit test to guarantee that no other application will write data before it can be read.
// - The clipboard could be non-empty, which could cause the test to succeed when it would otherwise
//   fail. For example, a test might check whether a certain clipboard format is available. That
//   check could tend to succeed if earlier tests have left data in that format on the clipboard. It
//   can be difficult to ensure that the clipboard is consistently cleared, especially since the
//   test might not have much direct interaction with the clipboard itself.
//
// This class can be used as the backing clipboard store in tests. Note that the class isn't
// designed to be a complete recreation of the system clipboard. Notably, this class doesn't model
// clipboard ownership at all.
class SimulatedClipboardStore : public ClipboardStore
{
public:
	SimulatedClipboardStore();
	~SimulatedClipboardStore();

	bool Open() override;
	bool Close() override;

	bool IsDataAvailable(UINT format) const override;
	wil::unique_hglobal GetData(UINT format) const override;
	bool SetData(UINT format, wil::unique_hglobal global) override;

	wil::com_ptr_nothrow<IDataObject> GetDataObject() const override;
	bool SetDataObject(IDataObject *dataObject) override;
	bool IsDataObjectCurrent(IDataObject *dataObject) const override;
	bool FlushDataObject() override;

	bool Clear() override;

private:
	enum class DelegateType
	{
		// Indicates the delegate is internal to the clipboard itself and is used to hold data
		// assigned via SetData().
		Internal,

		// Indicates the delegate has been supplied via a call to SetDataObject(). In this case,
		// it's not possible to assign further data to the object.
		External
	};

	void SetExternalDelegate(IDataObject *externalDelegate);
	void ResetDelegate();
	static FORMATETC GetFormatEtc(UINT format);

	// This is just a wrapper. All the clipboard data is contained within the current delegate
	// that's assigned.
	const winrt::com_ptr<SimulatedClipboardDataObject> m_dataObject;

	DelegateType m_delegateType;
};

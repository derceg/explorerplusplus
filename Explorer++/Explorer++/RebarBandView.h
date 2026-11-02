// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <optional>

// Represents a view that can be added to a rebar.
class RebarBandView
{
public:
	using SizeRequirementsChangedSignal = boost::signals2::signal<void()>;

	virtual ~RebarBandView() = default;

	virtual HWND GetHWND() const = 0;
	virtual int GetHeight() const = 0;

	// The width and height of a view are treated differently. The height will always be a required
	// minimum. That is, the view needs a certain height in order to display correctly and if not
	// provided that height, the view either won't display or will be cut off.
	//
	// The width is different, in that the view can be shown with a width below its ideal width. The
	// ideal width is the width required to display all content (e.g. all buttons in a toolbar).
	// Depending on the particular view being used, there may not be an ideal width.
	virtual std::optional<int> MaybeGetIdealWidth() const;

	// This will be triggered when the required size of a band changes. For example, when the
	// required height of the band changes. This also indicates that the ideal width may have
	// changed.
	boost::signals2::connection AddSizeRequirementsChangedObserver(
		const SizeRequirementsChangedSignal::slot_type &observer);

protected:
	void NotifySizeRequirementsChanged();

private:
	SizeRequirementsChangedSignal m_sizeRequirementsChangedSignal;
};

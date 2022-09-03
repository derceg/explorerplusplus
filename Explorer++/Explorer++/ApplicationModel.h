// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <memory>
#include <optional>
#include <vector>

namespace Applications
{

class Application;

class ApplicationModel
{
public:
	using ApplicationsList = std::vector<std::unique_ptr<Application>>;

	using ApplicationAddedSignal =
		boost::signals2::signal<void(Application *application, size_t index)>;
	using ApplicationUpdatedSignal = boost::signals2::signal<void(Application *application)>;
	using ApplicationRemovedSignal =
		boost::signals2::signal<void(const Application *application, size_t oldIndex)>;

	ApplicationModel() = default;
	~ApplicationModel();

	ApplicationModel(const ApplicationModel &) = delete;
	ApplicationModel(ApplicationModel &&) = delete;
	ApplicationModel &operator=(const ApplicationModel &) = delete;
	ApplicationModel &operator=(ApplicationModel &&) = delete;

	Application *AddApplication(std::unique_ptr<Application> application);
	Application *AddApplication(std::unique_ptr<Application> application, size_t index);

	void RemoveApplication(const Application *application);

	const ApplicationsList &GetApplications() const;
	std::optional<size_t> GetApplicationIndex(const Application *application) const;
	const Application *GetApplicationAtIndex(size_t index) const;

	boost::signals2::connection AddApplicationAddedObserver(
		const ApplicationAddedSignal::slot_type &observer);
	boost::signals2::connection AddApplicationUpdatedObserver(
		const ApplicationUpdatedSignal::slot_type &observer);
	boost::signals2::connection AddApplicationRemovedObserver(
		const ApplicationRemovedSignal::slot_type &observer);

private:
	void OnApplicationUpdated(Application *application);

	ApplicationsList m_applications;

	ApplicationAddedSignal m_applicationAddedSignal;
	ApplicationUpdatedSignal m_applicationUpdatedSignal;
	ApplicationRemovedSignal m_applicationRemovedSignal;
};

}

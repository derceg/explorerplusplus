// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationModel.h"
#include "Application.h"
#include <cassert>

namespace Applications
{

ApplicationModel::~ApplicationModel() = default;

Application *ApplicationModel::AddApplication(std::unique_ptr<Application> application)
{
	return AddApplication(std::move(application), m_applications.size());
}

Application *ApplicationModel::AddApplication(std::unique_ptr<Application> application,
	size_t index)
{
	assert(index <= m_applications.size());

	application->AddUpdatedObserver(std::bind_front(&ApplicationModel::OnApplicationUpdated, this));

	auto *rawApplication = application.get();
	m_applications.insert(m_applications.begin() + index, std::move(application));
	m_applicationAddedSignal(rawApplication, index);

	return rawApplication;
}

void ApplicationModel::OnApplicationUpdated(Application *application)
{
	m_applicationUpdatedSignal(application);
}

void ApplicationModel::RemoveApplication(const Application *application)
{
	auto itr = std::find_if(m_applications.begin(), m_applications.end(),
		[application](const auto &currentEntry)
		{
			return currentEntry.get() == application;
		});

	if (itr == m_applications.end())
	{
		return;
	}

	std::unique_ptr<Application> ownedApplication = std::move(*itr);
	size_t index = itr - m_applications.begin();

	m_applications.erase(itr);

	m_applicationRemovedSignal(ownedApplication.get(), index);
}

const ApplicationModel::ApplicationsList &ApplicationModel::GetApplications() const
{
	return m_applications;
}

std::optional<size_t> ApplicationModel::GetApplicationIndex(const Application *application) const
{
	auto itr = std::find_if(m_applications.begin(), m_applications.end(),
		[application](const auto &currentEntry)
		{
			return currentEntry.get() == application;
		});

	if (itr == m_applications.end())
	{
		return std::nullopt;
	}

	return itr - m_applications.begin();
}

const Application *ApplicationModel::GetApplicationAtIndex(size_t index) const
{
	if (index >= m_applications.size())
	{
		return nullptr;
	}

	return m_applications[index].get();
}

boost::signals2::connection ApplicationModel::AddApplicationAddedObserver(
	const ApplicationAddedSignal::slot_type &observer)
{
	return m_applicationAddedSignal.connect(observer);
}

boost::signals2::connection ApplicationModel::AddApplicationUpdatedObserver(
	const ApplicationUpdatedSignal::slot_type &observer)
{
	return m_applicationUpdatedSignal.connect(observer);
}

boost::signals2::connection ApplicationModel::AddApplicationRemovedObserver(
	const ApplicationRemovedSignal::slot_type &observer)
{
	return m_applicationRemovedSignal.connect(observer);
}

}

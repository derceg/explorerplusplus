// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace Applications
{

class Application;
class ApplicationModel;

bool operator==(const Application &first, const Application &second);

}

void BuildLoadSaveReferenceModel(Applications::ApplicationModel *model);

// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ColorRuleModel;
class ColorRule;

bool operator==(const ColorRule &first, const ColorRule &second);

void BuildLoadSaveReferenceModel(ColorRuleModel *model);

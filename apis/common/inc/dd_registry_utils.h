/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <set>
#include <string>
#include <vector>
#include <dd_settings_api.h>

namespace DevDriver
{
void GetRegistryPaths(std::set<std::string>* pRegistryPaths);

DD_RESULT EnumerateDriverRegistry(const std::string& rootKey,
                                  std::vector<DDSettingsRegistryInfo>& output);

DD_RESULT DeleteRegistrySetting(const std::string&            rootKey,
                                const DDSettingsRegistryInfo* pRegistrySetting);
}

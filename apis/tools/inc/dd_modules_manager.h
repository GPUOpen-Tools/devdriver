/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_api_registry_api.h>
#include <dd_modules_api.h>
#include <dd_common_api.h>
#include <dd_mutex.h>

#include <vector>
#include <string>
#include <filesystem>

namespace DevDriver
{

/// This class manages DevDriver modules.
class ModulesManager
{
private:
    DDApiRegistry*     m_pApiRegistry;

    std::string        m_modulesDir;
    std::vector<void*> m_dynamicModules;

    std::vector<DDModulesCallbacks*> m_modulesCallbacksImpls;
    Mutex                            m_modulesCallbacksMutex;

public:
    ModulesManager(DDApiRegistry* pApiRegistry, std::string&& modulesDir);

    DD_RESULT Initialize();

    DD_RESULT LoadDynamicModules();
    void UnloadDynamicModules();

    // Due to the complexity of dependency sorting, we currently fail initialization is any module fails
    // to initialize.
    DD_RESULT InitializeModules();

    void DestroyModules();

    void AddModulesCallbacks(DDModulesCallbacks* pCallbacks);

private:

    /// Load a module at the path \param modulePath. And extract the function by the name
    /// "DDModuleLoad_xxx" (xxx is the module filename without the extension) and invoke
    /// the function.
    void LoadDynamicModule(const std::filesystem::path& modulePath);

    ModulesManager(ModulesManager&& manager) = delete;
    ModulesManager(const ModulesManager& manager) = delete;
    ModulesManager& operator=(ModulesManager&& manager) = delete;
    ModulesManager& operator=(const ModulesManager& manager) = delete;
};

} // namespace DevDriver

/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_modules_manager.h>
#include <dd_assert.h>
#include <dd_logger_api.h>
#include <dd_result.h>

#include <filesystem>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <dlfcn.h>
#endif

// Prepend '##' to __VA_ARGS__ to swallow prior ',' when no argument is passed.
#define LOG_ERROR(fmt, ...) s_pLogger->Log(                \
                                s_pLogger->pInstance,      \
                                DD_LOG_LVL_ERROR,          \
                                "[DDModulesManager] " fmt, \
                                ## __VA_ARGS__)

#define LOG_INFO(fmt, ...) s_pLogger->Log(                \
                               s_pLogger->pInstance,      \
                               DD_LOG_LVL_INFO,           \
                               "[DDModulesManager] " fmt, \
                               ## __VA_ARGS__)

#define LOG_VERBOSE(fmt, ...) s_pLogger->Log(                \
                                  s_pLogger->pInstance,      \
                                  DD_LOG_LVL_VERBOSE,        \
                                  "[DDModulesManager] " fmt, \
                                  ## __VA_ARGS__)

namespace
{

DDLoggerApi* s_pLogger;

#ifdef _WIN32
std::string WCharToU8Str(const wchar_t* pWcStr)
{
    DD_ASSERT(pWcStr != nullptr);

    int u8StrSize = WideCharToMultiByte(CP_UTF8, 0, pWcStr, -1, NULL, 0, NULL, NULL);
    std::string u8str(u8StrSize, '\0');
    int bytesCopied = WideCharToMultiByte(
        CP_UTF8,
        0,
        pWcStr,
        -1,
        u8str.data(),
        u8StrSize,
        NULL,
        NULL);
    if (bytesCopied == 0)
    {
        LOG_ERROR("Failed to convert WCHAR string to UTF8. Windows Error: %i", GetLastError());
    }
    return u8str;
}
#endif

#ifdef _WIN32
void* LoadDynamicLib(const wchar_t* pPath)
{
    // pPath is always an absolute path from directory_iterator.
    // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR restricts search to the DLL's own directory + System32.
    void* pLib = LoadLibraryExW(pPath, nullptr,
                                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (pLib == nullptr)
    {
        std::string u8Path = WCharToU8Str(pPath);
        LOG_ERROR("Failed to Load dynamic lib at: %s. Windows error code: %i.", u8Path.data(), GetLastError());
    }
    return pLib;
}
#else
void* LoadDynamicLib(const char* pPath)
{
    void* pLib = dlopen(pPath, RTLD_LAZY);
    if (pLib == nullptr)
    {
        LOG_ERROR("Failed to load dynamic lib. Error: %s.", dlerror());
    }
    return pLib;
}
#endif

void UnloadDynamicLib(void* pLib)
{
    (void)pLib;
#ifdef _WIN32
    if (FreeLibrary(static_cast<HMODULE>(pLib)) == 0)
    {
        LOG_ERROR("Failed to unload dynamic library. Windows error code: %i.", GetLastError());
    }

#else
    if (dlclose(pLib) != 0)
    {
        LOG_ERROR("Failed to unload dynamic library. Error: %s.", dlerror());
    }
#endif
}

void* GetFunction(void* pLib, const char* pFnName)
{
#ifdef _WIN32
    FARPROC pFn = GetProcAddress(static_cast<HMODULE>(pLib), pFnName);
    if (pFn == nullptr)
    {
        LOG_ERROR("Failed to get function pointer. Windows error code: %i.", GetLastError());
    }
#else
    void* pFn = dlsym(pLib, pFnName);
    if (pFn == nullptr)
    {
        LOG_ERROR("Failed to get function pointer. Error: %s.", dlerror());
    }
#endif

    return reinterpret_cast<void*>(pFn);
}

using DDModuleLoad_FN = DD_RESULT (*)(DDApiRegistry* pApiRegistry);

DD_RESULT DDModulesApi_AddModulesCallbacks(DDModulesManagerInstance* pInstance, DDModulesCallbacks* pCallbacks)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    if ((pInstance == nullptr) || (pCallbacks == nullptr))
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }

    if (result == DD_RESULT_SUCCESS)
    {
        DevDriver::ModulesManager* pModulesManager = reinterpret_cast<DevDriver::ModulesManager*>(pInstance);
        pModulesManager->AddModulesCallbacks(pCallbacks);
    }

    return result;
}

} // anonymous namespace

namespace DevDriver
{

ModulesManager::ModulesManager(DDApiRegistry* pApiRegistry, std::string&& modulesDir)
    : m_pApiRegistry(pApiRegistry)
    , m_modulesDir(modulesDir)
{
}

DD_RESULT ModulesManager::Initialize()
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    m_dynamicModules.reserve(32);

    DDVersion modulesApiVersion {
        DD_MODULES_API_VERSION_MAJOR,
        DD_MODULES_API_VERSION_MINOR,
        DD_MODULES_API_VERSION_PATCH
    };

    DDModulesApi modulesApi {
        reinterpret_cast<DDModulesManagerInstance*>(this),
        DDModulesApi_AddModulesCallbacks
    };

    result = m_pApiRegistry->Get(
        m_pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion {
            DD_LOGGER_API_VERSION_MAJOR,
            DD_LOGGER_API_VERSION_MINOR,
            DD_LOGGER_API_VERSION_PATCH},
            reinterpret_cast<void**>(&s_pLogger));

    if (result == DD_RESULT_SUCCESS)
    {
        result = m_pApiRegistry->Add(
            m_pApiRegistry->pInstance,
            DD_MODULES_API_NAME,
            modulesApiVersion,
            &modulesApi,
            sizeof(modulesApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDModulesApi. DD_RESULT: %s", StringResult(result));
        }
    }

    return result;
}

DD_RESULT ModulesManager::LoadDynamicModules()
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    std::filesystem::path modulesDirPath;

    if (m_modulesDir.size() > 0)
    {
#if defined(_WIN32)
        DD_ASSERT(m_modulesDir.size() < (INT32_MAX / 2));
        int modulesDirWBufSize = static_cast<int>(m_modulesDir.size() * 2 + 1);
        wchar_t* pModulesDirWBuf = new wchar_t[modulesDirWBufSize];
        int modulesDirWSize = MultiByteToWideChar(
            CP_UTF8,
            0,
            m_modulesDir.c_str(),
            static_cast<int>(m_modulesDir.size()),
            pModulesDirWBuf,
            modulesDirWBufSize);

        if (modulesDirWSize > 0)
        {
            pModulesDirWBuf[modulesDirWSize] = '\0';
            modulesDirPath = pModulesDirWBuf;
        }
        else
        {
            DWORD err = GetLastError();
            if (err == ERROR_INSUFFICIENT_BUFFER)
            {
                result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
            }
            else
            {
                result = DD_RESULT_COMMON_INVALID_PARAMETER;
            }
            LOG_ERROR("Failed to convert UTF8 to UTF16. Windows error code: %i", err);
        }

        delete[] pModulesDirWBuf;
#else
        modulesDirPath = m_modulesDir;
#endif

        if (result == DD_RESULT_SUCCESS)
        {
            if (std::filesystem::exists(modulesDirPath))
            {
                for (auto const& entry : std::filesystem::directory_iterator{modulesDirPath})
                {
                    if (entry.is_regular_file())
                    {
                        LoadDynamicModule(entry.path());
                    }
                }
            }
            else
            {
                LOG_ERROR("Specified modules directory doesn't exist: %s.", m_modulesDir.c_str());
                result = DD_RESULT_COMMON_DOES_NOT_EXIST;
            }
        }
    }
    else
    {
        LOG_INFO("No modules directory set. Skip loading dynamic modules.");
    }

    return result;
}

void ModulesManager::UnloadDynamicModules()
{
    for (auto pModule : m_dynamicModules)
    {
        UnloadDynamicLib(pModule);
    }
}

void ModulesManager::AddModulesCallbacks(DDModulesCallbacks* pCallbacks)
{
    LockGuard lock(m_modulesCallbacksMutex);

    m_modulesCallbacksImpls.push_back(pCallbacks);
}

DD_RESULT ModulesManager::InitializeModules()
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    LockGuard lock(m_modulesCallbacksMutex);

    for (DDModulesCallbacks* pCallbacksImpl : m_modulesCallbacksImpls)
    {
        result = pCallbacksImpl->Initialize(pCallbacksImpl->pInstance);
        if (result != DD_RESULT_SUCCESS)
        {
            break;
        }
    }

    return result;
}

void ModulesManager::DestroyModules()
{
    LockGuard lock(m_modulesCallbacksMutex);

    for (DDModulesCallbacks* pCallbacksImpl : m_modulesCallbacksImpls)
    {
        pCallbacksImpl->Destroy(pCallbacksImpl->pInstance);
    }
}

void ModulesManager::LoadDynamicModule(const std::filesystem::path& modulePath)
{
    void* pModule = nullptr;

#ifdef _WIN32
    if (wcscmp(modulePath.extension().c_str(), L".dll") == 0)
    {
        pModule = LoadDynamicLib(modulePath.c_str());
    }
    else
    {
        std::string u8str = WCharToU8Str(modulePath.c_str());
        LOG_VERBOSE("Skip loading non-library file: %s", u8str.data());
    }
#else
    if (strcmp(modulePath.extension().c_str(), ".so") == 0)
    {
        pModule = LoadDynamicLib(modulePath.c_str());
    }
    else
    {
        LOG_VERBOSE("Skip loading non-library file: %s.", modulePath.c_str());
    }
#endif

    if (pModule)
    {
        std::string fnName("DDModuleLoad_");

#ifdef _WIN32
        // convert stem from wchar_t to char.
        std::filesystem::path stemPath = modulePath.stem();
        std::string u8str = WCharToU8Str(stemPath.c_str());
        fnName += u8str;
#else
        std::string stem = modulePath.stem();
        if (std::strncmp(stem.c_str(), "lib", 3) == 0)
        {
            fnName += stem.substr(3);
        }
        else
        {
            fnName += stem;
        }
#endif

        DDModuleLoad_FN pLoadFn = (DDModuleLoad_FN)GetFunction(pModule, fnName.c_str());
        if (pLoadFn)
        {
            DD_RESULT result = pLoadFn(m_pApiRegistry);
            if (result == DD_RESULT_SUCCESS)
            {
                m_dynamicModules.push_back(pModule);
            }
            else
            {
                UnloadDynamicLib(pModule);
            }
        }
    }
}

} // namespace DevDriver

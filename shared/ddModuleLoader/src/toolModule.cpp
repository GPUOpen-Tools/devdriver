/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <toolModule.h>

#include <inttypes.h>

/// Version of the ddModule API that's currently supported by ddTool
DD_STATIC_CONST DDApiVersion kRequiredModuleApiVersion = { DD_MODULE_API_MAJOR_VERSION,
                                                           DD_MODULE_API_MINOR_VERSION,
                                                           DD_MODULE_API_PATCH_VERSION };

/// Category for Loader logging
constexpr const char kModuleLoaderCateogry[] = "ddModuleLoader";

#define DD_MODULE_LOADER_LOG(logger, level, message) DD_API_LOG(logger, level, kModuleLoaderCateogry, message)
#define DD_MODULE_LOADER_LOGF(logger, level, fmt, ...)                                                                 \
    DD_API_LOGF(logger, level, kModuleLoaderCateogry, fmt, __VA_ARGS__)

using namespace DevDriver;

namespace DDTool
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ToolModule::ToolModule(
    LoggerUtil*              pLogger,
    const ApiAllocCallbacks& apiAllocCb,
    const DDModuleInterface* pInterface,
    const DynamicModuleInfo* pDynamicInfo)
    : m_pUserdata(nullptr), m_pInterface(pInterface), m_pLogger(pLogger)
{
    DD_ASSERT(pLogger != nullptr);
    DD_ASSERT(m_pInterface != nullptr);

    // Fill out the loader interface for this module
    m_loaderInterface.apiAllocCb.pfnAlloc  = apiAllocCb.pAllocCallback;
    m_loaderInterface.apiAllocCb.pfnFree   = apiAllocCb.pFreeCallback;
    m_loaderInterface.apiAllocCb.pUserdata = apiAllocCb.pUserdata;
    m_loaderInterface.logger               = pLogger->GetInfo();

    // Initialize the dynamic library path to an empty string
    m_dynamicLibraryPath[0] = '\0';

    // Transfer ownership of the module library to this object if it's valid
    // Only dynamic modules will have a library object.
    if (pDynamicInfo != nullptr)
    {
        Platform::Strncpy(m_dynamicLibraryPath, pDynamicInfo->pPath);
        m_moduleLibrary.Swap(pDynamicInfo->pLibrary);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ToolModule::~ToolModule() {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const DDModuleDescription& ToolModule::GetDescription() const
{
    DD_ASSERT(m_pInterface != nullptr);

    return m_pInterface->description;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DDModuleLoadedInfo ToolModule::GetModuleInfo()
{
    DDModuleLoadedInfo moduleInfo = {};

    moduleInfo.hContext    = reinterpret_cast<DDModuleContext>(this);
    moduleInfo.description = GetDescription();
    moduleInfo.pPath       = GetDynamicModulePath();

    return moduleInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Result ToolModule::CreateModuleSystemContext(
    const DDModuleSystemContextCreateInfo& createInfo,
    DDModuleSystemContext*                 phSystemContext)
{
    Result result = Result::Unavailable;

    if (HasSystemApi())
    {
        const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);

        const DD_RESULT moduleResult = pApi->pSystemContextApi->pfnCreateModuleSystemContext(
            &createInfo,
            phSystemContext);

        result = (moduleResult == DD_RESULT_SUCCESS) ? Result::Success : Result::Error;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::DestroyModuleSystemContext(DDModuleSystemContext hSystemContext)
{
    DD_ASSERT(HasSystemApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    pApi->pSystemContextApi->pfnDestroyModuleSystemContext(hSystemContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::HandleSystemEvent(
    DDModuleSystemContext  hContext,
    DD_MODULE_SYSTEM_EVENT eventId,
    const void*            pEventData,
    size_t                 eventDataSize)
{
    DD_ASSERT(HasSystemApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    pApi->pSystemContextApi->pfnHandleSystemEvent(hContext, eventId, pEventData, eventDataSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Result ToolModule::QueryModuleClientContextAllocInfo(DDModuleClientContextAllocInfo* pAllocInfo)
{
    Result result = Result::Unavailable;

    if (HasClientApi())
    {
        const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
        pApi->pClientContextApi->pfnQueryModuleClientContextAllocInfo(pAllocInfo);

        result = Result::Success;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT ToolModule::CreateModuleClientContext(
    const DDModuleSystemContextCreateInfo& systemCreateInfo,
    const DDModuleClientInfo&              clientInfo,
    DDModuleDataContext                    hDataContext,
    void*                                  pContextMemory)
{
    DD_ASSERT(HasClientApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);

    const DDModuleClientContextCreateInfo createInfo = GenerateClientCreateInfo(
        systemCreateInfo,
        clientInfo,
        hDataContext);

    return pApi->pClientContextApi->pfnCreateModuleClientContext(&createInfo, pContextMemory);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::DestroyModuleClientContext(DDModuleClientContext hContext)
{
    DD_ASSERT(HasClientApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    pApi->pClientContextApi->pfnDestroyModuleClientContext(hContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::HandleClientEvent(
    DDModuleClientContext  hContext,
    DD_MODULE_CLIENT_EVENT eventId,
    const void*            pEventData,
    size_t                 eventDataSize)
{
    DD_ASSERT(HasClientApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    pApi->pClientContextApi->pfnHandleClientEvent(hContext, eventId, pEventData, eventDataSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Result ToolModule::CreateDataContext(const void* pData, size_t dataSize, DDModuleDataContext* phDataContext)
{
    Result result = Result::Unavailable;

    if (HasDataApi())
    {
        DDModuleDataContextCreateInfo createInfo = {};
        createInfo.loader                        = m_loaderInterface;

        const DDModuleApi_0000* pApi         = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
        const DD_RESULT         moduleResult = pApi->pDataContextApi->pfnCreateDataContext(
            &createInfo,
            pData,
            dataSize,
            phDataContext);

        result = (moduleResult == DD_RESULT_SUCCESS) ? Result::Success : Result::Error;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::DestroyDataContext(DDModuleDataContext* phDataContext)
{
    DD_ASSERT(HasDataApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    pApi->pDataContextApi->pfnDestroyDataContext(phDataContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Result ToolModule::SerializeDataContext(DDModuleDataContext hDataContext, void* pData, size_t* pDataSize)
{
    DD_ASSERT(HasDataApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    return (pApi->pDataContextApi->pfnSerializeDataContext(hDataContext, pData, pDataSize) == DD_RESULT_SUCCESS) ?
               Result::Success :
               Result::Error;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT ToolModule::CreateConnectionContext(
    const DDModuleConnectionContextCreateInfo& createInfo,
    DDModuleConnectionContext*                 phContext)
{
    DD_ASSERT(HasConnectionApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    return pApi->pConnectionContextApi->pfnCreateConnectionContext(&createInfo, phContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::DestroyConnectionContext(DDModuleConnectionContext hContext)
{
    DD_ASSERT(HasConnectionApi());

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    pApi->pConnectionContextApi->pfnDestroyConnectionContext(hContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Result ToolModule::QueryModuleExtension(
    DDModuleExtensionId                extensionId,
    const DDApiVersion*                pRequiredVersion,
    const DDModuleExtensionInterface** ppExtensionInterface)
{
    DD_ASSERT(pRequiredVersion != nullptr);
    DD_ASSERT(ppExtensionInterface != nullptr);

    Result result = Result::Unavailable;

    const DDModuleApi_0000* pApi = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);

    // The extension API is optional so we need to check for nullptr here
    if (pApi->pfnQueryExtension != nullptr)
    {
        const DDModuleExtensionInterface* pExtInterface = pApi->pfnQueryExtension(extensionId);
        if (pExtInterface != nullptr)
        {
            DD_MODULE_LOADER_LOGF(
                *m_pLogger,
                DD_LOG_LEVEL_VERBOSE,
                "Required %s Module Extension API Version %u.%u.%u | %s Module Extension API Version %u.%u.%u | "
                "Extension Id 0x%" PRIx64,
                m_pInterface->description.pName,
                pRequiredVersion->major,
                pRequiredVersion->minor,
                pRequiredVersion->patch,
                m_pInterface->description.pName,
                pExtInterface->apiVersion.major,
                pExtInterface->apiVersion.minor,
                pExtInterface->apiVersion.patch,
                extensionId);

            // Check if the extension meets the caller's version requirements
            if (ddIsVersionCompatible(*pRequiredVersion, pExtInterface->apiVersion))
            {
                *ppExtensionInterface = pExtInterface;

                result = Result::Success;
            }
            else
            {
                result = Result::VersionMismatch;

                DD_MODULE_LOADER_LOGF(
                    *m_pLogger,
                    DD_LOG_LEVEL_WARN,
                    "Version mismatch when attempting to query extension api 0x%" PRIx64 " from module %s!",
                    extensionId,
                    m_pInterface->description.pName);
            }
        }
        else
        {
            DD_MODULE_LOADER_LOGF(
                *m_pLogger,
                DD_LOG_LEVEL_VERBOSE,
                "Unable to acquire extension api 0x%" PRIx64 " from module %s!",
                extensionId,
                m_pInterface->description.pName);
        }
    }
    else
    {
        DD_MODULE_LOADER_LOGF(
            *m_pLogger,
            DD_LOG_LEVEL_VERBOSE,
            "Extension apis not supported by module %s!",
            m_pInterface->description.pName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DDModuleClientContextCreateInfo ToolModule::GenerateClientCreateInfo(
    const DDModuleSystemContextCreateInfo& systemCreateInfo,
    const DDModuleClientInfo&              clientInfo,
    DDModuleDataContext                    hDataContext) const
{
    DDModuleClientContextCreateInfo createInfo = {};

    createInfo.loader       = m_loaderInterface;
    createInfo.connection   = systemCreateInfo.connection;
    createInfo.hDataContext = hDataContext;
    createInfo.clientInfo   = clientInfo;

    // Copy the system client data over from the system create info structure.
    constexpr size_t systemClientsSize = sizeof(createInfo.systemClients);
    Platform::Memcpy_s(createInfo.systemClients, systemClientsSize, systemCreateInfo.systemClients, systemClientsSize);

    return createInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ToolModule::HasSystemApi() const
{
    const DDModuleApi_0000* pApi                   = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    const bool              supportsSystemContexts = m_pInterface->description.flags.fields.supportsSystemContexts;

    if (supportsSystemContexts && (pApi->pSystemContextApi == nullptr))
    {
        DD_ASSERT_REASON("Interface reports supporting System contexts, but has no context api pointer!");
    }
    if ((supportsSystemContexts == false) && (pApi->pSystemContextApi != nullptr))
    {
        DD_ASSERT_REASON("Interface reports NOT supporting System contexts, but has context api pointer!");
    }

    return (supportsSystemContexts && (pApi->pSystemContextApi != nullptr));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ToolModule::HasClientApi() const
{
    const DDModuleApi_0000* pApi                   = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    const bool              supportsClientContexts = m_pInterface->description.flags.fields.supportsClientContexts;

    if (supportsClientContexts && (pApi->pClientContextApi == nullptr))
    {
        DD_ASSERT_REASON("Interface reports supporting Client contexts, but has no context api pointer!");
    }
    if ((supportsClientContexts == false) && (pApi->pClientContextApi != nullptr))
    {
        DD_ASSERT_REASON("Interface reports NOT supporting Client contexts, but has context api pointer!");
    }

    return (supportsClientContexts && (pApi->pClientContextApi != nullptr));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ToolModule::HasDataApi() const
{
    const DDModuleApi_0000* pApi                 = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    const bool              supportsDataContexts = m_pInterface->description.flags.fields.supportsDataContexts;

    if (supportsDataContexts && (pApi->pDataContextApi == nullptr))
    {
        DD_ASSERT_REASON("Interface reports supporting Data contexts, but has no context api pointer!");
    }
    if ((supportsDataContexts == false) && (pApi->pDataContextApi != nullptr))
    {
        DD_ASSERT_REASON("Interface reports NOT supporting Data contexts, but has context api pointer!");
    }

    return (supportsDataContexts && (pApi->pDataContextApi != nullptr));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ToolModule::HasConnectionApi() const
{
    const DDModuleApi_0000* pApi          = reinterpret_cast<const DDModuleApi_0000*>(m_pInterface->pApi);
    const bool supportsConnectionContexts = m_pInterface->description.flags.fields.supportsConnectionContexts;

    if (supportsConnectionContexts && (pApi->pConnectionContextApi == nullptr))
    {
        DD_ASSERT_REASON("Interface reports supporting Connection contexts, but has no context api pointer!");
    }
    if ((supportsConnectionContexts == false) && (pApi->pConnectionContextApi != nullptr))
    {
        DD_ASSERT_REASON("Interface reports NOT supporting Connection contexts, but has context api pointer!");
    }

    return (supportsConnectionContexts && (pApi->pConnectionContextApi != nullptr));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT ToolModule::ProbeDynamicModule(LoggerUtil* pLogger, const char* pModulePath, DDModuleProbeInfo** ppProbeInfo)
{
    DD_ASSERT(pModulePath != nullptr);
    DD_ASSERT(ppProbeInfo != nullptr);

    Platform::Library moduleLibrary;

    DynamicModuleInfo dynamicInfo = {};
    dynamicInfo.pPath             = pModulePath;
    dynamicInfo.pLibrary          = &moduleLibrary;

    const DDModuleInterface* pModuleInterface = nullptr;

    DD_RESULT result = LoadDynamicModuleInterface(pLogger, &dynamicInfo, &pModuleInterface);

    if (result == DD_RESULT_SUCCESS)
    {
        // Calculate the size requirement for the buffer
        // The name and description are stored in memory after the main structure
        // Reasonable maximums for module name and description strings
        constexpr size_t kMaxModuleNameLength = 256;
        constexpr size_t kMaxModuleDescLength = 1024;
        const size_t nameSize        = Platform::Strlen_s(pModuleInterface->description.pName, kMaxModuleNameLength) + 1;
        const size_t descriptionSize = Platform::Strlen_s(pModuleInterface->description.pDescription, kMaxModuleDescLength) + 1;

        // Assert if strings exceed our assumed maximums (would indicate truncation)
        DD_ASSERT(nameSize <= kMaxModuleNameLength);
        DD_ASSERT(descriptionSize <= kMaxModuleDescLength);
        const size_t infoBufferSize  = (sizeof(DDModuleProbeInfo) + nameSize + descriptionSize);

        void* pProbeInfoMemory = Platform::GenericAllocCb.Alloc(infoBufferSize, alignof(DDModuleProbeInfo), false);

        if (pProbeInfoMemory != nullptr)
        {
            // Write the probe info into the buffer

            // Calculate the name and description pointers into the buffer
            void* pName        = VoidPtrInc(pProbeInfoMemory, sizeof(DDModuleProbeInfo));
            void* pDescription = VoidPtrInc(pProbeInfoMemory, sizeof(DDModuleProbeInfo) + nameSize);

            // Write the string data for name and description into the end of the buffer
            Platform::Memcpy_s(pName, nameSize, pModuleInterface->description.pName, nameSize);
            Platform::Memcpy_s(pDescription, descriptionSize, pModuleInterface->description.pDescription, descriptionSize);

            // Fill in the regular structure information
            DDModuleProbeInfo* pProbeInfo = reinterpret_cast<DDModuleProbeInfo*>(pProbeInfoMemory);

            pProbeInfo->pName        = reinterpret_cast<const char*>(pName);
            pProbeInfo->pDescription = reinterpret_cast<const char*>(pDescription);
            pProbeInfo->version      = pModuleInterface->description.moduleVersion;
            pProbeInfo->isCompatible = ddIsVersionCompatible(kRequiredModuleApiVersion, pModuleInterface->apiVersion);

            // Return the structure pointer to the caller
            (*ppProbeInfo) = pProbeInfo;
        }
        else
        {
            result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
        }
    }

    // The module library will unload itself here because of the scope exit.

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ToolModule::FreeProbeInfo(DDModuleProbeInfo** ppProbeInfo)
{
    DD_ASSERT(ppProbeInfo != nullptr);

    // Free the probe module info memory that the pointer points to
    Platform::GenericAllocCb.Free(*ppProbeInfo);

    // Null out the caller's pointer
    (*ppProbeInfo) = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Log some information about the versions we're dealing with
static void LogModuleVersions(LoggerUtil* pLogger, const DDModuleInterface* pInterface)
{
    const char* pModuleName = pInterface->description.pName;

    DD_MODULE_LOADER_LOGF(
        *pLogger,
        DD_LOG_LEVEL_INFO,
        "ddModuleLoader's Module API Version %u.%u.%u | %s's Module API Version %u.%u.%u",
        kRequiredModuleApiVersion.major,
        kRequiredModuleApiVersion.minor,
        kRequiredModuleApiVersion.patch,
        pModuleName,
        pInterface->apiVersion.major,
        pInterface->apiVersion.minor,
        pInterface->apiVersion.patch);

    DD_MODULE_LOADER_LOGF(
        *pLogger,
        DD_LOG_LEVEL_INFO,
        "%s's Module Version %u.%u.%u",
        pModuleName,
        pInterface->description.moduleVersion.major,
        pInterface->description.moduleVersion.minor,
        pInterface->description.moduleVersion.patch);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Internal helper function for loading dynamic module interfaces from a path
DD_RESULT LoadDynamicModuleInterface(
    LoggerUtil*               pLogger,
    const DynamicModuleInfo*  pDynamicInfo,
    const DDModuleInterface** ppInterface)
{
    DD_ASSERT(pDynamicInfo != nullptr);
    DD_ASSERT(pDynamicInfo->pPath != nullptr);
    DD_ASSERT(pDynamicInfo->pLibrary != nullptr);
    DD_ASSERT(ppInterface != nullptr);

    const char* pModulePath = pDynamicInfo->pPath;

    Platform::Library moduleLibrary;

    Result result = moduleLibrary.Load(pModulePath, Platform::LibrarySearchPaths::DllLoadDir);

    if (result == Result::Success)
    {
        DD_MODULE_LOADER_LOGF(
            *pLogger,
            DD_LOG_LEVEL_VERBOSE,
            "Successfully loaded module library from path: %s",
            pModulePath);
    }
    else
    {
        DD_MODULE_LOADER_LOGF(*pLogger, DD_LOG_LEVEL_ERROR, "Failed to load module library from path: %s", pModulePath);
    }

    PFN_ddModuleQueryModule pfnQueryModule = nullptr;
    if (result == Result::Success)
    {
        result = moduleLibrary.GetFunction(DD_MODULE_QUERY_MODULE_EXPORT_NAME, &pfnQueryModule) ?
                     Result::Success :
                     Result::FunctionNotFound;

        if (result == Result::Success)
        {
            DD_MODULE_LOADER_LOGF(
                *pLogger,
                DD_LOG_LEVEL_VERBOSE,
                "Successfully found exported module entry point %s in %s",
                DD_MODULE_QUERY_MODULE_EXPORT_NAME,
                pModulePath);
        }
        else
        {
            DD_MODULE_LOADER_LOGF(
                *pLogger,
                DD_LOG_LEVEL_ERROR,
                "Failed to find exported module entry point %s in %s",
                DD_MODULE_QUERY_MODULE_EXPORT_NAME,
                pModulePath);
        }
    }

    const DDModuleInterface* pInterface = nullptr;
    if (result == Result::Success)
    {
        pInterface = pfnQueryModule();
        result     = (pInterface != nullptr) ? Result::Success : Result::InterfaceNotFound;

        if (result == Result::Success)
        {
            DD_MODULE_LOADER_LOGF(
                *pLogger,
                DD_LOG_LEVEL_VERBOSE,
                "Successfully acquired module interface from %s",
                pModulePath);
        }
        else
        {
            DD_MODULE_LOADER_LOGF(
                *pLogger,
                DD_LOG_LEVEL_ERROR,
                "Failed to acquire module interface from %s",
                pModulePath);
        }
    }

    if (result == Result::Success)
    {
        // Return the results to the caller
        pDynamicInfo->pLibrary->Swap(&moduleLibrary);
        (*ppInterface) = pInterface;
    }
    else
    {
        // Nothing to do here, the library object will automatically unload itself once we leave this scope.
    }

    return DevDriverToDDResult(result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT ToolModule::LoadBuiltin(
    LoggerUtil*              pLogger,
    const ApiAllocCallbacks& apiAllocCb,
    const DDModuleInterface* pInterface,
    ToolModule**             ppModule)
{
    // With built-in modules, we already have the interface loaded.
    // We can skip populating dynamic info, and pass in NULL to ToolModule.
    const DynamicModuleInfo* pDynamicInfo = nullptr;

    return ToolModule::Create(pLogger, apiAllocCb, pInterface, pDynamicInfo, ppModule);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT ToolModule::LoadDynamic(
    LoggerUtil*              pLogger,
    const ApiAllocCallbacks& apiAllocCb,
    const char*              pModulePath,
    ToolModule**             ppModule)
{
    DD_ASSERT(pModulePath != nullptr);

    // First load the dynamic info for the module.
    // This is both the path and the loaded Library. The ToolModule will own the Library until it is destroyed.
    Platform::Library moduleLibrary;

    DynamicModuleInfo dynamicInfo = {};
    dynamicInfo.pPath             = pModulePath; // TODO: This seems sketchy...
    dynamicInfo.pLibrary          = &moduleLibrary;

    const DDModuleInterface* pInterface = nullptr;

    DD_RESULT result = LoadDynamicModuleInterface(pLogger, &dynamicInfo, &pInterface);

    if (result == DD_RESULT_SUCCESS)
    {
        result = ToolModule::Create(pLogger, apiAllocCb, pInterface, &dynamicInfo, ppModule);
    }

    return result;
}

DD_RESULT ToolModule::Create(
    LoggerUtil*              pLogger,
    const ApiAllocCallbacks& apiAllocCb,
    const DDModuleInterface* pInterface,
    const DynamicModuleInfo* pDynamicInfo,
    ToolModule**             ppModule)
{
    DD_RESULT result = DD_RESULT_COMMON_VERSION_MISMATCH;

    LogModuleVersions(pLogger, pInterface);

    // Make sure the module interface exposed by the module is compatible with the single version we support
    if (ddIsVersionCompatible(kRequiredModuleApiVersion, pInterface->apiVersion))
    {
        *ppModule = new ToolModule(pLogger, apiAllocCb, pInterface, pDynamicInfo);
        result    = DD_RESULT_SUCCESS;
    }

    return result;
}

// Since all creation goes through Create(), we centralize de-allocation here.
// This keeps the new/delete style consistent.
void ToolModule::Destroy() { delete this; }

} // namespace DDTool

/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>

#include <ddCommon.h>

#include <util/hashMap.h>

namespace DDTool
{

/// Helper structure used to carry information that's specific to dynamically loaded modules
struct DynamicModuleInfo
{
    const char*                   pPath;    /// [in] Path where the module was loaded from
    DevDriver::Platform::Library* pLibrary; /// [in/out] OS library object for the module
};

/// Class that encapsulates a ddModule API based "module" that's loaded into ddTool
/// This class is meant to insulate the ddTool code from the ddModule API to reduce the effort required for backwards
/// compatibility. Because of this, it's interface is very similar to ddModule's API.
class ToolModule
{
public:
    /// Allocates and loads a specified built-in module
    static DD_RESULT LoadBuiltin(
        LoggerUtil*              pLogger,
        const ApiAllocCallbacks& apiAllocCb,
        const DDModuleInterface* pInterface,
        ToolModule**             ppModule);

    /// Allocates and loads a specified dynamic module
    static DD_RESULT LoadDynamic(
        LoggerUtil*              pLogger,
        const ApiAllocCallbacks& apiAllocCb,
        const char*              pModulePath,
        ToolModule**             ppModule);

    static DD_RESULT ProbeDynamicModule(LoggerUtil* pLogger, const char* pModulePath, DDModuleProbeInfo** ppProbeInfo);

    static void FreeProbeInfo(DDModuleProbeInfo** ppProbeInfo);

    // Deallocates the ToolModule
    void Destroy();

    void* GetUserdata() const { return m_pUserdata; }

    void* SetUserdata(void* pUserdata)
    {
        void* pOldUserdata = m_pUserdata;
        m_pUserdata        = pUserdata;
        return pOldUserdata;
    }

    /// Returns the module description associated with this module
    const DDModuleDescription& GetDescription() const;

    /// Returns the loaded module info struct associated with this module
    DDModuleLoadedInfo GetModuleInfo();

    /// Returns true if this is a dynamically loaded module
    bool IsDynamicModule() const { return m_moduleLibrary.IsLoaded(); };

    /// If this is a dynamic module, this function returns the path where this module was loaded from
    /// Otherwise, it returns a null pointer
    const char* GetDynamicModulePath() const { return (IsDynamicModule() ? m_dynamicLibraryPath : nullptr); }

    /// Creates a system instance of the module
    DevDriver::Result CreateModuleSystemContext(
        const DDModuleSystemContextCreateInfo& createInfo,
        DDModuleSystemContext*                 phSystemContext); /// [out] Handle to the new system context

    /// Destroys a system instance of the module
    void DestroyModuleSystemContext(
        DDModuleSystemContext hSystemContext); /// [in] Handle to the module system context to destroy

    /// Handles a system event via the underlying module
    void HandleSystemEvent(
        DDModuleSystemContext  hContext,       /// Handle of the module system context to handle the event for
        DD_MODULE_SYSTEM_EVENT eventId,        /// Id of the event to handle
        const void*            pEventData,     /// [in] Pointer to event data
        size_t                 eventDataSize); /// Size of the event data pointed to by pEventData

    /// Queries the allocation info for a module client context
    /// Returns Unavailable if the module doesn't support client contexts
    DevDriver::Result QueryModuleClientContextAllocInfo(
        DDModuleClientContextAllocInfo* pAllocInfo); /// [out] Allocation info structure

    /// Creates a module client context for the provided client id
    DD_RESULT CreateModuleClientContext(
        const DDModuleSystemContextCreateInfo& systemCreateInfo, /// Information about the system
        const DDModuleClientInfo&              clientInfo,       /// Information about the client associated with this context
        DDModuleDataContext                    hDataContext,     /// Data context that will be associated with this client context
        void*                                  pContextMemory);  /// [in/out] Memory to create the client context in
                                                                 ///          This must be enough memory to fit the size previously returned by
                                                                 ///          the associated query client context size function.

    /// Destroys a module client context
    void DestroyModuleClientContext(
        DDModuleClientContext hContext); /// The handle of the client context to destroy

    /// Handles a client event via the underlying module
    void HandleClientEvent(
        DDModuleClientContext  hContext,       /// Handle of the module client context to handle the event for
        DD_MODULE_CLIENT_EVENT eventId,        /// Id of the event to handle
        const void*            pEventData,     /// [in] Pointer to event data
        size_t                 eventDataSize); /// Size of the event data pointed to by pEventData

    /// Creates a data context from this module
    DevDriver::Result CreateDataContext(
        const void*          pData,          /// [in] Optional pointer to initial data for the data context to use
                                             ///      If this is nullptr, an empty/default data context is created
        size_t               dataSize,       /// Size of the data pointed to by pData
        DDModuleDataContext* phDataContext); /// [out] Pointer to a handle where the newly created data context
                                             ///       handle should be written to.

    /// Destroys a data context from this module
    void DestroyDataContext(
        DDModuleDataContext* phDataContext); /// [in/out] Pointer to a data context handle associated with the data
                                             ///          context that should be destroyed

    /// Serializes a data context to memory
    DevDriver::Result SerializeDataContext(
        DDModuleDataContext hDataContext, /// Handle to the data context to serialize
        void*               pData,        /// [in] Pointer to the destination memory
        size_t*             pDataSize);   /// [in/out] This behavior of this parameter differs if pData is nullptr
                                          ///          pData == nullptr -> [out] Returns the size of the data context
                                          ///          pData != nullptr -> [in] Indicates the size of the memory pointed
                                          ///                                   to by pData

    /// Creates a connection context from this module
    DD_RESULT CreateConnectionContext(
        const DDModuleConnectionContextCreateInfo& createInfo, /// Information about the network this module is being loaded onto
        DDModuleConnectionContext*                 phContext); /// [out] Pointer to a handle where the newly created context is written

    /// Destroys a connection context from this module
    void DestroyConnectionContext(DDModuleConnectionContext hContext);

    /// Attempts to query an extension from the module
    DevDriver::Result QueryModuleExtension(
        DDModuleExtensionId                extensionId,           /// Extension identifier
        const DDApiVersion*                pRequiredVersion,      /// [in] Required version of the extension
        const DDModuleExtensionInterface** ppExtensionInterface); /// [out] Returned extension interface

    /// Returns true if this module supports the system context api
    bool HasSystemApi() const;

    /// Returns true if this module supports the client context api
    bool HasClientApi() const;

    /// Returns true if this module supports the data context api
    bool HasDataApi() const;

    /// Returns true if this module supports the connection context api
    bool HasConnectionApi() const;

private:
    /// Constructs a ToolModule if the requested module has a compatible version
    static DD_RESULT Create(
        LoggerUtil*              pLogger,
        const ApiAllocCallbacks& apiAllocCb,
        const DDModuleInterface* pInterface,
        const DynamicModuleInfo* pDynamicInfo,
        ToolModule**             ppModule);

    ToolModule(
        LoggerUtil*              pLogger,
        const ApiAllocCallbacks& apiAllocCb,
        const DDModuleInterface* pInterface,
        const DynamicModuleInfo* pDynamicInfo);

    ~ToolModule();

    /// Generates a client create info struct for this module
    DDModuleClientContextCreateInfo GenerateClientCreateInfo(
        const DDModuleSystemContextCreateInfo& systemCreateInfo,    /// Information about the system
        const DDModuleClientInfo&              clientInfo,          /// Information about the client
        DDModuleDataContext                    hDataContext) const; /// Data context to associate with the client

    void*                        m_pUserdata;
    const DDModuleInterface*     m_pInterface;
    DDModuleLoaderInterface      m_loaderInterface;
    DevDriver::Platform::Library m_moduleLibrary;
    char                         m_dynamicLibraryPath[DD_API_PATH_SIZE];
    LoggerUtil*                  m_pLogger;
};

DD_RESULT LoadDynamicModuleInterface(
    LoggerUtil*               pLogger,
    const DynamicModuleInfo*  pDynamicInfo,
    const DDModuleInterface** ppInterface);

/// Hash function implementation required for ToolModule to work in DevDriver's HashMap container
template <typename = void>
struct ToolModuleHasher
{
    uint32_t operator()(
        DDTool::ToolModule* pKey) const /// Module pointer to use as a key
    {
        uint32_t hash = 0;

        if (pKey != nullptr)
        {
            hash = DevDriver::MetroHash::MetroHash32(reinterpret_cast<const uint8_t*>(pKey), sizeof(*pKey));
        }
        else
        {
            DD_DEBUG_BREAK();
        }

        return hash;
    }
};

template <typename Value>
using ToolModuleHashMap = DevDriver::HashMap<
    ToolModule*,
    Value,
    sizeof(ToolModule*) * 8,
    ToolModuleHasher
>;

}; // namespace DevDriver

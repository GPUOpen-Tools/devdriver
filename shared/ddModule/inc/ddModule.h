/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddApi.h>

/// Compile time version information
#define DD_MODULE_API_MAJOR_VERSION 1
#define DD_MODULE_API_MINOR_VERSION 18
#define DD_MODULE_API_PATCH_VERSION 0

#define DD_MODULE_API_VERSION_STRING DD_API_STRINGIFY_VERSION(DD_MODULE_API_MAJOR_VERSION, \
                                                              DD_MODULE_API_MINOR_VERSION, \
                                                              DD_MODULE_API_PATCH_VERSION)

/// Name of the QueryModule function exported by dynamic modules
#define DD_MODULE_QUERY_MODULE_EXPORT_NAME "QueryModule"

// System client types
typedef enum
{
    DD_MODULE_SYSTEM_CLIENT_TYPE_ROUTER          = 0, /// The user mode system router
    DD_MODULE_SYSTEM_CLIENT_TYPE_GRAPHICS_DRIVER = 1, /// The kernel mode system graphics driver
    DD_MODULE_SYSTEM_CLIENT_TYPE_UTILITY_DRIVER  = 2, /// The kernel mode system utility driver

    DD_MODULE_SYSTEM_CLIENT_TYPE_COUNT                /// Number of system client types defined
} DD_MODULE_SYSTEM_CLIENT_TYPE;

/// Opaque handle that represents a "native" module specific interface
typedef const struct DDModuleNativeApi_t* DDModuleNativeApi;

/// Structure that contains information for interacting with the code that loaded this module
typedef struct DDModuleLoaderInterface
{
    DDLoggerInfo      logger;
    DDAllocCallbacks  apiAllocCb;
} DDModuleLoaderInterface;

/// Structure that contains information about a ddModule client
typedef struct DDModuleClientInfo
{
    char        name[256];        /// Name of the client
    char        description[256]; /// Description of the client
    DDClientId  clientId;         /// Message bus client id
    DDProcessId processId;        /// Id of the process that the client is running in
} DDModuleClientInfo;

/// Structure that contains information about a system client
/// NOTE: This structure only exists because we aren't able to produce the normal DDModuleClientInfo structure
///       data for system clients yet. We need to update the Pong system message in DevDriver to return full client
///       information and pipe it through DiscoverClients in order to replace this structure.
typedef struct DDModuleSystemClientInfo
{
    DDClientId id;
} DDModuleSystemClientInfo;

/// Structure that contains the information required to create a module system context
typedef struct DDModuleSystemContextCreateInfo
{
    DDModuleLoaderInterface  loader;
    DDNetConnection          connection;
    DDModuleSystemClientInfo systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_COUNT];
} DDModuleSystemContextCreateInfo;

/// Structure that contains the information required to create a module client context
typedef struct DDModuleClientContextCreateInfo
{
    DDModuleLoaderInterface  loader;
    DDNetConnection          connection;
    DDModuleDataContext      hDataContext;
    DDModuleSystemClientInfo systemClients[DD_MODULE_SYSTEM_CLIENT_TYPE_COUNT];
    DDModuleClientInfo       clientInfo;
} DDModuleClientContextCreateInfo;

/// Structure that contains the information required to create a module data context
typedef struct DDModuleDataContextCreateInfo
{
    DDModuleLoaderInterface loader;
} DDModuleDataContextCreateInfo;

/// Structure that contains the information required to create a module connection context
typedef struct DDModuleConnectionContextCreateInfo
{
    DDModuleLoaderInterface  loader;        /// Interface provided by the loader of the module
    DDNetConnection          hConnection;   /// A ddNet Connection to the Network. This is always a valid connection.
    DDRpcServer              hRpcServer;    /// Some module loaders may provide their own RpcServer. Modules that wish to
                                            /// register an RPC Service should prefer this to creating their own server.
    DDEventServer            hEventServer;  /// Some module loaders may provide their own EventServer. Modules that wish to
                                            /// register an Event Provider should prefer this to creating their own server.
} DDModuleConnectionContextCreateInfo;

/// Types of events that PFN_ddModuleSystemEventCallback may receive
typedef enum
{
    DD_MODULE_SYSTEM_EVENT_CLIENT_CONNECT = 0, /// Generated when a new client connects to the system
    DD_MODULE_SYSTEM_EVENT_CLIENT_DISCONNECT,  /// Generated when an existing client disconnects from the system
} DD_MODULE_SYSTEM_EVENT;

/// Types of events that PFN_ddModuleClientEventCallback may receive
typedef enum
{
    DD_MODULE_CLIENT_EVENT_STATE_CHANGED = 0 /// Generated when the target client transitions to a new
                                             /// initialization state
} DD_MODULE_CLIENT_EVENT;

/// Event data structure for the system CLIENT_CONNECT event
struct DDModuleSystemEventClientConnect
{
    DDModuleClientInfo clientInfo;
};

/// Event data structure for the system CLIENT_DISCONNECT event
struct DDModuleSystemEventClientDisconnect
{
    DDClientId clientId;
};

/// Event data structure for the client STATE_CHANGED event
struct DDModuleClientEventStateChanged
{
    DD_DRIVER_STATE newState; /// New state the client has transitioned into
};

/// Contains information about the memory needed to create a client context
typedef struct DDModuleClientContextAllocInfo
{
    size_t size;      /// Size in bytes of the allocation
    size_t alignment; /// Alignment in bytes for the allocation
} DDModuleClientContextAllocInfo;

/// Creates a system instance of the module
typedef DD_RESULT (*PFN_ddModuleCreateModuleSystemContext)(
    const DDModuleSystemContextCreateInfo* pCreateInfo,      /// [in] Description of a system context
    DDModuleSystemContext*                 phSystemContext); /// [out] Handle to the new system context

/// Destroys a system instance of the module
typedef void (*PFN_ddModuleDestroyModuleSystemContext)(
    DDModuleSystemContext hSystemContext); /// [in] Handle to the module system context to destroy

/// Queries a module for information about the memory needed to create a client context
typedef void (*PFN_ddModuleQueryModuleClientContextAllocInfo)(
    DDModuleClientContextAllocInfo* pAllocInfo); /// [out] Memory allocation information structure

/// Creates a per client instance of the module
typedef DD_RESULT (*PFN_ddModuleCreateModuleClientContext)(
    const DDModuleClientContextCreateInfo* pCreateInfo,     /// [in]     Description of a client context
    void*                                  pContextMemory); /// [in/out] Pointer to destination memory for the context data
                                                            ///          The caller is expected to query the number of bytes
                                                            ///          required for this memory prior to calling this function.
                                                            ///          The memory that this points to should be at least as
                                                            ///          large as the size that was previously returned from the
                                                            ///          associated client context size query function.

/// Destroys a per client instance of the module
typedef void (*PFN_ddModuleDestroyModuleClientContext)(
    DDModuleClientContext hClientContext); /// [in] Handle to the module client context to destroy

///  Creates a module data context (optionally based on a provided previously serialized data blob)
typedef DD_RESULT (*PFN_ddModuleCreateDataContext)(
    const DDModuleDataContextCreateInfo* pCreateInfo,    /// [in]  Information required to create a data context
    const void*                          pData,          /// [in]  Data used to initialize the new data context
    size_t                               dataSize,       /// [in]  Size of the data in pData
    DDModuleDataContext*                 phDataContext); /// [out] Handle to the newly created module data context

///  Destroys a module data context.
typedef void (*PFN_ddModuleDestroyDataContext)(
    DDModuleDataContext* phDataContext);  /// [in/out] Handle to the module data context to be destroyed

/// Serializes a module data context to a binary blob that is used for saving data between runs. This function can be
/// called with a NULL pData pointer to query the size required.
///  NOTE: Any calls that are made which may change the data context may change the required size.
typedef DD_RESULT (*PFN_ddModuleSerializeDataContext)(
    const DDModuleDataContext hDataContext, /// [in] Handle to the module data context
    void*                     pData,        /// [out] Location where serialized data will be copied to
    size_t*                   pDataSize);   /// [in/out] As an input this is the size of memory backed by pData, the
                                            /// function will update this to reflect the number of bytes written
                                            /// to pData.

/// Allows the caller to add a named, opaque block of serialized userdata to the provided data context's serialized data.
/// Passing a NULL pointer for pBytes will remove the node from the data context.
/// This data is serialized to disk, so avoid storing machine dependent information like pointers.
/// It is recommended to supply Json.
typedef DD_RESULT (*PFN_ddModuleUpdateUserdataNode)(
    DDModuleDataContext hDataContext, /// [in] Handle to the module data context
    const char*         pNodeName,    /// Name of the node. This is used to retrieve the node data later.
                                      /// Recommended to just be your app's name.
    const void*         pBytes,       /// Pointer to a bytes array that is copied into the data context.
    size_t              bytesSize);   /// Number of bytes to copy.

/// Allows the caller to retrieve a named, opaque block of serialized userdata from the provided data context's serialized data.
/// This data should have been added before using PFN_ddModuleUpdateUserdataNode.
/// If the node does exist, pfnReceive is called with data that does not live longer than the function call.
///     Either process the data in-place, or copy it out.
/// If the node does not exist, pfnReceive is not called.
typedef DD_RESULT (*PFN_ddModuleQueryUserdataNode)(
    DDModuleDataContext  hDataContext,     /// [in] Handle to the module data context
    const char*          pNodeName,        /// The name of the node being queried.
    void*                pUserdata,        /// Userdata for the receive callback
    PFN_ddReceiveBinary  pfnReceiveBytes); /// [out] Callback used to receive the stored node data

/// Queries the status of the provided client.
/// NOTE: This functions should only be called in response to the DD_TOOL_EVENT_CLIENT_INITIALIZE event. Calling at
///       other times may result in a different/incorrect status.
/// Expected result codes:
///    DD_RESULT_SUCCESS                 - The module has initialized and connected to the client successfully
///    DD_RESULT_COMMON_VERSION_MISMATCH - The module and driver versions are not compatible for this module
///    DD_RESULT_COMMON_UNSUPPORTED      - The driver does not implement/support this module.
///    Other                             - An error occurred during module init/connnect unrelated to the client
typedef DD_RESULT(*PFN_ddModuleQueryStatus)(
    DDModuleClientContext hClientContext);  /// Handle to the client context to receive data from

/// Queries the protocol version of the provided client
typedef DD_RESULT(*PFN_ddModuleQueryClientProtocolVersion)(
    DDModuleClientContext hClientContext, /// Handle to the client context to query
    DDApiVersion*         pVersion);      /// [out] Pointer to the version to be returned

/// Queries per-system information as a JSON string, which can then be inspected through
/// the provided callback function.
/// Returns a failing error code if a router connection has not been established.
typedef DD_RESULT(*PFN_ddModuleQuerySystemInfo)(
    DDModuleSystemContext hSystemContext,  /// Handle to the system context to receive data from
    void*                 pUserdata,       /// Optional userdata passed to pfnReceiveJson
    PFN_ddReceiveText     pfnReceiveJson); /// A callback to receive the system info

/// Sends a system event to a module to allow it to perform any required processing for it
typedef void (*PFN_ddModuleHandleSystemEvent)(
    DDModuleSystemContext  hSystemContext, /// [in] Handle to the per system module instance
    DD_MODULE_SYSTEM_EVENT eventId,        /// Id value for the event
    const void*            pEventData,     /// [in] Pointer to event data
    size_t                 eventDataSize); /// Size of the event data in bytes

/// Sends a client event to a module to allow it to perform any required processing for it
typedef void (*PFN_ddModuleHandleClientEvent)(
    DDModuleClientContext  hClientContext, /// [in] Handle to the per client module instance
    DD_MODULE_CLIENT_EVENT eventId,        /// Id value for the event
    const void*            pEventData,     /// [in] Pointer to event data
    size_t                 eventDataSize); /// Size of the event data in bytes

/// Creates a new module connection context using the provided information
typedef DD_RESULT (*PFN_ddModuleCreateConnectionContext)(
    const DDModuleConnectionContextCreateInfo* pInfo,      /// [in] Information about the network that the router context
                                                           /// may use to hook itself into the network.
                                                           /// Modules should cache this if they need it to unregister themselves!
    DDModuleConnectionContext*                 phContext); /// [out] Handle to the newly created module connection context

/// Destroys an existing module connection context
typedef void (*PFN_ddModuleDestroyConnectionContext)(
    DDModuleConnectionContext hContext); /// [in] Handle to the module connection context to be destroyed

/// Queries the module for an extension interface
typedef const DDModuleExtensionInterface* (*PFN_ddModuleQueryExtension)(
    DDModuleExtensionId id); /// Unique id of the desired module extension

/// Version 0 of the generic ddModule API
typedef struct DDModuleCommonApi
{
    PFN_ddModuleQuerySystemInfo            pfnQuerySystemInfo;
    PFN_ddModuleUpdateUserdataNode         pfnUpdateUserdataNode;
    PFN_ddModuleQueryUserdataNode          pfnQueryUserdataNode;
    PFN_ddModuleQueryStatus                pfnQueryStatus;
    PFN_ddModuleQueryClientProtocolVersion pfnQueryClientProtocolVersion;
} DDModuleCommonApi;

typedef struct DDModuleSystemContextApi_0000
{
    PFN_ddModuleCreateModuleSystemContext  pfnCreateModuleSystemContext;
    PFN_ddModuleDestroyModuleSystemContext pfnDestroyModuleSystemContext;
    PFN_ddModuleHandleSystemEvent          pfnHandleSystemEvent;
} DDModuleSystemContextApi_0000;

typedef struct DDModuleClientContextApi_0000
{
    PFN_ddModuleQueryModuleClientContextAllocInfo pfnQueryModuleClientContextAllocInfo;
    PFN_ddModuleCreateModuleClientContext         pfnCreateModuleClientContext;
    PFN_ddModuleDestroyModuleClientContext        pfnDestroyModuleClientContext;
    PFN_ddModuleHandleClientEvent                 pfnHandleClientEvent;
} DDModuleClientContextApi_0000;

typedef struct DDModuleDataContextApi_0000
{
    PFN_ddModuleCreateDataContext         pfnCreateDataContext;
    PFN_ddModuleDestroyDataContext        pfnDestroyDataContext;
    PFN_ddModuleSerializeDataContext      pfnSerializeDataContext;
} DDModuleDataContextApi_0000;

typedef struct DDModuleConnectionContextApi_0000
{
    PFN_ddModuleCreateConnectionContext  pfnCreateConnectionContext;
    PFN_ddModuleDestroyConnectionContext pfnDestroyConnectionContext;
} DDModuleConnectionContextApi_0000;

/// Version 0 of the generic ddModule API
typedef struct DDModuleApi_0000
{
    const DDModuleSystemContextApi_0000*     pSystemContextApi;
    const DDModuleClientContextApi_0000*     pClientContextApi;
    const DDModuleDataContextApi_0000*       pDataContextApi;
    const DDModuleConnectionContextApi_0000* pConnectionContextApi;
    PFN_ddModuleQueryExtension               pfnQueryExtension;
} DDModuleApi_0000;

/// Function prototype used to query the interface for a dynamic module
typedef const DDModuleInterface* (*PFN_ddModuleQueryModule)(void);

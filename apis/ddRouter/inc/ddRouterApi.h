/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_ROUTER_API_HEADER
#define DD_ROUTER_API_HEADER

#include <ddApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Compile time version information
#define DD_ROUTER_API_MAJOR_VERSION 0
#define DD_ROUTER_API_MINOR_VERSION 13
#define DD_ROUTER_API_PATCH_VERSION 0

#define DD_ROUTER_API_VERSION_STRING DD_API_STRINGIFY_VERSION(DD_ROUTER_API_MAJOR_VERSION, \
                                                              DD_ROUTER_API_MINOR_VERSION, \
                                                              DD_ROUTER_API_PATCH_VERSION)

/// Name of the API
#define DD_ROUTER_API_NAME "ddRouter"

/// Description of the API
#define DD_ROUTER_API_DESCRIPTION "API that allows applications to manage driver communication networks"

/// Identifier for the API
/// This identifier is used to acquire access to the API's interface
// Note: This is "drvroutr" in big endian ASCII
#define DD_ROUTER_API_ID 0x647276726f757472

/// DDRouter is an opaque pointer to a router
typedef struct DDRouter_t* DDRouter;

/// Flags structure used to configure transport support for the router
typedef union DDRouterTransportFlags
{
    struct DDRouterTransportFlagsFields
    {
        /// Disables the kernel transport
        /// Programs connected through the kernel network cannot interact with
        /// this router
        uint32_t disableKernelTransport : 1;

        /// Disables the remote transport
        /// Programs on the network cannot interact with this router
        uint32_t disableRemoteTransport : 1;

        /// Restricts the remote transport's network access to the local machine
        /// Programs on other machines cannot interact with this router
        uint32_t disableExternalNetwork : 1;

        // if kernel transport is enabled (disableKernelTransport == 0) then
        // the only kernel transport to be enabled will be the KMD transport
        uint32_t kernelTransportKMDOnly : 1;

        /// Reserved for future usage (Program to 0)
        uint32_t reserved               : 28;
    } fields;
    uint32_t value;
} DDRouterTransportFlags;

// Default settings for the client timeout count:
static const uint32_t kDefaultClientTimeoutCount = 3;
static const uint32_t kDisableClientTimeout      = 0xFFFFFFFF;

/// Create info for creating a DDRouter
typedef struct DDRouterCreateInfo
{
    /// Brief description of the router
    /// This string is available to other programs on the bus
    const char*              pDescription;

    /// Configures what type of connections are supported
    DDRouterTransportFlags   transportFlags;

    /// An identifier for local inter-process communications. To use
    /// the default communication channel, set this to 0.
    uint16_t                 localPort;

    /// The port number for remote network communications. If
    /// set to 0, the default port number is chosen.
    uint16_t                 remotePort;

    /// Callbacks for memory allocation.
    /// If this is zeroed, ddRouter will use the system default allocator.
    DDAllocCallbacks         alloc;

    /// Callbacks for logging.
    /// If this is zeroed, ddRouter will use the system default logger.
    DDLoggerInfo             logger;

    /// Number of pings to give the client driver before disconnecting.
    /// The default was originally set to 3.
    uint32_t                 clientTimeoutCount;

} DDRouterCreateInfo;

/// Get version of the loaded library to check interface compatibility
typedef DDApiVersion (*PFN_ddRouterQueryVersion)(void);

/// Get human-readable representation of the loaded library version
typedef const char* (*PFN_ddRouterQueryVersionString)(void);

/// Convert a `DD_RESULT` into a human recognizable string.
/// Use this with `printf`-style functions to provide useful error messages:
///
///
/// ```c
///     const DD_RESULT result = ddRouterContextCreate(&info, &router);
///     if (result != DD_ROUTER_SUCCESS) {
///         printf("An error occurred: %s", ddRouterResultToString(result));
///     } else {
///         printf("%s" "Success!");
///     }
/// ```
typedef const char* (*PFN_ddRouterResultToString)(
    DD_RESULT result
);

/// Creates a router and sets up a developer mode message bus on the system.
/// The output parameter is a handle to the router and this handle must be
/// provided to all other function calls.
typedef DD_RESULT (*PFN_ddRouterCreate)(
    /// Creation info
    const DDRouterCreateInfo* pCreateInfo,

    /// Handle to the new router
    DDRouter*                 phRouter
);

/// Destroys a router. This shuts down the developer mode message bus on the
/// system
typedef void (*PFN_ddRouterDestroy)(
    /// Handle to the router being destroyed
    DDRouter hRouter
);

/// Loads a built-in module
typedef DD_RESULT (*PFN_ddRouterLoadBuiltinModule)(
    /// Handle to the DDRouter
    DDRouter                 hRouter,

    /// Pointer to a builtin module interface
    const DDModuleInterface* pInterface,

    /// A pointer to `DDModuleLoadedInfo`, which is populated when the module is
    /// sucessfully loaded. This argument can be nullptr if the info is not
    /// needed.
    DDModuleLoadedInfo*      pModuleInfo
);

/// Loads a dynamic module from the provided path
typedef DD_RESULT (*PFN_ddRouterLoadDynamicModule)(
    /// Handle to the DDRouter
    DDRouter            hRouter,

    /// Path to a dynamic module on the filesystem
    const char*         pModulePath,

    /// A pointer to `DDModuleLoadedInfo`, which is populated when the module is
    /// sucessfully loaded. This argument can be nullptr if the info is not
    /// needed.
    DDModuleLoadedInfo* pModuleInfo
);

/// Unloads a module
typedef DD_RESULT (*PFN_ddRouterUnloadModule)(
    /// Handle to the DDRouter
    DDRouter        hRouter,

    /// Handle to the module to be unloaded.
    DDModuleContext hModule);

/// API structure
typedef struct DDRouterApi
{
    PFN_ddRouterQueryVersion       pfnQueryVersion;
    PFN_ddRouterQueryVersionString pfnQueryVersionString;
    PFN_ddRouterResultToString     pfnResultToString;
    PFN_ddRouterCreate             pfnCreate;
    PFN_ddRouterDestroy            pfnDestroy;
    PFN_ddRouterLoadBuiltinModule  pfnLoadBuiltinModule;
    PFN_ddRouterLoadDynamicModule  pfnLoadDynamicModule;
    PFN_ddRouterUnloadModule       pfnUnloadModule;
} DDRouterApi;

#ifdef __cplusplus
} // extern "C"
#endif

#endif

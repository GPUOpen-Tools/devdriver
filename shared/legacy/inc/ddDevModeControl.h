/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */
#pragma once

#include <ddPlatform.h>

namespace DevDriver
{

#if (defined(DD_PLATFORM_WINDOWS_UM) || defined(DD_PLATFORM_WINDOWS_KM))
// TODO: Finalize these
DEFINE_GUID(AmdUtilityDriverGuid,
            0x56ad5226,
            0x614e, 0x6976,
            0x41, 0x79, 0x79, 0x6d, 0x64, 0x00, 0x1e, 0xf9);

// Filename to open when connecting to kernel mode amdlog driver
constexpr const char* kDriverFileName = "\\\\.\\AmdLog";

// Pipe name to open when connecting to the user mode utility driver
static constexpr const char* kWinIoCtlPipeName    = R"(\\.\pipe\AmdUmUtilityDriver)";

#endif

///////////////////////
// Typedef for the Router Prefix type
typedef uint32 RouterPrefix;

enum struct DevModeCmd: uint32
{
    Unknown = 0,                // Illegal command

    RegisterClient,             // Register a new client on the bus
    UnregisterClient,           // Unregister an existing client from the bus

    RegisterRouter,             // Register a new router on the bus
    UnregisterRouter,           // Unregister an existing router from the bus

    EnableDeveloperMode,        // Attempts to enable developer mode on the bus
    DisableDeveloperMode,       // Attempts to disable developer mode on the bus

    QueryCapabilities,          // Queries the capabilities of the bus
    QueryDeveloperModeStatus,   // Queries the current developer mode configuration

    Count,
};

/// DevMode Request Header
/// All DevMode command types have this header at the beginning of the struct.
DD_NETWORK_STRUCT(DevModeResponseHeader, 4)
{
    DevModeCmd cmd       = DevModeCmd::Unknown; /// The devmode command to be executed
    Result     result    = Result::Error;       /// The result of the devmode command execution

    uint32     reserved1 = 0;                   /// Reserved for future use (Program to zero)
    uint32     reserved0 = 0;                   /// Reserved for future use (Program to zero)

    static DevModeResponseHeader FromCmd(DevModeCmd devModeCmd)
    {
        DevModeResponseHeader header;
        header.cmd = devModeCmd;

        return header;
    }
};

DD_CHECK_SIZE(DevModeResponseHeader, 16);

enum struct DevModeBusType: uint32
{
    Unknown = 0, // Unknown

    Auto,        // Automatic selection
    UserMode,    // Request a user mode bus
    KernelMode,  // Request a kernel mode bus

    Count
};

static inline const char* DevModeCmdToHumanString(DevModeCmd cmd)
{
    switch(cmd)
    {
        case DevModeCmd::RegisterClient:           return "RegisterClient";
        case DevModeCmd::UnregisterClient:         return "UnregisterClient";

        case DevModeCmd::RegisterRouter:           return "RegisterRouter";
        case DevModeCmd::UnregisterRouter:         return "UnregisterRouter";

        case DevModeCmd::EnableDeveloperMode:      return "EnableDeveloperMode";
        case DevModeCmd::DisableDeveloperMode:     return "DisableDeveloperMode";

        case DevModeCmd::QueryCapabilities:        return "QueryCapabilities";
        case DevModeCmd::QueryDeveloperModeStatus: return "QueryDeveloperModeStatus";

        default:                                   return "<Unrecognized DevModeCmd>";
    }
}

///////////////////////
// Developer Mode Status Flags
union DeveloperModeFlags
{
    struct {
        uint32 enableEmbeddedClient : 1;  // Enable the embedded client
        uint32 enableTdrLogging     : 1;  // Enable TDR Logging
                                          // This only is used if the embedded client is available
        uint32 reserved             : 30; // Reserved for future use
    } flags;
    uint32 u32All;

    constexpr DeveloperModeFlags()
        : u32All(0)
    {
    }

};

///////////////////////
// Developer Mode Initialization Settings
DD_NETWORK_STRUCT(DeveloperModeSettings, 4)
{
    RouterPrefix       routerPrefix; // Routing prefix to be assigned by the router
    DeveloperModeFlags features;     // Developer Mode initialization flags
};

}

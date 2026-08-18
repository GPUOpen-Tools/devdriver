/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <stdbool.h>
#include <ddApi.h>

/// Compile time version information
#define COMMAND_EXTENSION_VERSION_MAJOR 0
#define COMMAND_EXTENSION_VERSION_MINOR 7
#define COMMAND_EXTENSION_VERSION_PATCH 0

/// Name of the extension
#define COMMAND_EXTENSION_NAME "Command"

/// Description of the extension
#define COMMAND_EXTENSION_DESCRIPTION "Allows modules to expose generic command execution functionality"

/// Identifier for the extension
/// This identifier is used to acquire access to the extension's interface

// Note: This is "command!" in ASCII
#define COMMAND_EXTENSION_ID 0x636f6d6d616e6421

/// Enumeration of command parameter types
typedef enum
{
    COMMAND_PARAMETER_TYPE_UNKNOWN        = 0, /// Unknown type
    COMMAND_PARAMETER_TYPE_BOOL           = 1, /// Boolean type
    COMMAND_PARAMETER_TYPE_INT            = 2, /// 64-bit signed integer
    COMMAND_PARAMETER_TYPE_UINT           = 3, /// 64-bit unsigned integer
    COMMAND_PARAMETER_TYPE_FLOAT          = 4, /// 64-bit floating point number
    COMMAND_PARAMETER_TYPE_STRING         = 5, /// String type
    COMMAND_PARAMETER_TYPE_BLOB           = 6, /// Binary blob
    COMMAND_PARAMETER_TYPE_DATA_CONTEXT   = 7, /// Data context
    COMMAND_PARAMETER_TYPE_SYSTEM_CONTEXT = 8, /// System context
    COMMAND_PARAMETER_TYPE_CLIENT_CONTEXT = 9, /// Client context

    COMMAND_PARAMETER_TYPE_COUNT          = 10, /// Count
} COMMAND_PARAMETER_TYPE;

/// Opaque identifier for an individual command
/// Acquired via the QueryCommands interface function
typedef uint32_t CommandId;

/// Structure that contains information about an arbitrary binary blob that's being passed as a command parameter
typedef struct CommandParameterBlobValue
{
    const void* pData;
    size_t      dataSize;
} CommandParameterBlobValue;

/// Tagged union containing a command parameter type and value
typedef struct CommandParameterValue
{
    COMMAND_PARAMETER_TYPE type; /// Type of value

    uint32_t _padding; /// Padding used to ensure that the entire struct remains 8 byte aligned

    union CommandParameterValueDataField
    {
        uint8_t                   boolVal;
        int64_t                   i64Val;
        uint64_t                  u64Val;
        double                    f64Val;
        const char*               pStrVal;
        CommandParameterBlobValue blobVal;
        DDModuleDataContext       hDataContext;
        DDModuleSystemContext     hSystemContext;
        DDModuleClientContext     hClientContext;
    } data;
} CommandParameterValue;

/// Struct representing a pair of a parameter value and its name.
typedef struct CommandParameter
{
    const char*           pName;
    CommandParameterValue value;
} CommandParameter;

/// Structure that describes a command parameter
typedef struct CommandParameterDescription
{
    const char*                  pName;         /// Name of the parameter
    const char*                  pDescription;  /// Description of the parameter
    COMMAND_PARAMETER_TYPE       type;          /// Type of the parameter
    bool                         isRequired;    /// Whether the parameter is required or optional
    const CommandParameterValue* pDefaultValue; /// The default value for the parameter, if no default is supported for the
                                                //< parameter this field will be NULL.
} CommandParameterDescription;

/// Structure that describes a command
typedef struct DDModuleCommandDescription
{
    const char*                        pName;         /// Name of the command
                                                      /// This string is used to uniquely identify the command
    const char*                        pDescription;  /// Description of the command
    const char*                        pOutputHint;   /// Output hint string
                                                      /// This string is used by commands to indicate the format of their output
                                                      /// data. Simple users of the Command API may choose to ignore this field
                                                      /// and always treat the output data as binary. Sophisticated users may
                                                      /// leverage this field to offer improved data visualization options.
    const CommandParameterDescription* pParameters;   /// Array of command parameter structures
    uint32_t                           numParameters; /// Number of entries in the pParameters array
} DDModuleCommandDescription;

/// Creation information for a command context
typedef struct DDModuleCommandContextCreateInfo
{
    DDLoggerInfo     loggerInfo; /// Required logging callbacks used by modules to log events.
} DDModuleCommandContextCreateInfo;

/// Creates a command extension context
typedef DD_RESULT (*PFN_CreateCommandContext)(
    const DDModuleCommandContextCreateInfo* pCreateInfo,     /// [in] Command context create info
    DDModuleCommandContext*                 phCommandContext /// [out] Handle to the created command context
);

/// Destroys a command extension context
typedef void (*PFN_DestroyCommandContext)(
    DDModuleCommandContext hCommandContext /// [out] Handle to command context to be destroyed
);

/// Returns descriptions for all of the supported commands
typedef void (*PFN_CommandQueryCommands)(
    DDModuleCommandContext             hCommandContext, /// [in]  The command context from which the commands will be queried.
    uint32_t*                          pNumCommands,    /// [out] Number of commands returned in ppCommands
    const DDModuleCommandDescription** ppCommands       /// [out] Pointer to an array of the supported command descriptions
);

/// Attempts to execute the command specified by the caller
typedef DD_RESULT (*PFN_CommandExecuteCommand)(
    DDModuleCommandContext  hCommandContext, /// [in]  The command context to execute the command.
    const char*             pCommandName,    /// The name of the command.
    uint32_t                numParameters,   /// Number of parameters in pParameters
    const CommandParameter* pParameters,     /// [in] Array of parameter to pass to the command
    const DDByteWriter*     pWriter          /// [in] Pointer to the output writer to use for the command
);

/// Command Extension API
typedef struct CommandExtApi_0000
{
    PFN_CreateCommandContext  pfnCreateContext;
    PFN_DestroyCommandContext pfnDestroyContext;
    PFN_CommandQueryCommands  pfnQueryCommands;
    PFN_CommandExecuteCommand pfnExecuteCommand;
} CommandExtApi_0000;

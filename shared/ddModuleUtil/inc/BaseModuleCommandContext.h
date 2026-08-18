/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>
#include <ModuleLogger.h>
#include <CommandModuleExt.h>

#include <util/vector.h>
#include <util/hashMap.h>

#define DD_COMMAND_LOG(logger, level, ...) \
    (logger).Printf(DD_MAKE_LOG_EVENT(level, "CommandModule"), __VA_ARGS__)

/// Defines an invalid command context handle
static const DDModuleCommandContext kInvalidCommandContextHandle = 0;

/// Class used to encapsulate the module specific per system state
class BaseModuleCommandContext
{
public:
    /// Attempts to initialize all state within the context
    DD_RESULT Initialize();

    /// Returns all commands supported by the context
    void QueryCommands(
        uint32_t*                          pNumCommands,
        const DDModuleCommandDescription** ppCommands);

    /// Executes a command
    DD_RESULT ExecuteCommand(
        const char*             pCommandName,
        uint32_t                numParameters,
        const CommandParameter* pParameters,
        const DDByteWriter*     pWriter);

    const LoggerUtil& GetLoggerInfo() const { return m_logger; }

protected:
    BaseModuleCommandContext(const DDModuleCommandContextCreateInfo& createInfo);
    virtual ~BaseModuleCommandContext();

    // Derived classes are expected to override this function and register their commands inside it
    virtual DD_RESULT RegisterCommands() = 0;

    /// Command executor callback implemented by individual modules.
    ///
    /// `pUserdata`    - A pointer to arbitrary user data. This is the same pointer passed to `RegisterCommand`.
    /// `ppParameters` - An array of pointers to each parameter value. The size of this array is
    ///                  guaranteed to be the total number of all parameters specified by the
    ///                  command. If a parameter is optional and users didn't provide it, it's
    ///                  represented by nullptr.
    /// `pWriter`      - A pointer to the output writer for the command.
    ///
    /// Note, since BaseModuleCommandContext already checks if all required parameters are present (for string
    /// parameter it also checks if `CommandParameterValue::data` is not nullptr) and returns error on
    /// check-failure before invoking this callback, individual module writers implementing this callback can
    /// be assured that the required parameters will not be nullptr.
    typedef DD_RESULT (*PFN_ExecuteRegisteredCommand)(
        void*                         pUserdata,
        const CommandParameterValue** ppParameters,
        const DDByteWriter*           pWriter);

    // Registers a command and exposes it to the module system for execution
    DD_RESULT RegisterCommand(
        const DDModuleCommandDescription& description,
        void*                             pUserdata,
        PFN_ExecuteRegisteredCommand      pfnExecute);

    // Helper structure used to organized registered command functions and their associated userdata
    struct RegisteredCommand
    {
        void*                        pUserdata;
        PFN_ExecuteRegisteredCommand pfnExecute;
        size_t                       commandDescIndex;
    };

private:
    LoggerUtil                                         m_logger;
    bool                                               m_isInitialized;
    DevDriver::Vector<DDModuleCommandDescription>      m_commandDescs;
    DevDriver::HashMap<const char*, RegisteredCommand> m_commands;
};

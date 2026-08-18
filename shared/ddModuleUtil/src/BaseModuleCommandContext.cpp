/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <BaseModuleCommandContext.h>
#include <ddCommon.h>
#include <util/vector.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleCommandContext::BaseModuleCommandContext(
    const DDModuleCommandContextCreateInfo& createInfo)
    : m_logger(createInfo.loggerInfo)
    , m_isInitialized(false)
    , m_commandDescs(DevDriver::Platform::GenericAllocCb)
    , m_commands(DevDriver::Platform::GenericAllocCb)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleCommandContext::~BaseModuleCommandContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleCommandContext::Initialize()
{
    DD_RESULT result = RegisterCommands();

    if (result == DD_RESULT_SUCCESS)
    {
        m_isInitialized = true;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BaseModuleCommandContext::QueryCommands(
    uint32_t*                          pNumCommands,
    const DDModuleCommandDescription** ppCommands)
{
    if ((pNumCommands != nullptr) && (ppCommands != nullptr))
    {
        (*pNumCommands) = static_cast<uint32_t>(m_commandDescs.Size());
        (*ppCommands) = m_commandDescs.IsEmpty() ? nullptr : m_commandDescs.Data();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleCommandContext::ExecuteCommand(
    const char*             pCommandName,
    uint32_t                numUserParameters,
    const CommandParameter* pUserParameters,
    const DDByteWriter*     pWriter)
{
    DD_RESULT result = DD_RESULT_COMMON_DOES_NOT_EXIST;

    // Final parameter values passed to command executor callback.
    // nullptr represents an optinal parameter that isn't provided by users.
    DevDriver::Vector<const CommandParameterValue*> finalParameters(DevDriver::Platform::GenericAllocCb);

    // Attempt to locate the requested command
    const RegisteredCommand* pCommand = m_commands.FindValue(pCommandName);
    if (pCommand != nullptr)
    {
        // We found the requested command
        result = DD_RESULT_SUCCESS;

        DD_ASSERT(pCommand->commandDescIndex < m_commandDescs.Size());
        const DDModuleCommandDescription& commandDesc = m_commandDescs[pCommand->commandDescIndex];

        finalParameters.Reserve(commandDesc.numParameters);

        for (size_t paramIdx = 0; paramIdx < commandDesc.numParameters; ++paramIdx)
        {
            const CommandParameterDescription* pParameterDesc = &commandDesc.pParameters[paramIdx];

            // Check if a parameter is supplied by user.
            const CommandParameterValue* pParamFound = nullptr;
            for (uint32_t userParamIdx = 0; userParamIdx < numUserParameters; ++userParamIdx)
            {
                const CommandParameter* availableParam = &pUserParameters[userParamIdx];
                if (strcmp(pParameterDesc->pName, availableParam->pName) == 0)
                {
                    pParamFound = &availableParam->value;

                    // If it is available, check its type.
                    if (commandDesc.pParameters[paramIdx].type != availableParam->value.type)
                    {
                        result = DD_RESULT_COMMON_INVALID_PARAMETER;

                        DD_COMMAND_LOG(
                            m_logger,
                            DD_LOG_LEVEL::DD_LOG_LEVEL_WARN,
                            "Command parameter mismatch encountered at index %u: Expected type %u does not match provided type %u.",
                            static_cast<uint32_t>(paramIdx),
                            pParameterDesc->type,
                            availableParam->value.type
                        );
                    }
                    break;
                }
            }

            if (result != DD_RESULT_SUCCESS)
            {
                // Stop further validation if there is already an error (type mismatch).
                break;
            }

            if ((pParameterDesc->isRequired == true) &&
                ((pParamFound == nullptr) || ((pParamFound->type == COMMAND_PARAMETER_TYPE_STRING) && (pParamFound->data.pStrVal == nullptr))))
            {
                // If user didn't supply this parameter value, but it's required, this is a user error.
                result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;

                DD_COMMAND_LOG(
                    m_logger,
                    DD_LOG_LEVEL::DD_LOG_LEVEL_ERROR,
                    "Missing required parameter: %s.",
                    commandDesc.pParameters[paramIdx].pName
                );

                break;
            }
            else
            {
                // Otherwise, push the parameter regardless if found.
                finalParameters.PushBack(pParamFound);
            }
        }
    }

    if (result == DD_RESULT_SUCCESS)
    {
        // Make sure finalParameters has the right amount of items (even if some of them might be nullptr).
        DD_ASSERT(finalParameters.Size() == m_commandDescs[pCommand->commandDescIndex].numParameters);

        // We've passed all validation, attempt to execute the command.
        result = pCommand->pfnExecute(pCommand->pUserdata, finalParameters.Data(), pWriter);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleCommandContext::RegisterCommand(
    const DDModuleCommandDescription& description,
    void*                             pUserdata,
    PFN_ExecuteRegisteredCommand      pfnExecute)
{
    DD_RESULT result = DD_RESULT_COMMON_UNSUPPORTED;

    if (m_isInitialized == false)
    {
        RegisteredCommand command = {};

        command.pUserdata    = pUserdata;
        command.pfnExecute   = pfnExecute;
        command.commandDescIndex = m_commandDescs.Size();

        // Attempt to add the command to the command map by its name.
        // This will fail if a command with the same name is already registered.
        result = DevDriverToDDResult(m_commands.Create(description.pName, command));

        if (result == DD_RESULT_SUCCESS)
        {
            result = m_commandDescs.PushBack(description) ? DD_RESULT_SUCCESS : DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;

            // Remove the command from the map if we fail to save the command description
            if (result != DD_RESULT_SUCCESS)
            {
                DD_UNHANDLED_RESULT(m_commands.Erase(description.pName));
            }
        }
    }

    return result;
}

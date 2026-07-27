/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_enhanced_crash_info_api.h>
#include <dd_api_registry_api.h>
#include <dd_logger_api.h>
#include <ddNet.h>

namespace DevDriver
{
class EnhancedCrashInfo
{
private:

    DDNetConnection m_net;
    uint16_t        m_amdLogConnectionId;
    DDLoggerApi*    m_pLogger;

public:
    EnhancedCrashInfo();
    ~EnhancedCrashInfo();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);
    void      ClearAfterRouterDisconnect();

    void SetRpcClientInfo(DDNetConnection ddNet, uint16_t amdLogConnectionId);

    DD_RESULT QueryEnhancedCrashInfoConfig(DDEnhancedCrashInfoConfig* pEnhancedCrashInfoConfig);
    DD_RESULT SetEnhancedCrashInfoConfig(const DDEnhancedCrashInfoConfig* pEnhancedCrashInfoConfig);
};

} // namespace DevDriver

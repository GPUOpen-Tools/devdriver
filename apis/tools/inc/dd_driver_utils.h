/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_driver_utils_api.h>
#include <dd_common_api.h>
#include <dd_api_registry_api.h>
#include <dd_mutex.h>

#include <ddNet.h>

#include <string>
#include <vector>
#include <cstdint>

namespace DevDriver
{

class Tool;

class DriverUtils
{
private:
    using FeatureFlag = DD_DRIVER_UTILS_FEATURE_FLAG;
    using SetterNames = std::vector<std::string>;

    struct Feature
    {
        FeatureFlag flag;
        const char* pName;
        Mutex       mutex;
        SetterNames setterNames;
    };

    Feature m_features[DD_DRIVER_UTILS_FEATURE_COUNT];

    Tool*           m_pTool;
    DDNetConnection m_net;

public:
    DriverUtils(Tool* pTool);

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);

    void SetRpcClientInfo(DDNetConnection ddNet);

    DD_RESULT SetFeature(
        DD_DRIVER_UTILS_FEATURE      feature,
        DD_DRIVER_UTILS_FEATURE_FLAG flag,
        const char*                  pSetterName,
        uint32_t                     setterNameSize);

    DD_RESULT QueryPalDriverInfo(
        DDConnectionId      umdConnection,
        const DDByteWriter& writer);

    void SendDriverFeatureFlags(DDConnectionId umdConnectionId);

    DD_RESULT SetDriverOverlayString(
        DDConnectionId  umdConnectionId,
        const char*     pOverlayString,
        uint32_t        strIdx);

    DD_RESULT SetDbgLogSeverityLevel(
        DDConnectionId  umdConnectionId,
        uint32_t        severity);

    DD_RESULT SetDbgLogOriginationMask(
        DDConnectionId  umdConnectionId,
        uint32_t        mask);

    DD_RESULT ModifyDbgLogOriginationMask(
        DDConnectionId  umdConnectionId,
        uint32_t        origination,
        bool            enable);
};

} // namespace DevDriver

/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <dd_connection_api.h>
#include <dd_api_registry_api.h>

#include <dd_logger_api.h>
#include <dd_mutex.h>

#include <ddNet.h>
#include <protocols/driverControlClient.h>

#include <vector>
#include <atomic>
#include <thread>

namespace DevDriver
{

struct ToolConnData
{
    using DriverControl = DriverControlProtocol::DriverControlClient;

    DDConnectionId    umdConnectionId;
    uint16_t          kmdConnectionId;
    DriverControl     driverControl;
    bool              finished;
    std::atomic<bool> exitRequested;
    std::thread       driverConnectionThread;

    ToolConnData(DDConnectionId umdId, uint16_t kmdId, DDNetConnection ddNet);
};

class ToolConnection
{
    DDNetConnection m_net;

    std::vector<DDConnectionCallbacks> m_connectionCallbacksImpls;
    RWLock                             m_connectionCallbacksLock;

    std::vector<ToolConnData*>         m_connections;
    Mutex                              m_connectionsMutex;
    std::atomic<uint32_t>              m_connectionCount;

    DDConnectionFilter                 m_filter;

public:
    ToolConnection();
    ~ToolConnection();

    DD_RESULT Initialize(DDApiRegistry* pApiRegistry);

    void SetDDNet(DDNetConnection ddNet);

    DD_RESULT GetDriverState(DDConnectionId umdConnectionId, DD_DRIVER_STATE* pState);

    bool FilterConnection(const DDConnectionInfo* pConnInfo);

    void HandleDriverConnection(DDConnectionId umdConnectionId, uint16_t kmdConnectionId);
    void CloseDriverConnections();

    void SetFilter(const DDConnectionFilter& filter);

    void IncConnectionCount() { m_connectionCount.fetch_add(1); }
    void DecConnectionCount() { m_connectionCount.fetch_sub(1); }
    uint32_t GetConnectionCount() { return m_connectionCount.load(); }

    DD_RESULT AddConnectionCallbacks(const DDConnectionCallbacks* pCallbacks);
    DD_RESULT RemoveConnectionCallbacks(const DDConnectionCallbacksImpl* pCallbacks);

    void OnRouterConnected(DDConnectionId connectionId);
    void OnRouterDisconnected();

    void OnDriverConnected(const DDConnectionInfo* pConnInfo);
    void OnDriverDisconnected(DDConnectionId connectionId);
    void OnDriverStateChanged(DDConnectionId connectionId, DD_DRIVER_STATE state);
};

} // namespace DevDriver

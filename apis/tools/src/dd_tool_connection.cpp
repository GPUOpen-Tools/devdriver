/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_tool_connection.h>
#include <dd_constants.h>

#include <ddCommon.h>
#include <msgChannel.h>

#include <algorithm>

#define LOG_ERROR(fmt, ...) s_pLogger->Log(           \
                                s_pLogger->pInstance, \
                                DD_LOG_LVL_ERROR,     \
                                "[DDToolConn] " fmt,  \
                                ## __VA_ARGS__)

#define LOG_INFO(fmt, ...) s_pLogger->Log(           \
                               s_pLogger->pInstance, \
                               DD_LOG_LVL_INFO,      \
                               "[DDToolConn] " fmt,  \
                               ## __VA_ARGS__)

namespace
{

using namespace DevDriver;
using namespace DevDriver::DriverControlProtocol;

constexpr uint32_t kWaitForDisconnectTimeoutMillis = 300;

DDLoggerApi* s_pLogger;

DD_DRIVER_STATE DevDriverToDDDriverState(DriverStatus state)
{
    switch (state)
    {
    case DriverStatus::HaltedOnPlatformInit: return DD_DRIVER_STATE_PLATFORMINIT;
    case DriverStatus::HaltedOnDeviceInit:   return DD_DRIVER_STATE_DEVICEINIT;
    case DriverStatus::HaltedPostDeviceInit: return DD_DRIVER_STATE_POSTDEVICEINIT;
    case DriverStatus::Running:              return DD_DRIVER_STATE_RUNNING;
    case DriverStatus::Paused:               return DD_DRIVER_STATE_PAUSED;
    default:
    {
        DD_ASSERT(false);
        return DD_DRIVER_STATE_UNKNOWN;
    }
    }
}

const char* DriverStatusToString(DriverStatus state)
{
    switch (state)
    {
    case DriverStatus::HaltedOnPlatformInit: return "PlatformInit";
    case DriverStatus::HaltedOnDeviceInit:   return "DeviceInit";
    case DriverStatus::HaltedPostDeviceInit: return "PostDeviceInit";
    case DriverStatus::Running:              return "Running";
    case DriverStatus::Paused:               return "Paused";
    default:                                 return "Unknown";
    }
}

Result StepThroughDriverInitialization(ToolConnection* pToolConn, ToolConnData* pConnData)
{
    Result stepResult = Result::Success;

    // `HaltedOnPlatformInit` is always the first in driver initialization.
    DriverStatus driverState = DriverStatus::HaltedOnPlatformInit;

    pToolConn->OnDriverStateChanged(pConnData->umdConnectionId, DevDriverToDDDriverState(driverState));

    while (driverState != DriverStatus::HaltedPostDeviceInit)
    {
        DriverStatus newDriverState = DriverStatus::Count;
        stepResult = pConnData->driverControl.AdvanceDriverState(&newDriverState);

        if (stepResult == Result::Success)
        {
            pToolConn->OnDriverStateChanged(pConnData->umdConnectionId, DevDriverToDDDriverState(newDriverState));
            driverState = newDriverState;
        }
        else
        {
            LOG_ERROR(
                "Failed to step driver initialization from %s to %s, with connection id: %u.",
                DriverStatusToString(driverState),
                DriverStatusToString(newDriverState),
                pConnData->umdConnectionId);
            break;
        }
    }

    if (stepResult == Result::Success)
    {
        stepResult = pConnData->driverControl.ResumeDriver();
        if (stepResult == Result::Success)
        {
            DriverStatus newDriverState = DriverStatus::Count;
            stepResult = pConnData->driverControl.QueryDriverStatus(&newDriverState);
            if (stepResult == Result::Success)
            {
                pToolConn->OnDriverStateChanged(
                    pConnData->umdConnectionId,
                    DevDriverToDDDriverState(newDriverState));
            }
            else
            {
                LOG_ERROR("Failed to query driver status, with connection id: %u", pConnData->umdConnectionId);
            }
        }
        else
        {
            LOG_ERROR(
                "Failed to resume driver after initialization, with connection id: %u",
                pConnData->umdConnectionId);
        }
    }

    return stepResult;
}

void DriverConnectionThreadFn(ToolConnection* pToolConn, ToolConnData* pConnData)
{
    Result result = pConnData->driverControl.Connect(
        pConnData->umdConnectionId,
        kDriverControlConnectTimeoutMillisec);

    if (result != Result::Success)
    {
        LOG_ERROR(
            "DriverControl failed to connect to UMD with id %u. DD_RESULT: %s",
            pConnData->umdConnectionId,
            ddApiResultToString(DevDriverToDDResult(result)));
    }

    ClientInfoStruct clientInfo{};

    if (result == Result::Success)
    {
        result = pConnData->driverControl.QueryClientInfo(&clientInfo);
        if (result != Result::Success)
        {
            LOG_ERROR("DriverControl failed to query client info.");
        }
    }

    DriverStatus initialState = DriverStatus::Count;

    if (result == Result::Success)
    {
        result = pConnData->driverControl.QueryDriverStatus(&initialState);
        if (result != Result::Success)
        {
            LOG_ERROR("DriverControl failed to query driver state.");
        }
    }

    bool ignoreConnection = false;
    DDConnectionInfo connInfo{};

    if (result == Result::Success)
    {
        if (initialState == DriverStatus::HaltedOnPlatformInit)
        {
            connInfo.umdConnectionId = pConnData->umdConnectionId;
            connInfo.kmdConnectionId = pConnData->kmdConnectionId;
            connInfo.processId       = clientInfo.processId;
            connInfo.pProcessName    = clientInfo.clientName;
            connInfo.pDescription    = clientInfo.clientDescription;

            ignoreConnection = pToolConn->FilterConnection(&connInfo);
        }
        else
        {
            result = Result::InvalidParameter;
            LOG_ERROR("The initial driver state is unexpected: %u", initialState);
        }
    }

    if (result == Result::Success)
    {
        if (ignoreConnection)
        {
            LOG_INFO("Ignoring the driver connection to the process: %s", clientInfo.clientName);

            DriverStatus driverState = DriverStatus::HaltedOnPlatformInit;
            result = pConnData->driverControl.IgnoreDriver();

            // If we're dealing with an older driver, we're forced to step it through all init stages manually.
            // Newer drivers will simply disconnect themselves after PlatformInit once they've been ignored.
            if (result == Result::VersionMismatch)
            {
                // Reset the result to success before we attempt to walk the driver through the initialization
                // process.
                result = Result::Success;

                while ((result == Result::Success) && (driverState != DriverStatus::HaltedPostDeviceInit))
                {
                    result = pConnData->driverControl.AdvanceDriverState(&driverState);
                    if (result != Result::Success)
                    {
                        LOG_ERROR(
                            "Failed to advance driver state (connection id: %u).",
                            pConnData->umdConnectionId);
                    }
                }
            }

            if (result == Result::Success)
            {
                result = pConnData->driverControl.ResumeDriver();
                if (result != Result::Success)
                {
                    LOG_ERROR(
                        "Failed to resume driver after initialization (connection id: %u).",
                        pConnData->umdConnectionId);
                }
            }
        }
        else
        {
            LOG_INFO("Start driver initialization (connection id: %u).", pConnData->umdConnectionId);

            pToolConn->IncConnectionCount();
            pToolConn->OnDriverConnected(&connInfo);

            result = StepThroughDriverInitialization(pToolConn, pConnData);
            if (result == Result::Success)
            {
                LOG_INFO("Successfully initialized driver (connection id: %u).", pConnData->umdConnectionId);

                for (;;)
                {
                    Result waitResult = pConnData->driverControl.WaitForDisconnection(kWaitForDisconnectTimeoutMillis);
                    if (waitResult == Result::NotReady)
                    {
                        if (pConnData->exitRequested)
                        {
                            // Connection is still running, but user requested exit.
                            break;
                        }
                    }
                    else
                    {
                        if (waitResult != Result::Success)
                        {
                            LOG_ERROR("Unexpected result from WaitForDisconnection: %u.", waitResult);
                        }
                        break;
                    }
                }
            }

            pToolConn->OnDriverDisconnected(pConnData->umdConnectionId);
        }
    }

    pConnData->driverControl.Disconnect();

    if (!ignoreConnection)
    {
        pToolConn->DecConnectionCount();
    }

    // Signal that `pConnData` can be removed.
    pConnData->finished = true;
}

// Wrapper functions for DDConnectionApi

void ConnectionApi_SetConnectionFilter(DDConnectionInstance* pInstance, DDConnectionFilter filter)
{
    DevDriver::ToolConnection* pToolConn = reinterpret_cast<DevDriver::ToolConnection*>(pInstance);
    pToolConn->SetFilter(filter);
}

DD_RESULT ConnectionApi_AddConnectionCallbacks(
    DDConnectionInstance*        pInstance,
    const DDConnectionCallbacks* pCallback)
{
    DD_RESULT result = DD_RESULT_SUCCESS;
    DevDriver::ToolConnection* pToolConn = reinterpret_cast<DevDriver::ToolConnection*>(pInstance);
    if (pCallback->pImpl)
    {
        result = pToolConn->AddConnectionCallbacks(pCallback);
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }
    return result;
}

DD_RESULT ConnectionApi_RemoveConnectionCallbacks(
    DDConnectionInstance* pInstance,
    const DDConnectionCallbacksImpl* pImpl)
{
    DD_RESULT result = DD_RESULT_SUCCESS;
    DevDriver::ToolConnection* pToolConn = reinterpret_cast<DevDriver::ToolConnection*>(pInstance);
    if (pImpl)
    {
        result = pToolConn->RemoveConnectionCallbacks(pImpl);
    }
    else
    {
        result = DD_RESULT_COMMON_INVALID_PARAMETER;
    }
    return result;
}

DD_RESULT ConnectionApi_GetDriverState(DDConnectionInstance* pInstance,
                                       DDConnectionId        umdConnectionId,
                                       DD_DRIVER_STATE*      pState)
{
    DevDriver::ToolConnection* pToolConn = reinterpret_cast<DevDriver::ToolConnection*>(pInstance);

    return pToolConn->GetDriverState(umdConnectionId, pState);
}

} // anonymous namespace

namespace DevDriver
{

ToolConnData::ToolConnData(DDConnectionId umdId, uint16_t kmdId, DDNetConnection ddNet)
    : umdConnectionId{umdId}
    , kmdConnectionId{kmdId}
    , driverControl{reinterpret_cast<IMsgChannel*>(ddNet)}
    , finished{false}
    , exitRequested{false}
{}

ToolConnection::ToolConnection()
    : m_net{DD_API_INVALID_HANDLE}
    , m_connectionCount(0)
    , m_filter{}
{
    m_connections.reserve(8);
}

ToolConnection::~ToolConnection()
{
    for (auto connIter = m_connections.begin(); connIter != m_connections.end(); connIter++)
    {
        ToolConnData* pConnData = *connIter;

        if ((*connIter)->finished == false)
        {
            LOG_INFO(
                "Driver connection (umdConnectionId: %u) still alive during ToolConnection destruction.",
                (*connIter)->umdConnectionId);
        }

        pConnData->exitRequested = true;
        if (pConnData->driverConnectionThread.joinable())
        {
            pConnData->driverConnectionThread.join();
        }

        delete (*connIter);
    }
}

DD_RESULT ToolConnection::Initialize(DDApiRegistry* pApiRegistry)
{
    DD_RESULT result = pApiRegistry->Get(
        pApiRegistry->pInstance,
        DD_LOGGER_API_NAME,
        DDVersion {
            DD_LOGGER_API_VERSION_MAJOR,
            DD_LOGGER_API_VERSION_MINOR,
            DD_LOGGER_API_VERSION_PATCH},
        reinterpret_cast<void**>(&s_pLogger));

    DD_ASSERT(result == DD_RESULT_SUCCESS);

    if (result == DD_RESULT_SUCCESS)
    {
        DDConnectionApi connApi {
            reinterpret_cast<DDConnectionInstance*>(this),
            ConnectionApi_SetConnectionFilter,
            ConnectionApi_AddConnectionCallbacks,
            ConnectionApi_RemoveConnectionCallbacks,
            ConnectionApi_GetDriverState };

        result = pApiRegistry->Add(
            pApiRegistry->pInstance,
            DD_CONNECTION_API_NAME,
            DDVersion {
                DD_CONNECTION_API_VERSION_MAJOR,
                DD_CONNECTION_API_VERSION_MINOR,
                DD_CONNECTION_API_VERSION_PATCH},
            &connApi,
            sizeof(connApi));

        if (result != DD_RESULT_SUCCESS)
        {
            LOG_ERROR("Failed to register DDConnectionApi. DD_RESULT: %u.", result);
        }
    }

    return result;
}

void ToolConnection::SetDDNet(DDNetConnection ddNet)
{
    m_net = ddNet;
}

DD_RESULT ToolConnection::GetDriverState(DDConnectionId umdConnectionId, DD_DRIVER_STATE* pState)
{
    LockGuard lock(m_connectionsMutex);

    DD_RESULT result = DD_RESULT_DD_GENERIC_INVALID_PARAMETER;

    if (pState != nullptr)
    {
        auto foundConn =
            std::find_if(m_connections.begin(),
                         m_connections.end(),
                         [umdConnectionId](ToolConnData* conn) { return (umdConnectionId == conn->umdConnectionId); });

        if (foundConn != m_connections.end())
        {
            DriverStatus state;
            result = DevDriverToDDResult((*foundConn)->driverControl.QueryDriverStatus(&state));

            *pState = DevDriverToDDDriverState(state);
        }
        else
        {
            *pState = DD_DRIVER_STATE_UNKNOWN;
            result  = DD_RESULT_DD_GENERIC_UNAVAILABLE;
        }
    }

    return result;
}

bool ToolConnection::FilterConnection(const DDConnectionInfo* pConnInfo)
{
    bool ignore = false;
    if (m_filter.filter)
    {
        ignore = m_filter.filter(m_filter.pUserData, pConnInfo);
    }
    return ignore;
}

void ToolConnection::HandleDriverConnection(DDConnectionId umdConnectionId, uint16_t kmdConnectionId)
{
    LockGuard lock(m_connectionsMutex);

    auto foundConn = std::find_if(
        m_connections.begin(),
        m_connections.end(),
        [umdConnectionId](ToolConnData* conn) { return (umdConnectionId == conn->umdConnectionId); });

    if (foundConn == m_connections.end())
    {
        ToolConnData* pConnData = new ToolConnData(umdConnectionId, kmdConnectionId, m_net);
        m_connections.push_back(pConnData);

        // We spawn a thread for each driver connection. After driver initialization finishes, the thread
        // goes into a loop checking for driver disconnection before it exits.
        pConnData->driverConnectionThread = std::thread(DriverConnectionThreadFn, this, pConnData);
    }
    else
    {
        LOG_ERROR("Connection (umd id: %u, kmd id: %u) already exists.", umdConnectionId, kmdConnectionId);
    }

    // Erase finished connections.
    for (auto connIter = m_connections.begin(); connIter != m_connections.end();)
    {
        if ((*connIter)->finished)
        {
            if ((*connIter)->driverConnectionThread.joinable())
            {
                (*connIter)->driverConnectionThread.join();
            }

            delete (*connIter);
            connIter = m_connections.erase(connIter);
        }
        else
        {
            ++connIter;
        }
    }
}

void ToolConnection::CloseDriverConnections()
{
    LockGuard lock(m_connectionsMutex);
    for (auto connIter = m_connections.begin(); connIter != m_connections.end();)
    {
        ToolConnData* pConnData = (*connIter);

        pConnData->exitRequested = true;
        if (pConnData->driverConnectionThread.joinable())
        {
            pConnData->driverConnectionThread.join();
        }

        connIter = m_connections.erase(connIter);
        delete pConnData;
    }
}

void ToolConnection::SetFilter(const DDConnectionFilter& filter)
{
    m_filter = filter;
}

DD_RESULT ToolConnection::AddConnectionCallbacks(const DDConnectionCallbacks* pCallbacks)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RWLockGuard<RWLock::LockType::Write> lock(m_connectionCallbacksLock);

    auto found = std::find_if(
        m_connectionCallbacksImpls.begin(),
        m_connectionCallbacksImpls.end(),
        [pCallbacks](const DDConnectionCallbacks& pItem) {
            return (pItem.pImpl == pCallbacks->pImpl);
        });

    if (found == m_connectionCallbacksImpls.end())
    {
        m_connectionCallbacksImpls.push_back(*pCallbacks);
    }
    else
    {
        LOG_ERROR("Failed to add DDConnectionCallbacks: already exists.");
        result = DD_RESULT_COMMON_ALREADY_EXISTS;
    }

    return result;
}

DD_RESULT ToolConnection::RemoveConnectionCallbacks(const DDConnectionCallbacksImpl* pImpl)
{
    DD_RESULT result = DD_RESULT_SUCCESS;

    RWLockGuard<RWLock::LockType::Write> lock(m_connectionCallbacksLock);

    bool removed = false;
    for (auto itr = m_connectionCallbacksImpls.begin(); itr != m_connectionCallbacksImpls.end(); ++itr)
    {
        if (itr->pImpl == pImpl)
        {
            m_connectionCallbacksImpls.erase(itr);
            removed = true;
            break;
        }
    }

    if (!removed)
    {
        LOG_ERROR("Failed to remove DDConnectionCallbacks: doesn't exist.");
        result = DD_RESULT_COMMON_DOES_NOT_EXIST;
    }

    return result;
}

void ToolConnection::OnRouterConnected(DDConnectionId connectionId)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_connectionCallbacksLock);

    for (auto callbacksItr = m_connectionCallbacksImpls.begin();
         callbacksItr != m_connectionCallbacksImpls.end();
         ++callbacksItr)
    {
        if (callbacksItr->OnRouterConnected)
        {
            callbacksItr->OnRouterConnected(callbacksItr->pImpl, connectionId);
        }
    }
}

void ToolConnection::OnRouterDisconnected()
{
    RWLockGuard<RWLock::LockType::Read> lock(m_connectionCallbacksLock);

    for (auto callbacksItr = m_connectionCallbacksImpls.begin();
         callbacksItr != m_connectionCallbacksImpls.end();
         ++callbacksItr)
    {
        if (callbacksItr->OnRouterDisconnected)
        {
            callbacksItr->OnRouterDisconnected(callbacksItr->pImpl);
        }
    }
}

void ToolConnection::OnDriverConnected(const DDConnectionInfo* pConnInfo)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_connectionCallbacksLock);

    for (auto callbacksItr = m_connectionCallbacksImpls.begin();
         callbacksItr != m_connectionCallbacksImpls.end();
         ++callbacksItr)
    {
        if (callbacksItr->OnDriverConnected)
        {
            callbacksItr->OnDriverConnected(callbacksItr->pImpl, pConnInfo);
        }
    }
}

void ToolConnection::OnDriverDisconnected(DDConnectionId umdConnectionId)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_connectionCallbacksLock);

    for (auto callbacksItr = m_connectionCallbacksImpls.begin();
         callbacksItr != m_connectionCallbacksImpls.end();
         ++callbacksItr)
    {
        if (callbacksItr->OnDriverDisconnected)
        {
            callbacksItr->OnDriverDisconnected(callbacksItr->pImpl, umdConnectionId);
        }
    }
}

void ToolConnection::OnDriverStateChanged(DDConnectionId umdConnectionId, DD_DRIVER_STATE state)
{
    RWLockGuard<RWLock::LockType::Read> lock(m_connectionCallbacksLock);

    for (auto callbacksItr = m_connectionCallbacksImpls.begin();
         callbacksItr != m_connectionCallbacksImpls.end();
         ++callbacksItr)
    {
        if (callbacksItr->OnDriverStateChanged)
        {
            callbacksItr->OnDriverStateChanged(callbacksItr->pImpl, umdConnectionId, state);
        }
    }
}

} // namespace DevDriver

/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <string>

#include <ddRouter.h>
#include <ddCommon.h>
#include <ddNet.h>
#include <devDriverServer.h>
#include <devDriverClient.h>
#include <ddRpcClientApi.h>

#if defined(_MSC_VER)
    #pragma warning(push)
    // Disable MSVC warning about converting signed character to unsigned in gtest test name template code
    // This doesn't hurt anything for us
    #pragma warning(disable : 6330)
#endif

#include <gtest/gtest.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

/// Teach gtest how to display DD_RESULT
inline std::ostream& operator<<(std::ostream& os, DD_RESULT result)
{
    return os << ddApiResultToString(result) << " (" << static_cast<int>(result) << ")";
}

/// Operator overloads for DDApiVersion
inline bool operator==(const DDApiVersion& a, const DDApiVersion& b)
{
    return ((a.major == b.major) && (a.minor == b.minor) && (a.patch == b.patch));
}

inline bool operator!=(const DDApiVersion& a, const DDApiVersion& b)
{
    return !(a == b);
}

// Dumb RAII'd wrapper around ddRouter
class DDTestRouter
{
public:
    /// A convenient timeout for use in general Router operations
    static constexpr uint32_t kCommonRouterTimeoutMs = 1000;

    ~DDTestRouter();

    /// Initialize a router with a unique connection id based on the process id.
    ///
    /// Each test must do this to avoid collisions.
    DD_RESULT Init(const char* pTestName);

    /// Returns the local port (connection id) associated with the router.
    /// 0 means a default connection id is used.
    uint16_t GetLocalPort() const { return m_localPort; }

    /// Returns the network port used by the router
    uint16_t GetRemotePort() const { return m_remotePort; }

    /// Returns a local connection info struct that will connect a client to this router's network
    DDNetConnectionInfo GenerateLocalInfo();

    /// Returns a remote connection info struct that will connect a client to this router's network
    DDNetConnectionInfo GenerateRemoteInfo();

private:
    DDRouter m_hRouter = DD_API_INVALID_HANDLE;

    /// Local connection id used by the router
    uint16_t m_localPort = 0;

    /// Network port used by the router
    uint16_t m_remotePort = 0;
};

/// A test fixture with no network access
/// Only used to test simple API usage
class DDNoNetworkTest : public ::testing::Test
{
};

/// A fully network test fixture
/// This fixture provides a router and client/server message channel pair
class DDNetworkedTest : public DDNoNetworkTest
{
protected:
    static constexpr uint32_t kConnectionTimeoutMs = 3000;

    void SetUp() override;
    void TearDown() override;

    /// Router object that owns the network used by the tests
    DDTestRouter m_router;

    DDNetConnection m_hServerConnection = DD_API_INVALID_HANDLE;
    DDClientId      m_serverClientId    = DD_API_INVALID_CLIENT_ID;

    DDNetConnection m_hClientConnection = DD_API_INVALID_HANDLE;
    DDClientId      m_clientClientId    = DD_API_INVALID_CLIENT_ID;
};

class ClientServerTest : public DDNetworkedTest
{
protected:
    ClientServerTest() :
        m_pClient(nullptr),
        m_pServer(nullptr),
        m_haltedEvent(false) {}

    void SetUp();
    void TearDown();
    void HandleMessageChannelEvent(DevDriver::BusEventType type, const void* pEventData, size_t eventDataSize);

    DevDriver::DevDriverClient* m_pClient;
    DevDriver::DevDriverServer* m_pServer;
    DevDriver::Platform::Event  m_haltedEvent;
};

class RpcClientServerTest : public DDNetworkedTest
{
protected:

    // Arbitrary protocol id values used for testing, 0 is default. This is effectively the port.
    static constexpr DDProtocolId kTestProtocolId = 64;

    RpcClientServerTest() :
         m_hServer(DD_API_INVALID_HANDLE),
         m_hClient(DD_API_INVALID_HANDLE) {}

    void SetUp();
    void TearDown();

    DDRpcServer m_hServer;
    DDRpcClient m_hClient;
};

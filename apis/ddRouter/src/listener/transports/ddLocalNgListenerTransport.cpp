/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <listener/transports/ddLocalNgListenerTransport.h>
#include <ddPlatform.h>
#include <ddDevModeControlCmds.h>

namespace DevDriver
{
    LocalNgListenerTransport::LocalNgListenerTransport(const AllocCb& allocCb, RouterPrefix listenerPrefix, RouterPrefix kmdPrefix)
        : m_allocCb(allocCb)
        , m_sharedQueue()
        , m_transportThread()
        , m_devModeControlDevice(allocCb)
        , m_pRouter(nullptr)
        , m_transportHandle(0)
        , m_listenerPrefix(listenerPrefix & kRouterPrefixMask)
        , m_kmdPrefix(kmdPrefix & kRouterPrefixMask)
        , m_isEnabled(false)
    {
    }

    LocalNgListenerTransport::~LocalNgListenerTransport()
    {
    }

    Result LocalNgListenerTransport::ReceiveMessage(ConnectionInfo& connectionInfo, MessageBuffer& message, uint32 timeoutInMs)
    {
        Result result = m_sharedQueue.ReceiveMessage(message, timeoutInMs);
        if (result == Result::Success)
        {
            connectionInfo.handle = m_transportHandle;
            connectionInfo.size = 0;
        }
        return result;
    }

    Result LocalNgListenerTransport::TransmitMessage(const ConnectionInfo& connectionInfo, const MessageBuffer& message)
    {
        DD_ASSERT(connectionInfo.handle == m_transportHandle);
        DD_UNUSED(connectionInfo);
        return m_sharedQueue.TransmitMessage(message, kTransmitTimeoutInMs);
    }

    Result LocalNgListenerTransport::TransmitBroadcastMessage(const MessageBuffer& message)
    {
        return m_sharedQueue.TransmitMessage(message, kTransmitTimeoutInMs);
    }

    Result LocalNgListenerTransport::Enable(RouterCore* pRouter, TransportHandle handle)
    {
        Result result = Result::Error;

        if (m_isEnabled == false)
        {
            // Initialize our shared queue
            result = m_sharedQueue.Initialize(
                kMaxQueueLength,
                kMaxMessageSizeInBytes);

            if (result == Result::Success)
            {
                // Initialize a devmode control device
                result = m_devModeControlDevice.Initialize(DevModeBusType::Auto);
            }

            bool developerModeEnabled = false;

            if (result == Result::Success)
            {
                // Enable developer mode in the kernel
                EnableDeveloperModeRequest request = {};

                request.input.settings.routerPrefix = m_kmdPrefix;
                request.input.settings.features.flags.enableEmbeddedClient = true;

                result = m_devModeControlDevice.MakeDevModeRequest(&request);
                if (result == Result::Success)
                {
                    result = request.header.result;
                }

                if (result == Result::Success)
                {
                    developerModeEnabled = true;
                }
            }

            if (result == Result::Success)
            {
                RegisterRouterRequest request = {};

                // The input for the devmode request input is a raw copy of the send and receive queue, along with the
                // routing prefix for the listener.
                request.input.sendQueue = m_sharedQueue.GetSendQueue();
                request.input.receiveQueue = m_sharedQueue.GetReceiveQueue();
                request.input.routingPrefix = m_listenerPrefix;

                // Issue the RegisterRouter devmode request to register the Listener as a routing destination
                result = m_devModeControlDevice.MakeDevModeRequest(&request);
                if (result == Result::Success)
                {
                    result = request.header.result;
                }

                if (result == Result::Success)
                {
                    // If it is true we overwrite the send/receive queues with the received ones and start a routing
                    // thread up for this transport.
                    m_sharedQueue.SetSendQueue(request.output.sendQueue);
                    m_sharedQueue.SetReceiveQueue(request.output.receiveQueue);
                    m_transportThread.Start(pRouter, this);
                    m_transportHandle = handle;

                    m_isEnabled = true;
                }
            }

            // If we fail to initialize, make sure we attempt to disable developer mode and clean up resources
            if (result != Result::Success)
            {
                if (developerModeEnabled)
                {
                    DisableDeveloperModeRequest request = {};
                    DD_UNHANDLED_RESULT(m_devModeControlDevice.MakeDevModeRequest(&request));
                    if (result == Result::Success)
                    {
                        DD_UNHANDLED_RESULT(request.header.result);
                    }
                }

                m_devModeControlDevice.Destroy();
                m_sharedQueue.Destroy();
            }
        }

        return result;
    }

    Result LocalNgListenerTransport::Disable()
    {
        Result result = Result::Error;

        if (m_isEnabled)
        {
            // Stop the routing thread
            m_transportHandle = 0;
            m_transportThread.Stop();

            // Issue the UnregisterRouter devmode request
            UnregisterRouterRequest unregisterRequest = {};

            unregisterRequest.input.routingPrefix = m_listenerPrefix;

            result = m_devModeControlDevice.MakeDevModeRequest(&unregisterRequest);
            if (result == Result::Success)
            {
                result = unregisterRequest.header.result;
            }

            if (result == Result::Success)
            {
                // Destroy the message transport
                m_sharedQueue.Destroy();

                // Disable developer mode
                DisableDeveloperModeRequest disableRequest = {};
                result = m_devModeControlDevice.MakeDevModeRequest(&disableRequest);
                if (result == Result::Success)
                {
                    result = disableRequest.header.result;
                }

                // Destroy the control device (even if we fail to disable developer mode)
                m_devModeControlDevice.Destroy();
            }

            if (result == Result::Success)
            {
                m_isEnabled = false;
            }
        }

        return result;
    }

    DD_NODISCARD Result LocalNgListenerTransport::GetKernelDevDriverVersion(uint32 &version, DevDriver::DeveloperModeFlags &features)
    {
        Result result = Result::Error;
        // m_devModeControlDevice may not be initialized at this time so create
        // a temporary local DevModeControlDevice for the version query
        DevModeControlDevice devModeControlDevice(m_allocCb);
        QueryCapabilitiesRequest capsRequest = {};

        result = devModeControlDevice.Initialize(DevModeBusType::Auto);

        if (result == Result::Success)
        {
            result = devModeControlDevice.MakeDevModeRequest(&capsRequest);
            if (result == Result::Success)
            {
                result = capsRequest.header.result;
                version = capsRequest.output.version;
                features = capsRequest.output.features;
            }

            devModeControlDevice.Destroy();
        }

        return result;
    }
} // DevDriver

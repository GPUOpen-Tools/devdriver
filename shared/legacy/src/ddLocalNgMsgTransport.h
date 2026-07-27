/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddDevModeControlDevice.h>
#include <ddDevModeQueue.h>
#include <msgTransport.h>

namespace DevDriver
{
    class LocalNgMsgTransport : public IMsgTransport
    {
    public:
        explicit LocalNgMsgTransport(
            const AllocCb& allocCb,
            Component      componentType,
            StatusFlags    initialFlags);
        ~LocalNgMsgTransport();

        Result Connect(ClientId* pClientId, uint32 timeoutInMs) override;
        Result Disconnect() override;

        Result ReadMessage(MessageBuffer& messageBuffer, uint32 timeoutInMs) override;
        Result WriteMessage(const MessageBuffer& messageBuffer) override;

        const char* GetTransportName() const override
        {
            return "Local Ng";
        }

        DD_STATIC_CONST bool RequiresKeepAlive()
        {
            return false;
        }

        DD_STATIC_CONST bool RequiresClientRegistration()
        {
            return false;
        }

        static Result TestConnection(const AllocCb& allocCb);
    private:
        bool IsConnected() const { return m_isConnected; }

        ClientId             m_clientId;
        Component            m_componentType;
        StatusFlags          m_initialClientFlags;
        DevModeControlDevice m_devModeControlDevice;
        AllocCb              m_allocCb;
        SharedQueue          m_sharedQueue;
        bool                 m_isConnected;

        DD_STATIC_CONST uint32 kTransmitTimeoutInMs = 50;
        DD_STATIC_CONST uint32 kReceiveTimeoutInMs = 50;
    };

} // DevDriver

/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <gpuopen.h>
#include <ddRouterInterface.h>
#include <router/ddRouterContext.h>
#include <router/ddAmdLogUtilsService.h>

namespace DevDriver
{
    // MsgRouter is expected to be the only implementation of IRouter.
    // We're only using an abstract interface here to avoid exposing implementation details.
    class MsgRouter final : public IRouter
    {
    public:
        MsgRouter(const AllocCb&         allocCb,
                  pfnNotifyKernalEnable  kernalEnableCb,
                  pfnNotifyKernalDisable kernalDisableCb);
        ~MsgRouter();

        const AllocCb& GetAllocCb() const { return m_allocCb; }

        Result Initialize(const RouterCreateInfo& createInfo);
        void Destroy();

        void RegisterServices(size_t servicesCount, IService* const* pServices);

		Kmd::KContext* GetContext() { return &m_context; }

        void SignalDriverResetEvent();

        void SendNotificationToKmd(uint32_t notificationType, void *pData);

        Result ProcessDevModeCmd(ProcessId processId, DevModeCmd cmd, size_t bufferSize, void* pBuffer) override;

        void OnProcessClose(ProcessId processId) override;

        void RegisterRpcServices();
        void DestroyRpcServices();

    private:
        AllocCb                m_allocCb;
        Kmd::KContext          m_context;
        Vector<IService*>      m_servicesToRegister;
        pfnNotifyKernalEnable  m_kernalEnableCb;
        pfnNotifyKernalDisable m_kernalDisableCb;
        ProcessId              m_devDriverPID;
        // m_kernalEnableCb() and m_kernalDisableCb() modify global OS state
        // and we must not call m_kernalEnableCb() twice without calling
        // m_kernalDisableCb() in between.
        // m_kernalEnableState tracks whether we have called m_kernalEnableCb()
        // so we can follow the correct sequence.
        bool                   m_kernalEnableState;
        AmdLogEventVersion     m_version;
        AmdLogUtilsService::AmdLogUtilsService  m_amdLogUtilsService;
        DDRpcServer                             m_rpcServer;
    };

} // DevDriver

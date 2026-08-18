/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddRouter.h>

#include <ddCommon.h>
#include <gpuopen.h>

#include <toolModule.h>

#include <ddRpcServer.h>
#include <ddEventServer.h>

#include <ddNet.h>

using DDTool::ToolModule;

/// Category for Router logging
constexpr const char kRouterCategory[] = "ddRouter";

#define DD_ROUTER_LOG(logger, level, message)   DD_API_LOG(logger, level, kRouterCategory, message)
#define DD_ROUTER_LOGF(logger, level, fmt, ...) DD_API_LOGF(logger, level, kRouterCategory, fmt, __VA_ARGS__)

// TODO: Enable and test DD_API macros for Push/Pop semantics
#define DD_ROUTER_PUSHF(logger, level, fmt, ...) DD_ROUTER_LOGF(logger, level, fmt, __VA_ARGS__)
#define DD_ROUTER_PUSH(logger)
#define DD_ROUTER_POP(logger)

namespace DevDriver
{
class IListener;
}

struct RouterCreateInfo
{
    ApiAllocCallbacks apiAlloc;                                  /// ddRouter API allocation callbacks
    DDLoggerInfo      logger;                                    /// Logging callbacks
    char              pDescription[DevDriver::kMaxStringLength]; /// Description of the router
};

class Router
{
public:
    Router(const RouterCreateInfo& createInfo);
    ~Router();

    static DD_RESULT Create(const DDRouterCreateInfo& routerInfo, Router** ppOutRouter);

    const DevDriver::AllocCb& DDAlloc() { return m_ddAlloc; }
    const ApiAllocCallbacks   ApiAlloc() { return m_createInfo.apiAlloc; }

    /// Loads a built-in module
    DD_RESULT LoadBuiltinModule(const DDModuleInterface* pInterface, DDModuleLoadedInfo* pLoadedInfo);

    /// Loads a dynamic module from the provided path
    DD_RESULT LoadDynamicModule(const char* pModulePath, DDModuleLoadedInfo* pLoadedInfo);

    /// Unloads a module that was previously loaded based on its context handle
    DD_RESULT UnloadModule(DDModuleContext hContext);

private:
    // Connect a module to the network
    // After this call, the Router owns the module pointer and will control its lifetime.
    DD_RESULT ConnectModule(ToolModule* pModule);

    // Disconnect and unload a module from the network
    // This will destroy the object pointed to by pModule, but not modify any other Router state.
    void DestroyModule(ToolModule* pModule);

    RouterCreateInfo               m_createInfo;   /// Creation information
    DevDriver::AllocCb             m_ddAlloc;      /// DevDriver allocation callbacks
    DevDriver::IListener*          m_pListener;    /// Pointer to the current listener object
    LoggerUtil                     m_logger;       /// Log callback used to emit log messages from the api
    DDNetConnection                m_hConnection;  /// Local connection to the network owned by the router
    DDRpcServer                    m_hRpcServer;   /// Handle to an RPC Server that the router manages for modules
    DDEventServer                  m_hEventServer; /// Handle to an Event Server that the router manages for modules
    DevDriver::Vector<ToolModule*> m_modules;      /// All loaded modules
};

DD_DEFINE_HANDLE(DDRouter, Router*);

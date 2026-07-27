/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddApi.h>

/// Compile time version information
#define MERCURY_EXTENSION_VERSION_MAJOR 0
#define MERCURY_EXTENSION_VERSION_MINOR 14
#define MERCURY_EXTENSION_VERSION_PATCH 0

/// Name of the extension
#define MERCURY_EXTENSION_NAME "Mercury"

/// Description of the extension
#define MERCURY_EXTENSION_DESCRIPTION "Allows Mercury to create Qt widgets from modules"

/// Identifier for the extension
/// This identifier is used to acquire access to the extension's interface
#define MERCURY_EXTENSION_ID 0x007972756372656d

/// Opaque handle that represents a Qt widget
typedef struct MercuryQtWidget_t* MercuryQtWidget;

/// Opaque handle that represents a model for a mercury widget
typedef struct MercuryModel_t* MercuryModel;

/// Opaque handle that represents a Qt settings object
typedef struct MercuryQtSettings_t* MercuryQtSettings;

/// Function prototype used by Mercury to push the log stack
typedef void(*PFN_MercuryLogPush)(void* pUserdata);

/// Function prototype used by Mercury to append log information
typedef void(*PFN_MercuryLogAppend)(void* pUserdata, DD_LOG_LEVEL level, const char* pMsg);

/// Function prototype used by Mercury to pop the log stack
typedef void(*PFN_MercuryLogPop)(void* pUserdata);

/// Function prototype used by Mercury utility view to signal value changed
/// A utility view would use this to signal to the consuming tool that a value
/// has been modified. As a result, the tool may want to trigger serialization
/// of the new values, or enable/disable UI in response.
typedef void (*PFN_MercuryUtilityViewValueChanged)(void* pUserData);

typedef enum MercuryClientApi : uint32_t
{
    MERCURY_CLIENT_API_DIRECTX12 = 0,
    MERCURY_CLIENT_API_DIRECTX11,
    MERCURY_CLIENT_API_DIRECTX9,
    MERCURY_CLIENT_API_HIP,
    MERCURY_CLIENT_API_OPENCL,
    MERCURY_CLIENT_API_OPENGL,
    MERCURY_CLIENT_API_VULKAN,
    MERCURY_CLIENT_API_COUNT
} MercuryClientApi;

/// Structure that contains information about the currently selected application
typedef struct MercurySelectedAppInfo
{
    char name[DD_API_PATH_SIZE];
    char description[DD_API_PATH_SIZE];
} MercurySelectedAppInfo;

/// Structure that contains extra information about the module
typedef struct MercuryModuleInfo
{
    char displayName[DD_API_PATH_SIZE]; /// Human friendly name of the module
} MercuryModuleInfo;

/// Structure that contains information about the log callback exposed by this extension
typedef struct MercuryLogCallbackInfo
{
    void*                pUserdata;
    PFN_MercuryLogPush   pfnLogPushCallback;
    PFN_MercuryLogAppend pfnLogAppendCallback;
    PFN_MercuryLogPop    pfnLogPopCallback;
} MercuryLogCallbackInfo;

/// Function prototype used by Mercury to acquire information about the currently selected app
typedef void(*PFN_MercuryQuerySelectedAppInfo)(void* pUserdata, MercurySelectedAppInfo* pAppInfo);

/// Structure that contains information about the utility callbacks exposed by this extension
typedef struct MercuryUtilityCallbackInfo
{
    void*                           pUserdata;
    PFN_MercuryQuerySelectedAppInfo pfnQuerySelectedAppInfo;
} MercuryUtilityCallbackInfo;

/// Structure that contains information about the code that loaded this extension
typedef struct MercuryLoaderInterface
{
    MercuryLogCallbackInfo     logCb;
    MercuryUtilityCallbackInfo utilityCb;
    MercuryQtSettings          hGlobalSettings;
    MercuryQtSettings          hLocalSettings;
} MercuryLoaderInterface;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Returns the QT_VERSION value that the module was linked against
typedef uint32_t (*PFN_MercuryQueryQtVersion)(void);

/// Returns extra information about the module that exposes this extension
typedef void (*PFN_MercuryQueryModuleInfo)(
    MercuryModuleInfo* pModuleInfo); /// [out] Information about the module

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// System API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Structure that contains the information required to create a system view
typedef struct MercurySystemViewCreateInfo
{
    const MercuryLoaderInterface* pLoaderInterface; /// [in] Pointer to a loader information structure
    DDModuleDataContext           hDataContext;     /// Data Context Handle associated with the view
} MercurySystemViewCreateInfo;

/// Structure that contains the information required to update a system view
typedef struct MercurySystemViewUpdateInfo
{
    DDModuleSystemContext hContext; /// System Context Handle
} MercurySystemViewUpdateInfo;

/// Attempts to create a Qt system view widget for the module
typedef MercuryQtWidget (*PFN_MercuryCreateQtModuleSystemView)(
    const MercurySystemViewCreateInfo* pCreateInfo); /// [in] Pointer to a creation information structure

/// Destroys a previously created Qt system view widget
typedef void (*PFN_MercuryDestroyQtModuleSystemView)(
    MercuryQtWidget hQtWidget); /// [in] Handle to an existing Qt system view widget

/// Updates a previously created Qt system view widget
/// System views can visualize a system context and a data context.
/// Their main purpose is to expose functionality at the scope of the entire system
typedef void (*PFN_MercuryUpdateQtModuleSystemView)(
    MercuryQtWidget                    hQtWidget, /// [in] Handle to an existing Qt system view widget
    const MercurySystemViewUpdateInfo* pInfo);    /// [in] System view update info structure

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Client API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Structure that contains the information required to create a client view
typedef struct MercuryClientViewCreateInfo
{
    /// Name of the application associated with this client
    const char*                   pAppName;
    /// Pointer to a loader information structure
    const MercuryLoaderInterface* pLoaderInterface;
    /// Module Data Context Handle associated with the view
    DDModuleDataContext           hDataContext;
    /// Module System Context Handle associated with the view
    DDModuleSystemContext         hSystemContext;
} MercuryClientViewCreateInfo;

/// Structure that contains the information required to create a client view model.
typedef struct MercuryClientViewModelCreateInfo
{
    /// Name of the application associated with this client
    const char*                   pAppName;
    /// Pointer to a loader information structure
    const MercuryLoaderInterface* pLoaderInterface;
    /// Module Data Context Handle associated with the view
    DDModuleDataContext           hDataContext;
    /// Module System Context Handle associated with the view
    DDModuleSystemContext         hSystemContext;
} MercuryClientViewModelCreateInfo;

/// Structure that contains the information required to update a client view
typedef struct MercuryClientViewUpdateInfo
{
    const char*           pDescription; /// Client driver description
    DDModuleClientContext hContext;     /// Client Context Handle
} MercuryClientViewUpdateInfo;

/// Attempts to create a Qt client view widget for the module
typedef MercuryQtWidget (*PFN_MercuryCreateQtModuleClientView)(
    const MercuryClientViewCreateInfo* pCreateInfo); /// [in] Pointer to a creation information structure

/// Destroys a previously created Qt client view widget
typedef void (*PFN_MercuryDestroyQtModuleClientView)(
    MercuryQtWidget hQtWidget); /// [in] Handle to an existing Qt client view widget

/// Updates a previously created Qt client view widget
/// Client views can visualize a client context and a data context.
/// Their main purpose is to expose functionality at the scope of an individual client
typedef void (*PFN_MercuryUpdateQtModuleClientView)(
    MercuryQtWidget                    hQtWidget, /// [in] Handle to an existing Qt client view widget
    const MercuryClientViewUpdateInfo* pInfo);    /// [in] Client view update info structure

typedef bool (*PFN_MercuryIsClientApiSupported)(
    MercuryClientApi api); /// [in] the client api

/// @brief Creates a new model for a Qt client view.
typedef MercuryModel (*PFN_MercuryCreateQtModuleClientViewModel)(
    const MercuryClientViewModelCreateInfo* pCreateInfo); ///< [in] Pointer to a creation information structure.

/// @brief Updates the model used by the client view.
///
/// Once this is called on a view, the view becomes responsible for managing the lifecycle of the model.
/// It does not need to manually be destroyed.
typedef void (*PFN_MercuryUpdateQtModuleClientViewModel)(
    MercuryQtWidget hQtWidget, ///< [in] Handle to an existing Qt client view widget.
    MercuryModel   model);      /// [in] The model that the widget should use.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Structure that contains the information required to create a utility view
typedef struct MercuryUtilityViewCreateInfo
{
    void*                              pUserData;        /// User data passed to value changed callback
    PFN_MercuryUtilityViewValueChanged pfnValueChanged;  /// Value changed callback
    const MercuryLoaderInterface*      pLoaderInterface; /// [in] Pointer to a loader information structure
    DDModuleDataContext                hDataContext;     /// Data Context Handle associated with the view
} MercuryUtilityViewCreateInfo;

/// Structure that contains the information required to update a utility view
typedef struct MercuryUtilityViewUpdateInfo
{
    /// Module Data Context Handle associated with the view
    DDModuleDataContext           hDataContext;
} MercuryUtilityViewUpdateInfo;

/// Attempts to create a Qt utility view widget for the module
typedef MercuryQtWidget (*PFN_MercuryCreateQtModuleUtilityView)(
    const MercuryUtilityViewCreateInfo* pCreateInfo); /// [in] Utility view create info

/// Destroys a previously created Qt utility view widget
typedef void (*PFN_MercuryDestroyQtModuleUtilityView)(
    MercuryQtWidget hQtWidget); /// [in] Handle to an existing Qt utility view widget

/// Updates a previously created Qt utility view widget
typedef void (*PFN_MercuryUpdateQtModuleUtilityView)(
    MercuryQtWidget hQtWidget,                  /// [in] Handle to an existing Qt utility view widget
    const MercuryUtilityViewUpdateInfo* pInfo); /// [in] Utility view update info structure

typedef struct MercuryModuleExtSystemApi_0000
{
    PFN_MercuryCreateQtModuleSystemView  pfnCreateQtModuleSystemView;
    PFN_MercuryDestroyQtModuleSystemView pfnDestroyQtModuleSystemView;
    PFN_MercuryUpdateQtModuleSystemView  pfnUpdateQtModuleSystemView;
} MercuryModuleExtSystemApi_0000;

typedef struct MercuryModuleExtClientApi_0000
{
    PFN_MercuryCreateQtModuleClientView  pfnCreateQtModuleClientView;
    PFN_MercuryDestroyQtModuleClientView pfnDestroyQtModuleClientView;
    PFN_MercuryUpdateQtModuleClientView  pfnUpdateQtModuleClientView;

    PFN_MercuryCreateQtModuleClientViewModel pfnMercuryCreateClientViewModel;
    PFN_MercuryUpdateQtModuleClientViewModel pfnMercuryUpdateQtModuleClientViewModel;
    PFN_MercuryIsClientApiSupported          pfnIsClientApiSupported;
} MercuryModuleExtClientApi_0000;

typedef struct MercuryModuleExtUtilityApi_0000
{
    PFN_MercuryCreateQtModuleUtilityView  pfnCreateQtModuleUtilityView;
    PFN_MercuryDestroyQtModuleUtilityView pfnDestroyQtModuleUtilityView;
    PFN_MercuryUpdateQtModuleUtilityView  pfnUpdateQtModuleUtilityView;
} MercuryModuleExtUtilityApi_0000;

/// Mercury Extension API
typedef struct MercuryModuleExtApi_0000
{
    const MercuryModuleExtSystemApi_0000*  pSystemApi;
    const MercuryModuleExtClientApi_0000*  pClientApi;
    const MercuryModuleExtUtilityApi_0000* pUtilityApi;
    PFN_MercuryQueryQtVersion              pfnQueryQtVersion;
    PFN_MercuryQueryModuleInfo             pfnQueryModuleInfo;
} MercuryModuleExtApi_0000;

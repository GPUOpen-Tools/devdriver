/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <ddModule.h>
#include <ModuleLogger.h>

#include <util/hashMap.h>
#include <util/vector.h>

namespace DevDriver
{
    class StructuredValue;
    class IStructuredWriter;
}

/// Class used to encapsulate the module specific per client state
class BaseModuleDataContext
{
public:
    /// Returns the loader interface used to create this object
    const DDModuleLoaderInterface& GetLoader() const { return m_createInfo.loader; }

    /// Attempts to initialize this client context so it can communicate with the client provided in the create info
    virtual DD_RESULT Initialize(
        const void*  pData,     /// [Optional]
                                /// [in] Pointer to memory that contains a previously serialized data context
        size_t       dataSize); /// Size of the memory pointed to by pData

    /// Serializes the context data into the specified output buffer
    virtual DD_RESULT Serialize(
        void*   pData,            /// [out] Pointer to memory where the data should be written to
        size_t* pDataSize) const; /// [in/out] Pointer to a size_t value that contains the size of pData
                                  ///          If pData is nullptr, this will return the required size in bytes
                                  ///          If pData is valid, this indicates the size of the memory at pData

    /// Returns the name of the module
    const char* GetModuleName() const { return m_pModuleName; }

    static DD_RESULT UpdateUserdataNode(
        DDModuleDataContext hDataContext,
        const char*         pNodeName,
        const void*         pBytes,
        size_t              bytesSize);

    static DD_RESULT QueryUserdataNode(
        DDModuleDataContext  hDataContext,
        const char*          pNodeName,
        void*                pUserdata,
        PFN_ddReceiveBinary  pfnReceiveBytes);

protected:
    BaseModuleDataContext(const DDModuleDataContextCreateInfo& createInfo, const char* pModuleName, const DDApiVersion& serializationVersion);
    virtual ~BaseModuleDataContext();

    /// This base class is responsible for serialization and parsing of the the JSON document containing serialized
    /// data.  The Serialize/Initialize functions will write/validate a common header containing the module name
    /// and data version, then pass control to the Serialize/Deserialize for derived classes to add/parse their
    /// module specific serialized JSON data
    virtual void SerializeModuleData(DevDriver::IStructuredWriter* pWriter) const { DD_API_UNUSED(pWriter); }
    virtual DD_RESULT DeserializeModuleData(const DevDriver::StructuredValue& moduleData)
    {
        DD_API_UNUSED(moduleData);
        return DD_RESULT_SUCCESS;
    }

    DD_RESULT UpdateUserdataNode(
        const char* pNodeName,
        const void* pBytes,
        size_t      bytesSize);

    DD_RESULT QueryUserdataNode(
        const char*         pNodeName,
        void*               pUserdata,
        PFN_ddReceiveBinary pfnReceive);

    DDModuleDataContextCreateInfo m_createInfo;

private:
    void WriteDataHeader(DevDriver::IStructuredWriter* pWriter) const;
    DD_RESULT ValidateDataHeader(const DevDriver::StructuredValue& serializedHeader) const;

    /// Internal representation of a userdata node.
    /// These nodes are serialized with the data context, but can be queried from ddModule users.
    /// These nodes own their name and data in allocations, but have a large buffer to store the data inline in the common cases.
    struct UserdataNode
    {
        UserdataNode(DevDriver::AllocCb alloc) : name(alloc), data(alloc) {}

        DevDriver::Vector<char,    128>  name; // A name to uniquely identify the node for a ddModule user
        DevDriver::Vector<uint8_t, 1024> data; // The data that can be retrieved by a ddModule user
    };

    ///////////////////////// Member Data //////////////////////////////////////
protected:
    ModuleLogger                               m_logger;
private:
    const char*                                m_pModuleName;
    DDApiVersion                               m_dataVersion;
    DevDriver::AllocCb                         m_ddAlloc;
    DevDriver::HashMap<uint32_t, UserdataNode> m_userdataNodes;

};

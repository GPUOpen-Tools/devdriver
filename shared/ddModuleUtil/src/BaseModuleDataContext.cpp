/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <BaseModuleDataContext.h>
#include <util/ddStructuredReader.h>
#include <ddUriInterface.h>

#include <ddUriInterface.h>
#include <util/ddJsonWriter.h>
#include <util/ddStructuredReader.h>
#include <util/vector.h>

#include <gpuopen.h>
#include <ddCommon.h>

using namespace DevDriver;

// Hash a String like the default hash function does for a HashMap<>.
static DefaultHashFunc<const char*> kHashStr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleDataContext::BaseModuleDataContext(
    const DDModuleDataContextCreateInfo& createInfo,
    const char*                          pModuleName,
    const DDApiVersion&                  serializationVersion)
    : m_createInfo(createInfo)
    , m_logger(createInfo.loader)
    , m_pModuleName(pModuleName)
    , m_dataVersion(serializationVersion)
    , m_ddAlloc({&m_createInfo.loader.apiAllocCb, ddApiAlloc, ddApiFree})
    , m_userdataNodes(m_ddAlloc)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseModuleDataContext::~BaseModuleDataContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization keys
static const char* kHeaderMapStr        = "SerializedDataHeader";
static const char* kModuleNameStr       = "ModuleName";
static const char* kDataVersionStr      = "DataVersion";
static const char* kMajorVersionStr     = "Major";
static const char* kMinorVersionStr     = "Minor";
static const char* kPatchVersionStr     = "Patch";
static const char* kDataStr             = "ModuleData";
static const char* kUserdataNodesStr    = "UserdataNodes";
static const char* kUserdataNodeNameStr = "NodeName";
static const char* kUserdataStr         = "UserdataStr";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleDataContext::Initialize(
    const void* pData,
    size_t      dataSize)
{
    // Initialize to success because initial serialized data is not required
    DD_RESULT result = DD_RESULT_SUCCESS;

    if ((pData != nullptr) && (dataSize > 0))
    {
        // Extract a DevDriver AllocCb from our loader
        ApiAllocCallbacks apiAllocCb;
        AllocCb allocCb;
        ConvertAllocCallbacks(m_createInfo.loader.apiAllocCb, &apiAllocCb, &allocCb);

        IStructuredReader* pReader;
        result = DevDriverToDDResult(IStructuredReader::CreateFromJson(static_cast<const char*>(pData),
                                                                       dataSize,
                                                                       allocCb,
                                                                       &pReader));
        // First validate the data header
        StructuredValue root;
        if (result == DD_RESULT_SUCCESS)
        {
            root = pReader->GetRoot();
            if (root.IsNull() || root[kHeaderMapStr].IsNull() || root[kDataStr].IsNull())
            {
                result = DD_RESULT_PARSING_INVALID_JSON;
            }
        }

        if (result == DD_RESULT_SUCCESS)
        {
            result = ValidateDataHeader(root[kHeaderMapStr]);
        }

        // Then pass the rest of the serialized data to the derived class to deserialize the module specific data
        if (result == DD_RESULT_SUCCESS)
        {
            result = DeserializeModuleData(root[kDataStr]);
        }

        // Finally, parse out any serialized data nodes
        if (root[kUserdataNodesStr].IsArray())
        {
            for (size_t i = 0; i < root[kUserdataNodesStr].GetArrayLength(); ++i)
            {
                const auto& node = root[kUserdataNodesStr][i];
                DD_ASSERT(!node.IsNull());

                /// For now we expect this payload to be Json.
                const char* pBytes = node[kUserdataStr].GetStringPtr();
                DD_ASSERT(pBytes != nullptr);
                // Reasonable maximum for JSON userdata (1MB)
                constexpr size_t kMaxUserdataJsonSize = 1024 * 1024;
                const size_t bytesSize = (pBytes != nullptr) ? Platform::Strlen_s(pBytes, kMaxUserdataJsonSize) : 0;
                // Assert if JSON data exceeds our assumed maximum (would indicate truncation)
                DD_ASSERT(bytesSize < kMaxUserdataJsonSize);

                const char* pNodeName = node[kUserdataNodeNameStr].GetStringPtr();

                UpdateUserdataNode(pNodeName, pBytes, bytesSize);
            }
        }

        IStructuredReader::Destroy(&pReader);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleDataContext::Serialize(
    void*   pData,
    size_t* pDataSize) const
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (pDataSize != nullptr)
    {
        // Extract a DevDriver AllocCb from our loader
        ApiAllocCallbacks apiAllocCb;
        AllocCb allocCb;
        ConvertAllocCallbacks(m_createInfo.loader.apiAllocCb, &apiAllocCb, &allocCb);

        Vector<char> dataBuffer(allocCb);
        JsonWriter   jsonWriter(&dataBuffer);

        // First write the common data header
        jsonWriter.BeginMap();
        {
            WriteDataHeader(&jsonWriter);

            jsonWriter.KeyAndBeginMap(kDataStr);
            {
                // Then call the derived class to write the module specific data
                SerializeModuleData(&jsonWriter);
            }
            jsonWriter.EndMap();
        }

        // Finally add any userdata nodes
        if (m_userdataNodes.Size() > 0)
        {
            jsonWriter.KeyAndBeginList(kUserdataNodesStr);
            for (auto iter = m_userdataNodes.Begin(); iter != m_userdataNodes.End(); ++iter)
            {
                jsonWriter.BeginMap();
                {
                    const UserdataNode& node = iter->value;
                    jsonWriter.KeyAndValue(kUserdataNodeNameStr, node.name.Data());

                    // TODO: We assume that this is a string and serialize it like one.
                    const char* pJson = reinterpret_cast<const char*>(node.data.Data());
                    jsonWriter.KeyAndValue(kUserdataStr, pJson, node.data.Size());
                }
                jsonWriter.EndMap();
            }
            jsonWriter.EndList();
        }

        // Finally complete the JSON document
        jsonWriter.EndMap();
        result = DevDriverToDDResult(jsonWriter.End());

        // Report the string length as the data size to ensure we don't include the null byte that's added by the
        // json writer logic.
        // Use the vector size as the maximum (it includes null terminator)
        const size_t dataSize = Platform::Strlen_s(dataBuffer.Data(), dataBuffer.Size());

        // A zero length string is not a valid json document
        DD_ASSERT(dataSize > 0);

        if (result == DD_RESULT_SUCCESS)
        {
            if ((*pDataSize) >= dataSize)
            {
                if (pData != nullptr)
                {
                    Platform::Memcpy_s(pData, *pDataSize, dataBuffer.Data(), dataSize);
                }
                else
                {
                    result = DD_RESULT_COMMON_INVALID_PARAMETER;
                }
            }
            else if (pData != nullptr)
            {
                result = DD_RESULT_COMMON_BUFFER_TOO_SMALL;
            }
        }

        (*pDataSize) = dataSize;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writes a serialized data JSON header using the JSON write and data provided.
void BaseModuleDataContext::WriteDataHeader(
    IStructuredWriter* pWriter) const
{
    // Start Header
    pWriter->KeyAndBeginMap(kHeaderMapStr);
    {
        // Module Name
        pWriter->KeyAndValue(kModuleNameStr, m_pModuleName);

        // Data Version
        pWriter->KeyAndBeginMap(kDataVersionStr);
        {
            pWriter->KeyAndValue(kMajorVersionStr, m_dataVersion.major);
            pWriter->KeyAndValue(kMinorVersionStr, m_dataVersion.minor);
            pWriter->KeyAndValue(kPatchVersionStr, m_dataVersion.patch);
        }
        pWriter->EndMap();
    }
    // End Header
    pWriter->EndMap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates the serializedHeader stored in the StructuredValue using the data provided in expectedHeader.
DD_RESULT BaseModuleDataContext::ValidateDataHeader(
    const StructuredValue& serializedHeader) const
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if (serializedHeader.IsMap())
    {
        result = DD_RESULT_SUCCESS;

        const char* pModuleName = serializedHeader[kModuleNameStr].GetStringPtr();
        if ((pModuleName == nullptr) ||
            (strcmp(pModuleName, m_pModuleName) != 0))
        {
            result = DD_RESULT_PARSING_INVALID_STRUCTURE;
        }

        if (result == DD_RESULT_SUCCESS)
        {
            DDApiVersion moduleDataVersion = {};
            bool success = serializedHeader[kDataVersionStr][kMajorVersionStr].GetUint32(&moduleDataVersion.major);
            success     &= serializedHeader[kDataVersionStr][kMinorVersionStr].GetUint32(&moduleDataVersion.minor);
            success     &= serializedHeader[kDataVersionStr][kPatchVersionStr].GetUint32(&moduleDataVersion.patch);
            if (success)
            {
                if (ddIsVersionCompatible(m_dataVersion, moduleDataVersion) == 0)
                {
                    result = DD_RESULT_COMMON_VERSION_MISMATCH;
                }
            }
            else
            {
                result = DD_RESULT_PARSING_INVALID_STRUCTURE;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleDataContext::UpdateUserdataNode(
    DDModuleDataContext hDataContext,
    const char*         pNodeName,
    const void*         pBytes,
    size_t              bytesSize)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    BaseModuleDataContext* pContext = reinterpret_cast<BaseModuleDataContext*>(hDataContext);
    if ((pNodeName != nullptr) && (pContext != nullptr))
    {
        result = pContext->UpdateUserdataNode(pNodeName, pBytes, bytesSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleDataContext::QueryUserdataNode(
    DDModuleDataContext hDataContext,
    const char*         pNodeName,
    void*               pUserdata,
    PFN_ddReceiveBinary pfnReceiveBytes)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    BaseModuleDataContext* pContext = reinterpret_cast<BaseModuleDataContext*>(hDataContext);
    if ((pNodeName != nullptr) && (pfnReceiveBytes != nullptr) && (pContext != nullptr))
    {
        result = pContext->QueryUserdataNode(pNodeName, pUserdata, pfnReceiveBytes);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleDataContext::UpdateUserdataNode(
    const char* pNodeName,
    const void* pBytes,
    size_t      bytesSize)
{
    const uint32 nodeNameHash = kHashStr(pNodeName);
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;

    if ((pBytes != nullptr) && (bytesSize > 0))
    {
        result = DD_RESULT_SUCCESS;

        // If we're given data, we will leave the node in place and update it with new data.
        UserdataNode* pNode = m_userdataNodes.FindValue(nodeNameHash);
        if (pNode == nullptr)
        {
            // The node did not exist, so create and insert it now.
            result = DevDriverToDDResult(m_userdataNodes.Create(nodeNameHash, m_ddAlloc));

            if (result == DD_RESULT_SUCCESS)
            {
                pNode = m_userdataNodes.FindValue(nodeNameHash);

                // Name is already NULL-terminated, so copy it wholesale.
                // Reasonable maximum for node name
                constexpr size_t kMaxNodeNameLength = 256;
                const size_t nodeNameLen = Platform::Strlen_s(pNodeName, kMaxNodeNameLength);
                // Assert if node name exceeds our assumed maximum (would indicate truncation)
                DD_ASSERT(nodeNameLen < kMaxNodeNameLength);
                pNode->name.Append(pNodeName, nodeNameLen + 1);
            }
        }

        if ((result == DD_RESULT_SUCCESS) && (pNode != nullptr))
        {
            // Destroy the old data when updating with the new data. No one should be using this data anymore.
            pNode->data.Reset();
            pNode->data.Append(reinterpret_cast<const uint8*>(pBytes), bytesSize);

            // It is an error to insert an empty userdata node. Sanity check that this never happens.
            DD_ASSERT(!pNode->data.IsEmpty());
        }
        else
        {
            // We should have inserted the node by now. If it's still NULL, we failed to insert it.
            // The likely (only?) candidate for this is out of memory conditions.
            result = DD_RESULT_COMMON_OUT_OF_HEAP_MEMORY;
        }
    }
    else
    {
        // If we're not given data, we will remove the entire node, data and all.
        auto iter = m_userdataNodes.Find(nodeNameHash);
        if (iter != m_userdataNodes.End())
        {
            m_userdataNodes.Remove(iter);
            result = DD_RESULT_SUCCESS;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DD_RESULT BaseModuleDataContext::QueryUserdataNode(
    const char*         pNodeName,
    void*               pUserdata,
    PFN_ddReceiveBinary pfnReceiveBytes)
{
    DD_RESULT result = DD_RESULT_COMMON_INVALID_PARAMETER;
    if ((pNodeName != nullptr) && (pfnReceiveBytes != nullptr))
    {
        const uint32 nodeNameHash = kHashStr(pNodeName);

        UserdataNode* pNode = m_userdataNodes.FindValue(nodeNameHash);
        if (pNode != nullptr)
        {
            // pNode->data is not allowed to be empty at this point. We enforce this invariant in UpdateUserdataNode().
            DD_ASSERT(pNode->data.Size() != 0);

            pfnReceiveBytes(pUserdata, pNode->data.Data(), pNode->data.Size());

            result = DD_RESULT_SUCCESS;
        }
        else
        {
            result = DD_RESULT_COMMON_DOES_NOT_EXIST;
        }
    }

    return result;
}

/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include "ddUriInterface.h"

#include <cstring>

namespace DevDriver
{
    void IStructuredWriter::KeyAndBeginMap(const char* pKey)
    {
        Key(pKey);
        BeginMap();
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, const char* pValue)
    {
        Key(pKey);
        Value(pValue);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, const char* pValue, size_t length)
    {
        Key(pKey);
        Value(pValue, length);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, uint64 value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, uint32 value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, int64 value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, int32 value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, double value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, float value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValue(const char* pKey, bool value)
    {
        Key(pKey);
        Value(value);
    }

    void IStructuredWriter::KeyAndValueNull(const char* pKey)
    {
        Key(pKey);
        ValueNull();
    }

    PostDataInfo::PostDataInfo()
    {
        memset(this, 0, sizeof(*this));
    }
}

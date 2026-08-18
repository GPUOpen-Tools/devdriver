/* Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <yaml.h>
#include <stdint.h>

namespace DevDriver
{

/// Get the YAML node from a YAML document.
/// `pDoc` is the enclosing YAML document.
/// `pParent` is the parent YAML node. It must be a mapping node.
/// `pKey` is the mapping key to which the desired YAML node is associated with.
yaml_node_t* YamlDocumentFindNodeByKey(
    yaml_document_t* pDoc,
    yaml_node_t* pParent,
    const char* pKey);

/// Get the boolean value from a YAML node. Return true if the data contained
/// in the node can be converted to the desired value, false otherwise.
bool YamlNodeGetScalar(yaml_node_t* pValNode, bool* pOutValue);

/// Get the int8_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, int8_t* pOutValue);

/// Get the uint8_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, uint8_t* pOutValue);

/// Get the int16_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, int16_t* pOutValue);

/// Get the uint16_t value from a YAML node. The result is clamped to fit the range of the desired integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, uint16_t* pOutValue);

/// Get the int32_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, int32_t* pOutValue);

/// Get the uint32_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, uint32_t* pOutValue);

/// Get the int64_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, int64_t* pOutValue);

/// Get the uint64_t value from a YAML node. The result is clamped to fit the range of its integer type.
/// Return whether the parsing succeeded or not. If parsing failed, pOutValue is set to 0.
bool YamlNodeGetScalar(yaml_node_t* pValNode, uint64_t* pOutValue);

/// Get the float value from a YAML node. Return true if the data contained in
/// the node can be converted to the desired value, false otherwise.
bool YamlNodeGetScalar(yaml_node_t* pValNode, float* pOutValue);

} // namespace DevDriver

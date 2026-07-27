/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

#include <cstdint>

/// How long to wait before timing out when connecting to a router.
constexpr uint32_t kRouterConnectionTimeoutMillisec = 5000;

/// How long to wait before timing out when connection driver control for initialization.
constexpr uint32_t kDriverControlConnectTimeoutMillisec = 1000;


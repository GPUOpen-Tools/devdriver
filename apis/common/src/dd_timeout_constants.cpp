/* Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <dd_timeout_constants.h>

static const uint32_t kDefaultRetryTimeoutInMs         = 50;
static const uint32_t kDefaultCommunicationTimeoutInMs = 5000;
static const uint32_t kDefaultConnectionTimeoutInMs    = 1000;

TimeoutConstants g_timeoutConstants = { kDefaultConnectionTimeoutInMs,
                                        kDefaultRetryTimeoutInMs,
                                        kDefaultCommunicationTimeoutInMs };

DD_RESULT TimeoutConstantsInitialize(const TimeoutConstants* pTimeouts)
{
    g_timeoutConstants.connectionTimeoutInMs =
        (pTimeouts->connectionTimeoutInMs == 0) ? kDefaultConnectionTimeoutInMs : pTimeouts->connectionTimeoutInMs;

    g_timeoutConstants.retryTimeoutInMs =
        (pTimeouts->retryTimeoutInMs == 0) ? kDefaultRetryTimeoutInMs : pTimeouts->retryTimeoutInMs;

    g_timeoutConstants.communicationTimeoutInMs = (pTimeouts->communicationTimeoutInMs == 0) ?
                                                      kDefaultCommunicationTimeoutInMs :
                                                      pTimeouts->communicationTimeoutInMs;

    return DD_RESULT_SUCCESS;
}

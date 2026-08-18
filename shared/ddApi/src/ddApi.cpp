/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddApi.h>

extern "C" {

int ddIsVersionValid(
    DDApiVersion version)
{
    return ((version.major != 0) ||
            (version.minor != 0) ||
            (version.patch != 0));
}

int ddIsVersionCompatible(
    DDApiVersion requiredVersion,
    DDApiVersion actualVersion)
{
    const int isRequiredVersionValid = ddIsVersionValid(requiredVersion);

    const uint32_t requiredMajorVersion = ((requiredVersion.major != 0) ? requiredVersion.major
                                                                        : requiredVersion.minor);

    const int isActualVersionValid = ddIsVersionValid(actualVersion);

    const uint32_t actualMajorVersion = ((actualVersion.major != 0) ? actualVersion.major
                                                                    : actualVersion.minor);

    return ((isRequiredVersionValid && isActualVersionValid) &&
            (requiredMajorVersion == actualMajorVersion)      &&
            ((requiredVersion.minor < actualVersion.minor)    ||
             ((requiredVersion.minor == actualVersion.minor)  &&
              (requiredVersion.patch <= actualVersion.patch))));
}

int ddIsMajorVersionCompatible(
    DDApiVersion requiredVersion,
    DDApiVersion actualVersion)
{
    const int isRequiredVersionValid = ddIsVersionValid(requiredVersion);
    const int isActualVersionValid   = ddIsVersionValid(actualVersion);

    uint32_t requiredMajorVersion = requiredVersion.major;
    uint32_t actualMajorVersion   = actualVersion.major;

    if ((requiredVersion.major == 0) && (actualVersion.major == 0))
    {
        requiredMajorVersion = requiredVersion.minor;
        actualMajorVersion   = actualVersion.minor;
    }

    return ((isRequiredVersionValid && isActualVersionValid) &&
            (requiredMajorVersion   == actualMajorVersion));
}

} // extern "C"

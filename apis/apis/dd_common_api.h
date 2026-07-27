/* Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DD_COMMON_API_H
#define DD_COMMON_API_H

// TODO: in the far future when all legacy code is removed and all code lives under apis/ we should
// incorporate the content of ddApi.h in this file directly and remove ddApi.h.
#include <ddApi.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} DDVersion;

typedef union
{
    struct
    {
        uint32_t function : 8;
        uint32_t device   : 8;
        uint32_t bus      : 8;
        uint32_t reserved : 8;
    } bits;
    uint32_t u32All;
} PciLocation;

typedef uint16_t DDConnectionId;

/// GPU ID Determined from (BusID << 16) | (DeviceID << 8) | FunctionID
typedef uint32_t DDGpuId;

/// GPU ID used when it is unknown which GPU the message relates to
static const DDGpuId DDGpuIdUnknown = 0xFFFFFFFF;

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(__cplusplus) && !defined(DD_PLATFORM_IS_KM)

#include <string>

// Converts a PciLocation to a string ("bus%udev%ufunc%u")
inline std::string PciLocationToString(const PciLocation* pci)
{
    if (pci == nullptr)
    {
        return "busXdevYfuncZ";
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "bus%udev%ufunc%u", pci->bits.bus, pci->bits.device, pci->bits.function);
    return std::string(buf);
}

//Function to parse "bus%udev%ufunc%u" into bus, dev, func
inline bool ParseBusDevFuncString(const std::string& str, uint32_t& bus, uint32_t& dev, uint32_t& func)
{
    size_t busPos  = str.find("bus");
    size_t devPos  = str.find("dev");
    size_t funcPos = str.find("func");

    // Validate the presence and order of appearance
    // Expected format: "bus<N>dev<N>func<N>" where N is a decimal number
    if (busPos == std::string::npos || devPos == std::string::npos || funcPos == std::string::npos ||
        !(busPos < devPos && devPos < funcPos))
    {
        return false;
    }

    std::string busStr = str.substr(busPos + 3, devPos - (busPos + 3));
    std::string devStr = str.substr(devPos + 3, funcPos - (devPos + 3));
    std::string funcStr = str.substr(funcPos + 4);

    char* end = nullptr;

    // Parse bus (decimal)
    bus = static_cast<uint32_t>(std::strtoul(busStr.c_str(), &end, 10));
    if (end == busStr.c_str() || *end != '\0')
        return false;

    // Parse device (decimal)
    dev = static_cast<uint32_t>(std::strtoul(devStr.c_str(), &end, 10));
    if (end == devStr.c_str() || *end != '\0')
        return false;

    // Parse function (decimal)
    func = static_cast<uint32_t>(std::strtoul(funcStr.c_str(), &end, 10));
    if (end == funcStr.c_str() || *end != '\0')
        return false;

    return true;
}

inline bool StringToPciLocation(const std::string& str, PciLocation* outPci)
{
    if (outPci == nullptr)
    {
        return false;
    }
    uint32_t bus = 0;
    uint32_t dev = 0;
    uint32_t func = 0;

    if (!ParseBusDevFuncString(str, bus, dev, func))
    {
        return false;
    }
    outPci->bits.bus      = bus;
    outPci->bits.device   = dev;
    outPci->bits.function = func;

    return true;
}

// Converts a PciLocation to a BDF string ("bus:device.function")
inline std::string PciLocationToBdfString(const PciLocation* pci)
{
    if (pci == nullptr)
    {
        return "00:00.0";
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02x:%02x.%x", pci->bits.bus, pci->bits.device, pci->bits.function);
    return std::string(buf);
}

inline bool ParseBdfString(const std::string& bdf, uint32_t& bus, uint32_t& dev, uint32_t& func)
{
    const char* str = bdf.c_str();
    char* end = nullptr;

    // Parse bus (hex)
    bus = static_cast<uint32_t>(std::strtoul(str, &end, 16));
    if (end == str || *end != ':')
        return false;
    str = end + 1;

    // Parse device (hex)
    dev = static_cast<uint32_t>(std::strtoul(str, &end, 16));
    if (end == str || *end != '.')
        return false;
    str = end + 1;

    // Parse function (hex)
    func = static_cast<uint32_t>(std::strtoul(str, &end, 16));
    if (end == str || *end != '\0')
        return false;

    return true;

}

inline bool BdfStringToPciLocation(const std::string& bdf, PciLocation* outPci)
{
    if (outPci == nullptr)
    {
        return false;
    }

    uint32_t bus = 0;
    uint32_t dev = 0;
    uint32_t func = 0;

    if (!ParseBdfString(bdf, bus, dev, func))
    {
        return false;
    }

    outPci->bits.bus = bus;
    outPci->bits.device = dev;
    outPci->bits.function = func;

    return true;

}
#endif

#endif

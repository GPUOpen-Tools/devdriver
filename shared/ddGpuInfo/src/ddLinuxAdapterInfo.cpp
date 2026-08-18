/* Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <ddAdapterInfo.h>

#include <amdgpu.h>
#include <amdgpu_drm.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <xf86drm.h>

// WA: We need to build on CentOS 7, which uses an older libdrm that does not define this.
// This was introduced in libdrm 2.4.99.
#ifndef AMDGPU_VRAM_TYPE_GDDR6
    #define AMDGPU_VRAM_TYPE_GDDR6 9
#endif

// WA: We need to build on CentOS 7, which uses an older libdrm that does not define this.
// WA: We need to build on Ubuntu 16.04, which uses an older libdrm that does not define this.
#ifndef AMDGPU_VRAM_TYPE_DDR4
    #define AMDGPU_VRAM_TYPE_DDR4 8
#endif

// WA: We need to build on CentOS 7, which uses an older libdrm that does not define this.
// WA: We need to build on Ubuntu 16.04, which uses an older libdrm that does not define this.
#ifndef AMDGPU_VRAM_TYPE_DDR5
    #define AMDGPU_VRAM_TYPE_DDR5 10
#endif

// WA: We need to build on CentOS 7, which uses an older libdrm that does not define this.
// WA: We need to build on Ubuntu 16.04, which uses an older libdrm that does not define this.
#ifndef AMDGPU_VRAM_TYPE_LPDDR5
   #define AMDGPU_VRAM_TYPE_LPDDR5 12
#endif

namespace DevDriver
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Structure returned by amdgpu_query_gpu_info() to describe GPU h/w info
struct amdgpu_gpu_info
{
    /** Asic id */
    uint32_t asic_id;
    /** Chip revision */
    uint32_t chip_rev;
    /** Chip external revision */
    uint32_t chip_external_rev;
    /** Family ID */
    uint32_t family_id;
    /** Special flags */
    uint64_t ids_flags;
    /** max engine clock*/
    uint64_t max_engine_clk;
    /** max memory clock */
    uint64_t max_memory_clk;
    /** number of shader engines */
    uint32_t num_shader_engines;
    /** number of shader arrays per engine */
    uint32_t num_shader_arrays_per_engine;
    /**  Number of available good shader pipes */
    uint32_t avail_quad_shader_pipes;
    /**  Max. number of shader pipes.(including good and bad pipes  */
    uint32_t max_quad_shader_pipes;
    /** Number of parameter cache entries per shader quad pipe */
    uint32_t cache_entries_per_quad_pipe;
    /**  Number of available graphics context */
    uint32_t num_hw_gfx_contexts;
    /** Number of render backend pipes */
    uint32_t rb_pipes;
    /**  Enabled render backend pipe mask */
    uint32_t enabled_rb_pipes_mask;
    /** Frequency of GPU Counter */
    uint32_t gpu_counter_freq;
    /** CC_RB_BACKEND_DISABLE.BACKEND_DISABLE per SE */
    uint32_t backend_disable[4];
    /** Value of MC_ARB_RAMCFG register*/
    uint32_t mc_arb_ramcfg;
    /** Value of GB_ADDR_CONFIG */
    uint32_t gb_addr_cfg;
    /** Values of the GB_TILE_MODE0..31 registers */
    uint32_t gb_tile_mode[32];
    /** Values of GB_MACROTILE_MODE0..15 registers */
    uint32_t gb_macro_tile_mode[16];
    /** Value of PA_SC_RASTER_CONFIG register per SE */
    uint32_t pa_sc_raster_cfg[4];
    /** Value of PA_SC_RASTER_CONFIG_1 register per SE */
    uint32_t pa_sc_raster_cfg1[4];
    /* CU info */
    uint32_t cu_active_number;
    uint32_t cu_ao_mask;
    uint32_t cu_bitmap[4][4];
    /* video memory type info*/
    uint32_t vram_type;
    /* video memory bit width*/
    uint32_t vram_bit_width;
    /** constant engine ram size*/
    uint32_t ce_ram_size;
    /* vce harvesting instance */
    uint32_t vce_harvest_config;
    /* PCI revision ID */
    uint32_t pci_rev_id;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function pointer prototypes for the functions we will retrieve from the libdrm_amdgpu library
typedef int32 (*PFN_DrmGetDevices)(drmDevicePtr* pDevices, int32 maxDevices);

typedef int32 (*PFN_AmdgpuQueryGpuInfo)(amdgpu_device_handle hDevice, struct amdgpu_gpu_info* pInfo);

typedef int32 (*PFN_AmdgpuDeviceInitialize)(
    int                   fd,
    uint32*               pMajorVersion,
    uint32*               pMinorVersion,
    amdgpu_device_handle* pDeviceHandle);

typedef int32 (*PFN_AmdgpuDeviceDeinitialize)(amdgpu_device_handle hDevice);

typedef const char* (*PFN_AmdgpuGetMarketingName)(amdgpu_device_handle hDevice);

typedef int32 (*PFN_AmdgpuQueryInfo)(amdgpu_device_handle hDevice, uint32 infoId, uint32 size, void* pValue);

typedef int32 (
    *PFN_AmdgpuQueryHeapInfo)(amdgpu_device_handle hDevice, uint32 heap, uint32 flags, struct amdgpu_heap_info* pInfo);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static LocalMemoryType TranslateMemoryType(uint32 memType)
{
    switch (memType)
    {
        case AMDGPU_VRAM_TYPE_GDDR1:
        case AMDGPU_VRAM_TYPE_GDDR3:
        case AMDGPU_VRAM_TYPE_GDDR4:
            DD_ASSERT_REASON("Unexepcted memory type - GDDR1-4 are not supported by current drivers");
            break;

        case AMDGPU_VRAM_TYPE_DDR2: return LocalMemoryType::Ddr2; break;
        case AMDGPU_VRAM_TYPE_DDR3: return LocalMemoryType::Ddr3; break;
        case AMDGPU_VRAM_TYPE_DDR4: return LocalMemoryType::Ddr4; break;
	    case AMDGPU_VRAM_TYPE_DDR5: return LocalMemoryType::Ddr5; break;
	    case AMDGPU_VRAM_TYPE_GDDR5: return LocalMemoryType::Gddr5; break;
        case AMDGPU_VRAM_TYPE_GDDR6: return LocalMemoryType::Gddr6; break;
        case AMDGPU_VRAM_TYPE_HBM: return LocalMemoryType::Hbm; break;
        case AMDGPU_VRAM_TYPE_LPDDR5: return LocalMemoryType::Lpddr5; break;

        default: DD_ASSERT_REASON("Unrecognized memory type"); break;
    }

    return LocalMemoryType::Unknown;
}

void FillCuInformation(const amdgpu_gpu_info& info, ddAmdAdapterInfo::AsicInfo& asicInfo)
{
    DD_ASSERT(info.num_shader_engines <= 8);
    DD_ASSERT(info.num_shader_arrays_per_engine <= 2);

    memset(&asicInfo.cuMask, 0, sizeof(uint32) * (kMaxShaderEngines * kMaxShaderArraysPerEngine));
    asicInfo.numCus = 0;

    // Since the mask from libdrm is a 4x4 array, the KMD uses the left half to represent SE 0-3 and the right half to
    // represent SE 4-5 (for NV3).
    // See Device::InitGfx9CuMask in amdgpuDevice.cpp in PAL for more info.
    for (uint32 shaderEngine = 0; shaderEngine < info.num_shader_engines; ++shaderEngine)
    {
        for (uint32 shaderArray = 0; shaderArray < info.num_shader_arrays_per_engine; ++shaderArray)
        {
            const uint32 arrayMask = info.cu_bitmap[shaderEngine % 4][shaderArray + (2 * (shaderEngine / 4))];
            asicInfo.numCus += CountSetBits(arrayMask);
            asicInfo.cuMask[shaderEngine][shaderArray] = arrayMask;
        }
    }

    asicInfo.numShaderEngines = info.num_shader_engines;
    asicInfo.numShaderArraysPerEngine = info.num_shader_arrays_per_engine;
}

Result QueryAdapterInfo(Vector<ddAmdAdapterInfo>& gpus)
{
    Result result = Result::InvalidParameter;

    const char* kAmdGpuLibraryName = "libdrm_amdgpu.so.1";
    Platform::Library libdrmLoader;
    result = libdrmLoader.Load(kAmdGpuLibraryName);

    if (result == Result::Success)
    {
        constexpr uint32 MaxDevices           = 16;
        drmDevicePtr     pDevices[MaxDevices] = {};
        int32            deviceCount          = 0;

        // Get the devices
        PFN_DrmGetDevices pfnGetDevices;
        if (libdrmLoader.GetFunction("drmGetDevices", &pfnGetDevices) && (pfnGetDevices != nullptr))
        {
            deviceCount = pfnGetDevices(pDevices, MaxDevices);
        }

        result = (deviceCount > 0) ? Result::Success : Result::Unavailable;

        for (uint32 i = 0; (i < static_cast<uint32>(deviceCount)) && (result == Result::Success); ++i)
        {
            ddAmdAdapterInfo outAdapterInfo = {};

            // Copy over the PCI data
            outAdapterInfo.pci.bus      = pDevices[i]->businfo.pci->bus;
            outAdapterInfo.pci.device   = pDevices[i]->businfo.pci->dev;
            outAdapterInfo.pci.function = pDevices[i]->businfo.pci->func;

            drmPciDeviceInfoPtr pci_info        = pDevices[i]->deviceinfo.pci;
            outAdapterInfo.asic.ids.vendorId    = pci_info->vendor_id;
            outAdapterInfo.asic.ids.subsystemId = pci_info->subvendor_id + (pci_info->subdevice_id << 16);

            // Open the amdgpu device file descriptor
            int32 renderFd  = open(pDevices[i]->nodes[DRM_NODE_RENDER], O_RDONLY, 0);

            amdgpu_device_handle deviceHandle = NULL;
            uint32               majorVersion = 0;
            uint32               minorVersion = 0;

            // Initialize device
            if (renderFd < 0)
            {
                result = Result::Rejected;
            }
            else
            {
                // Initialize the amdgpu device.
                PFN_AmdgpuDeviceInitialize pfnDeviceInitialize;
                if (libdrmLoader.GetFunction("amdgpu_device_initialize", &pfnDeviceInitialize) &&
                    (pfnDeviceInitialize != nullptr))
                {
                    result = (pfnDeviceInitialize(renderFd, &majorVersion, &minorVersion, &deviceHandle) == 0) ?
                                    Result::Success :
                                    Result::Error;
                }
            }

            // Query GPU Info
            amdgpu_gpu_info gpuInfo = {};
            if (result == Result::Success)
            {
                PFN_AmdgpuQueryGpuInfo pfnQueryGpuInfo;
                if (libdrmLoader.GetFunction("amdgpu_query_gpu_info", &pfnQueryGpuInfo) &&
                    (pfnQueryGpuInfo != nullptr))
                {
                    result = (pfnQueryGpuInfo(deviceHandle, &gpuInfo) == 0) ? Result::Success : Result::Error;
                }
            }

            drm_amdgpu_memory_info memInfo = {};
            if (result == Result::Success)
            {
                // Now translate to our own ddAmdAdapterInfo struct
                outAdapterInfo.asic.ids.deviceId   = gpuInfo.asic_id;
                outAdapterInfo.asic.ids.eRevId     = gpuInfo.chip_external_rev;
                outAdapterInfo.asic.ids.revisionId = gpuInfo.pci_rev_id;
                outAdapterInfo.asic.ids.family     = gpuInfo.family_id;

                // Luids aren't applicable on Linux
                memset(outAdapterInfo.asic.ids.luid, 0, sizeof(outAdapterInfo.asic.ids.luid));

                // amdgpu reports this in KHz, we store it as Hz
                outAdapterInfo.engineClocks.max = gpuInfo.max_engine_clk * 1000;

                outAdapterInfo.asic.gpuIndex       = i;
                outAdapterInfo.asic.gpuCounterFreq = gpuInfo.gpu_counter_freq * 1000;

                FillCuInformation(gpuInfo, outAdapterInfo.asic);

                outAdapterInfo.memory.type           = TranslateMemoryType(gpuInfo.vram_type);
                outAdapterInfo.memory.memOpsPerClock = MemoryOpsPerClock(outAdapterInfo.memory.type);
                outAdapterInfo.memory.busBitWidth    = gpuInfo.vram_bit_width;
                // amdgpu reports this in KHz, we store it as Hz
                outAdapterInfo.memory.clocksHz.max = gpuInfo.max_memory_clk * 1000;

                outAdapterInfo.memory.hbccSize = 0; // Per PAL - Linux doesn't support HBCC

                // drm version info
                outAdapterInfo.drmVersion.Major = majorVersion;
                outAdapterInfo.drmVersion.Minor = minorVersion;

                // Get the marketing name string
                PFN_AmdgpuGetMarketingName pfnGetMarketingName;
                if (libdrmLoader.GetFunction("amdgpu_get_marketing_name", &pfnGetMarketingName) &&
                    (pfnGetMarketingName != nullptr))
                {
                    const char* pMarketingName = pfnGetMarketingName(deviceHandle);
                    if (pMarketingName != nullptr)
                    {
                        Platform::Strncpy(outAdapterInfo.name, pMarketingName);
                    }
                }

                // Query additional memory info
                PFN_AmdgpuQueryInfo pfnQueryInfo;
                if (libdrmLoader.GetFunction("amdgpu_query_info", &pfnQueryInfo) &&
                    (pfnQueryInfo != nullptr))
                {
                    if (pfnQueryInfo(deviceHandle, AMDGPU_INFO_MEMORY, sizeof(memInfo), &memInfo) != 0)
                    {
                        struct amdgpu_heap_info heap_info = {};
                        PFN_AmdgpuQueryHeapInfo pfnAmdgpuQueryHeapInfo;
                        if (libdrmLoader.GetFunction("amdgpu_query_heap_info", &pfnAmdgpuQueryHeapInfo) &&
                            (pfnAmdgpuQueryHeapInfo != nullptr))
                        {
                            if (pfnAmdgpuQueryHeapInfo(
                                    deviceHandle,
                                    AMDGPU_GEM_DOMAIN_VRAM,
                                    AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED,
                                    &heap_info) == 0)
                            {
                                outAdapterInfo.memory.localHeap.size = heap_info.heap_size;
                            }

                            if (pfnAmdgpuQueryHeapInfo(deviceHandle, AMDGPU_GEM_DOMAIN_VRAM, 0, &heap_info) == 0)
                            {
                                outAdapterInfo.memory.invisibleHeap.size = heap_info.heap_size;
                            }
                        }
                    }
                    else
                    {
                        outAdapterInfo.memory.localHeap.size     = memInfo.cpu_accessible_vram.total_heap_size;
                        outAdapterInfo.memory.invisibleHeap.size =
                            memInfo.vram.total_heap_size - outAdapterInfo.memory.localHeap.size;

                        // Currently libdrm doesn't provide base physical addresses. We just assume that
                        // the base address of the local visible memory region starts at 0, and the invisible
                        // memory region follows immediately after, and set their base addresses accordingly.
                        // See issue #361.
                        outAdapterInfo.memory.localHeap.physAddr     = 0;
                        outAdapterInfo.memory.invisibleHeap.physAddr = outAdapterInfo.memory.localHeap.size;
                    }
                }

                outAdapterInfo.version = DD_ADAPTERS_VERSION;
                outAdapterInfo.size    = sizeof(ddAmdAdapterInfo);
                outAdapterInfo.success = 1;
                outAdapterInfo.gpuId   = (outAdapterInfo.pci.bus << 16) | (outAdapterInfo.pci.device << 8) | outAdapterInfo.pci.function;

                // Not supported on Linux:
                outAdapterInfo.cpuHostAperEnabled              = false;
                outAdapterInfo.isResizeableBarControlSupported = false;

                gpus.PushBack(outAdapterInfo);
            }

            // Deinitialize device
            PFN_AmdgpuDeviceDeinitialize pfnDeviceDeinitialize;
            if (libdrmLoader.GetFunction("amdgpu_device_deinitialize", &pfnDeviceDeinitialize) &&
                (pfnDeviceDeinitialize != nullptr))
            {
                pfnDeviceDeinitialize(deviceHandle);
            }

            close(renderFd);
        }

        libdrmLoader.Close();
    }
    else
    {
        result = Result::FileNotFound;
    }

    return result;
}

} // namespace DevDriver

/* Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <RmtFtrace.h>
#include <ddPlatform.h>
#include <gtest/gtest.h>

using namespace rmt_ftrace;

static constexpr uint64_t kTestVirtualAddressStart  = 101;
static constexpr uint64_t kTestPhysicalAddressStart = 202;

TEST(PageTableUpdatePhyAddrCoalesceIterator, NoPage)
{
    PageTableUpdateEvent event = {};

    PageTableUpdatePhyAddrCoalesceIterator iter(event);

    PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair pair = {};
    ASSERT_FALSE(iter.Next(&pair));
}

TEST(PageTableUpdatePhyAddrCoalesceIterator, OnePage)
{
    const uint64_t kPageSize      = 8;
    const uint32_t kNumTotalPages = 1;

    uint64_t physical_addresses[kNumTotalPages] = {kTestPhysicalAddressStart};

    PageTableUpdateEvent event = {};
    event.start                = kTestVirtualAddressStart;
    event.dst                  = physical_addresses;
    event.nptes                = kNumTotalPages;
    event.incr                 = kPageSize;

    PageTableUpdatePhyAddrCoalesceIterator iter(event);

    PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair pair = {};

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart);
    ASSERT_EQ(pair.physical_address, kTestPhysicalAddressStart);
    ASSERT_EQ(pair.num_pages, (uint32_t)1);

    ASSERT_FALSE(iter.Next(&pair));
}

TEST(PageTableUpdatePhyAddrCoalesceIterator, AllContinuousPages)
{
    const uint64_t kPageSize      = 8;
    const uint32_t kNumTotalPages = 3;

    uint64_t physical_addresses[kNumTotalPages] = {kTestPhysicalAddressStart, kTestPhysicalAddressStart + kPageSize, kTestPhysicalAddressStart + 2 * kPageSize};

    PageTableUpdateEvent event = {};
    event.start                = kTestVirtualAddressStart;
    event.dst                  = physical_addresses;
    event.nptes                = kNumTotalPages;
    event.incr                 = kPageSize;

    PageTableUpdatePhyAddrCoalesceIterator iter(event);

    PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair pair = {};

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart);
    ASSERT_EQ(pair.physical_address, kTestPhysicalAddressStart);
    ASSERT_EQ(pair.num_pages, kNumTotalPages);

    ASSERT_FALSE(iter.Next(&pair));
}

TEST(PageTableUpdatePhyAddrCoalesceIterator, PartialContinuousPages)
{
    const uint64_t kPageSize      = 8;
    const uint32_t kNumTotalPages = 3;

    uint64_t physical_addresses[kNumTotalPages] = {kTestPhysicalAddressStart, kTestPhysicalAddressStart + kPageSize, kTestPhysicalAddressStart + 3 * kPageSize};

    PageTableUpdateEvent event = {};
    event.start                = kTestVirtualAddressStart;
    event.dst                  = physical_addresses;
    event.nptes                = kNumTotalPages;
    event.incr                 = kPageSize;

    PageTableUpdatePhyAddrCoalesceIterator iter(event);

    PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair pair = {};

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart);
    ASSERT_EQ(pair.physical_address, kTestPhysicalAddressStart);
    ASSERT_EQ(pair.num_pages, (uint32_t)2);

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart + 2 * kPageSize);
    ASSERT_EQ(pair.physical_address, physical_addresses[2]);
    ASSERT_EQ(pair.num_pages, (uint32_t)1);

    ASSERT_FALSE(iter.Next(&pair));
}

TEST(PageTableUpdatePhyAddrCoalesceIterator, AllDiscretePages)
{
    const uint64_t kPageSize      = 8;
    const uint32_t kNumTotalPages = 3;

    uint64_t physical_addresses[kNumTotalPages] = {
        kTestPhysicalAddressStart, kTestPhysicalAddressStart + 2 * kPageSize, kTestPhysicalAddressStart + 4 * kPageSize};

    PageTableUpdateEvent event = {};
    event.start                = kTestVirtualAddressStart;
    event.dst                  = physical_addresses;
    event.nptes                = kNumTotalPages;
    event.incr                 = kPageSize;

    PageTableUpdatePhyAddrCoalesceIterator iter(event);

    PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair pair = {};

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart);
    ASSERT_EQ(pair.physical_address, kTestPhysicalAddressStart);
    ASSERT_EQ(pair.num_pages, (uint32_t)1);

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart + 1 * kPageSize);
    ASSERT_EQ(pair.physical_address, physical_addresses[1]);
    ASSERT_EQ(pair.num_pages, (uint32_t)1);

    ASSERT_TRUE(iter.Next(&pair));
    ASSERT_EQ(pair.virtual_address, kTestVirtualAddressStart + 2 * kPageSize);
    ASSERT_EQ(pair.physical_address, physical_addresses[2]);
    ASSERT_EQ(pair.num_pages, (uint32_t)1);

    ASSERT_FALSE(iter.Next(&pair));
}

static void PrintPageTableUpdateEvent(const PageTableUpdateEvent& event, size_t event_num)
{
    std::cout << std::dec << "[event " << event_num << "]" << std::hex << " start: 0x" << event.start << " end: 0x" << event.end << " flags: 0x" << event.flags
              << std::dec << " nptes: " << event.nptes << " incr: " << event.incr << " pid: " << event.pid << " vm_ctx: " << std::hex << event.vm_ctx
              << " dis: ";

    PageTableUpdatePhyAddrCoalesceIterator                    iter(event);
    PageTableUpdatePhyAddrCoalesceIterator::VirPhyAddressPair pair = {};
    while (iter.Next(&pair))
    {
        std::cout << "(0x" << pair.physical_address << ";" << pair.num_pages << ")";
    }
    std::cout << "\n\n";
}

// There is no straight forward way to mock ftrace buffers to test if we're
// getting data and parsing them correctly. Here we use the real ftrace and
// print the data out and simply check if the values look reasonable. Run the
// following command:
// ```
// $ sudo ./_ninja/bin/Debug/tests_LinuxRMT --gtest_filter=FtraceEventCapture* --gtest_also_run_disabled_tests
// ```
TEST(FtraceEventCapture, DISABLED_EyeBalling)
{
    const int VULKAN_APP_WAIT_TIME = 5;

    FTraceContext ftrace;

    ASSERT_EQ(ftrace.Initialize(), DD_RESULT_SUCCESS);

    ftrace.Enable();

    printf("Run a Vulkan app now (e.g. vkcube). %d seconds wait.\n", VULKAN_APP_WAIT_TIME);
    // We don't know when the next page-table-update event will happen, so wait for a bit.
    sleep(VULKAN_APP_WAIT_TIME);

    EventRecord record(ftrace.PageTableUpdateEventFieldFormats(), DevDriver::Platform::GenericAllocCb);

    size_t event_num = 0;
    for (size_t i = 0; i < 200; ++i)
    {
        ftrace.PollEvents(&record);

        if (record.isNewEventPolled)
        {
            const DevDriver::Vector<PageTableUpdateEvent, 128>& ptu_events = record.ptu_events;
            for (size_t i = 0; i < ptu_events.Size(); ++i)
            {
                event_num++;
                PrintPageTableUpdateEvent(ptu_events[i], event_num);
            }
        }
    }

    ftrace.Disable();
    ftrace.Destroy();
}

/// GTest entry-point
GTEST_API_ int main(int argc, char** argv)
{
    // Run all tests
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

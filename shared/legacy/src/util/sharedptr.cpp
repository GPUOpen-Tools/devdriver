/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */

#include <util/sharedptr.h>

namespace DevDriver
{
    bool SharedPointerBase::IsNull() const
    {
        return m_pObject == nullptr;
    }

    SharedPointerBase::ContainerBase::ContainerBase(const AllocCb& allocCb)
        : m_allocCb(allocCb)
        , m_refCount(0)
    {
        //DD_PRINT(LogLevel::Never, "Created reference counted container %i", m_refCount);
    }

    int32 SharedPointerBase::ContainerBase::QueryReferenceCount(void) const
    {
        return RefCountValue();
    }

    SharedPointerBase::SharedPointerBase()
        : m_pContainer(nullptr)
        , m_pObject(nullptr)
    {
    }

    SharedPointerBase::SharedPointerBase(ContainerBase* pContainer, void* pObject)
        : m_pContainer(pContainer)
        , m_pObject(pObject)
    {
        // We should always have a valid object if the container is valid.
        DD_ASSERT((m_pContainer == nullptr) || (m_pObject != nullptr));

        // If we have a valid container, increment the reference count.
        if (m_pContainer != nullptr)
        {
            m_pContainer->Retain();
        }
    }

    SharedPointerBase::SharedPointerBase(const SharedPointerBase& right)
        : SharedPointerBase(right.m_pContainer, right.m_pObject)
    {
    }

    SharedPointerBase::SharedPointerBase(SharedPointerBase&& right)
        : m_pContainer(Platform::Exchange(right.m_pContainer, nullptr))
        , m_pObject(Platform::Exchange(right.m_pObject, nullptr))
    {
    }
} // DevDriver

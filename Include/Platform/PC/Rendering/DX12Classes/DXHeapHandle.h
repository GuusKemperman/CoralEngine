#pragma once
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"

class DXHeapHandle
{
    friend DXDescHeap;
public:
    //Creates a heap handle that is not bound to anything
    DXHeapHandle() = default;

    DXHeapHandle(DXHeapHandle&& other) noexcept {
        FreeResource();
        mIndex = other.mIndex; other.mIndex = 0;       
        mDescHeap = other.mDescHeap;
        other.mDescHeap.reset();
    }

    DXHeapHandle& operator=(DXHeapHandle&& other) noexcept {
        if (this == &other) return *this;
        FreeResource();

        mIndex = other.mIndex; other.mIndex = 0;       
        mDescHeap = other.mDescHeap;
        other.mDescHeap.reset();
        return *this;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetAddressCPU() const {
        if (auto lock = mDescHeap.lock()) 
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(lock->Get()->GetCPUDescriptorHandleForHeapStart(), mIndex, lock->GetDescriptorSize());
        return {};
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE GetAddressGPU() const {
        if (auto lock = mDescHeap.lock()) 
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(lock->Get()->GetGPUDescriptorHandleForHeapStart(), mIndex, lock->GetDescriptorSize());
        return {};
    }

    ~DXHeapHandle() {
        FreeResource();
    }

	// Delete the copy constructor
    DXHeapHandle(const DXHeapHandle&) = delete;
    DXHeapHandle& operator=(const DXHeapHandle&) = delete;

private:
    DXHeapHandle(uint32_t index, std::weak_ptr<DXDescHeap> descHeap)
        : mIndex(index), mDescHeap(descHeap) {};

    void FreeResource()
	{
        if (mIndex == -1)
            return;


        if (auto lock = mDescHeap.lock()) {
            lock->DeallocateResource(mIndex);
        }
    };

    int mIndex = -1;
    std::weak_ptr<DXDescHeap> mDescHeap;
};


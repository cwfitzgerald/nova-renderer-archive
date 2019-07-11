#pragma once

#include "d3dx12.h"
#include "nova_renderer/rhi_types.hpp"
#include "nova_renderer/shaderpack_data.hpp"

namespace nova::renderer::rhi {
    struct DX12DeviceMemory : DeviceMemory {
        ID3D12Heap* heap;
    };

    struct DX12Renderpass : Renderpass {};

    struct DX12Buffer : Buffer {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        bvestl::polyalloc::Bytes size{0};
    };

    struct DX12Image : Image {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    };

    struct DX12Framebuffer : Framebuffer {
        eastl::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> render_targets;

        bool has_depth_stencil = false;
        D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_image = {};

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap = nullptr;
    };

    struct DX12PipelineInterface : PipelineInterface {
        eastl::unordered_map<uint32_t, eastl::vector<ResourceBindingDescription>> table_layouts;

        eastl::vector<shaderpack::TextureAttachmentInfo> color_attachments;

        eastl::optional<shaderpack::TextureAttachmentInfo> depth_texture;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root_sig;
    };

    struct DX12DescriptorPool : DescriptorPool {
        // DX12 has a separate descriptor heap for each descriptor set in a material, meaning this struct can be
        // completely empty
    };

    struct DX12DescriptorSet : DescriptorSet {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;

        eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> descriptors;
    };

    struct DX12Pipeline : Pipeline {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = nullptr;
    };

    struct DX12Fence : Fence {
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        HANDLE event;
    };

    struct DX12Semaphore : Semaphore {
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    };
} // namespace nova::renderer::rhi

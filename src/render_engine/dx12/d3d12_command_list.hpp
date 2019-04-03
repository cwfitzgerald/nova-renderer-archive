/*!
 * \author ddubois
 * \date 30-Mar-19.
 */

#ifndef NOVA_RENDERER_D3D12_COMMAND_LIST_HPP
#define NOVA_RENDERER_D3D12_COMMAND_LIST_HPP
#include <d3d12.h>
#include <wrl/client.h>
#include <nova_renderer/command_list.hpp>

#include "d3d12_structs.hpp"

namespace nova::renderer {
    using namespace Microsoft::WRL;

    class d3d12_command_list : public command_list_t {
    public:
        explicit d3d12_command_list(ComPtr<ID3D12GraphicsCommandList> cmds);

        void resource_barriers([[maybe_unused]] pipeline_stage_flags stages_before_barrier,
                              [[maybe_unused]] pipeline_stage_flags stages_after_barrier,
                              const std::vector<resource_barrier_t>& barriers) override;

        void copy_buffer(resource_t* destination_buffer,
                         uint64_t destination_offset,
                         resource_t* source_buffer,
                         uint64_t source_offset,
                         uint64_t num_bytes) override;

        void execute_command_lists(const std::vector<command_list_t*>& lists) override;

        void begin_renderpass([[maybe_unused]] renderpass_t* renderpass, framebuffer_t* framebuffer) override;
        void end_renderpass() override;
        void bind_pipeline() override;
        void bind_material() override;

        void bind_vertex_buffers() override;
        void bind_index_buffer() override;
        void draw_indexed_mesh() override;

    private:
        ComPtr<ID3D12GraphicsCommandList> cmds;
    };
} // namespace nova::renderer

#endif // NOVA_RENDERER_D3D12_COMMAND_LIST_HPP

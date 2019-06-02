/*!
 * \author ddubois
 * \date 31-Mar-19.
 */

#include "gl3_command_list.hpp"

#include "gl3_structs.hpp"

namespace nova::renderer::rhi {
    Gl3Command::~Gl3Command() {
        switch(type) {
            case Gl3CommandType::BufferCopy:
                buffer_copy.~Gl3BufferCopyCommand();
                break;
            case Gl3CommandType::ExecuteCommandLists:
                execute_command_lists.~Gl3ExecuteCommandListsCommand();
                break;
            case Gl3CommandType::BeginRenderpass:
                begin_renderpass.~Gl3BeginRenderpassCommand();
                break;

            case Gl3CommandType::EndRenderpass:
                // TODO
                break;

            case Gl3CommandType::BindPipeline:
                bind_pipeline.~Gl3BindPipelineCommand();
                break;

            case Gl3CommandType::BindDescriptorSets:
                bind_descriptor_sets.~Gl3BindDescriptorSetsCommand();
                break;

            case Gl3CommandType::BindVertexBuffers:
                bind_vertex_buffers.~Gl3BindVertexBuffersCommand();
                break;

            case Gl3CommandType::BindIndexBuffer:
                bind_index_buffer.~Gl3BindIndexBufferCommand();
                break;

            case Gl3CommandType::DrawIndexedMesh:
                draw_indexed_mesh.~Gl3DrawIndexedMeshCommand();
                break;
        }
    }

    rhi::Gl3CommandList::Gl3CommandList() {
        // TODO: maintain an average of the number of commands per command list, and allocate enough commands for like 90% of all command
        // lists
        commands.reserve(128);
    }

    void Gl3CommandList::resource_barriers(PipelineStageFlags /* stages_before_barrier */,
                                           PipelineStageFlags /* stages_after_barrier */,
                                           const std::vector<ResourceBarrier>& /* barriers */) {
        // Don't need to do anything whoop
    }

    void Gl3CommandList::copy_buffer(Buffer* destination_buffer,
                                     const uint64_t destination_offset,
                                     Buffer* source_buffer,
                                     const uint64_t source_offset,
                                     const uint64_t num_bytes) {
        Gl3Buffer* dst_buf = reinterpret_cast<Gl3Buffer*>(destination_buffer);
        Gl3Buffer* src_buf = reinterpret_cast<Gl3Buffer*>(source_buffer);

        commands.emplace_back();

        Gl3Command& copy_command = commands.front();
        copy_command.type = Gl3CommandType::BufferCopy;
        copy_command.buffer_copy.destination_buffer = dst_buf->id;
        copy_command.buffer_copy.destination_offset = destination_offset;
        copy_command.buffer_copy.source_buffer = src_buf->id;
        copy_command.buffer_copy.source_offset = source_offset;
        copy_command.buffer_copy.num_bytes = num_bytes;
    }

    void Gl3CommandList::execute_command_lists(const std::vector<CommandList*>& lists) {
        commands.emplace_back();

        Gl3Command& execute_lists_command = commands.front();
        execute_lists_command.execute_command_lists.lists_to_execute = lists;
    }

    void Gl3CommandList::begin_renderpass(Renderpass* /* renderpass */, Framebuffer* framebuffer) {
        Gl3Framebuffer* gl_framebuffer = reinterpret_cast<Gl3Framebuffer*>(framebuffer);

        commands.emplace_back();

        Gl3Command& renderpass_command = commands.front();
        renderpass_command.type = Gl3CommandType::BeginRenderpass;
        renderpass_command.begin_renderpass.framebuffer = gl_framebuffer->id;
    }

    void Gl3CommandList::end_renderpass() {}

    void Gl3CommandList::bind_pipeline(const Pipeline* pipeline) {
        const Gl3Pipeline* gl_pipeline = static_cast<const Gl3Pipeline*>(pipeline);

        commands.emplace_back();

        Gl3Command& command = commands.front();
        command.type = Gl3CommandType::BindPipeline;
        command.bind_pipeline.program = gl_pipeline->id;
    }

    void Gl3CommandList::bind_descriptor_sets(const std::vector<DescriptorSet*>& descriptor_sets,
                                              const PipelineInterface* pipeline_interface) {
        // Alrighty here's where the fun happens
        // For each descriptor, get its uniform binding from the pipeline interface
        // Record a command to bind it

        const Gl3PipelineInterface* gl_interface = static_cast<const Gl3PipelineInterface*>(pipeline_interface);

        commands.emplace_back();

        Gl3Command& command = commands.front();
        command.type = Gl3CommandType::BindDescriptorSets;
        command.bind_descriptor_sets.pipeline_bindings = gl_interface->bindings;
        command.bind_descriptor_sets.uniform_cache = gl_interface->uniform_cache;
        command.bind_descriptor_sets.sets.reserve(descriptor_sets.size());

        for(const DescriptorSet* set : descriptor_sets) {
            const Gl3DescriptorSet* gl_set = static_cast<const Gl3DescriptorSet*>(set);
            command.bind_descriptor_sets.sets.emplace_back(gl_set);
        }
    }

    void Gl3CommandList::bind_vertex_buffers(const std::vector<Buffer*>& buffers) {
        commands.emplace_back();

        Gl3Command& command = commands.front();

        command.type = Gl3CommandType::BindVertexBuffers;
        command.bind_vertex_buffers.buffers.reserve(buffers.size());

        for(const Buffer* buffer : buffers) {
            const Gl3Buffer* gl_buffer = static_cast<const Gl3Buffer*>(buffer);

            command.bind_vertex_buffers.buffers.push_back(gl_buffer->id);
        }
    }

    void Gl3CommandList::bind_index_buffer(const Buffer* buffer) {
        const Gl3Buffer* gl_buffer = static_cast<const Gl3Buffer*>(buffer);

        commands.emplace_back();
        Gl3Command& command = commands.front();

        command.type = Gl3CommandType::BindIndexBuffer;
        command.bind_index_buffer.buffer = gl_buffer->id;
    }

    std::vector<Gl3Command> Gl3CommandList::get_commands() const { return commands; }
} // namespace nova::renderer::rhi

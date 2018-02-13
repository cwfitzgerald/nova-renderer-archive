#ifndef RENDERER_AUTO_ALLOCATED_BUFFER_H
#define RENDERER_AUTO_ALLOCATED_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "../../vulkan/render_context.h"

namespace nova {
    /*!
     * \brief A chunk of a buffer
     *
     * This class can be added to a lit to mark the free areas in a buffer
     */
    struct auto_buffer_chunk {
        vk::DeviceSize offset;
        vk::DeviceSize range;
    };

    /*!
     * \brief A buffer that can be allocated from
     *
     * This buffer will attempt to automatically allocate space for you. It does not handle fragmentation at all since
     * the intended use case is a buffer with lots of things that are the same size
     *
     * A buffer allocated through this class is set up to move data from the CPU to the GPU. If that isn't the case in
     * the future then future DethRaid will have some work to do
     */
    class auto_buffer {
    public:
        auto_buffer() = default;

        /*!
         * \brief Creates a new auto-allocating buffer of the given size on the provided device
         * \param total_size The size of the buffer
         * \param device The device to create the buffer on
         */
        auto_buffer(std::shared_ptr<render_context> context, vk::BufferCreateInfo create_info);

        /*!
         * \brief Destroys the buffer on the device
         *
         * This will invalidate all the allocations from this buffer but I don't do anything to tell abont about that.
         * Hope you're careful!
         */
        ~auto_buffer();

        /*!
         * \brief Allocates a chunk of the underlying buffer for your personal user
         *
         * This method will give you thr first free chunk it has, nothing else. If that leads to lots of fragmentation
         * then I'll deal with that later
         *
         * If there is not space, throws an out of memory error
         *
         * \param size The size, in bytes, of the space that you need
         * \return A representation of the allocation that's ready for use in a descriptor set
         */
        vk::DescriptorBufferInfo allocate_space(uint32_t size);

        /*!
         * \brief Frees the provided allocation
         *
         * \param to_free The allocation to free
         */
        void free_allocation(const vk::DescriptorBufferInfo& to_free);
    private:
        std::shared_ptr<render_context> context;

        vk::Device device;
        vk::Buffer buffer;
        VmaAllocation allocation;

        std::vector<auto_buffer_chunk> chunks;
    };

    vk::DeviceSize space_between(const auto_buffer_chunk& first, const auto_buffer_chunk& last);
}


#endif //RENDERER_AUTO_ALLOCATED_BUFFER_H

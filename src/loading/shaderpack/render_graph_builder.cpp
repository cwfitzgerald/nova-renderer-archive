/*!
 * \author ddubois
 * \date 17-Sep-18.
 */

#include "render_graph_builder.hpp"
#include <EASTL/unordered_set.h>
#include <minitrace/minitrace.h>
#include "../../util/logger.hpp"

namespace nova::renderer::shaderpack {
    /*!
     * \brief Adds all the passes that `pass_name` depends on to the list of ordered passes
     *
     * This method performs a depth-first traversal of the pass tree. It shouldn't matter whether we do depth or
     * breadth first, but depth first feels cleaner
     *
     * \param pass_name The passes that were just added to the list of ordered passes
     * \param passes A map from pass name to pass. Useful for the explicit dependencies of a pass
     * \param ordered_passes The passes in submissions order... almost. When this function adds to ordered_passes the
     * list has a lot of duplicates. They're removed in a later step
     * \param resource_to_write_pass A map from resource name to list of passes that write to that resource. Useful for
     * resolving the implicit dependencies of a pass
     * \param depth The depth in the tree that we're at. If this number ever grows bigger than the total number of
     * passes, there's a circular dependency somewhere in the render graph. This is Bad and we hate it
     */
    void add_dependent_passes(const eastl::string& pass_name,
                              const eastl::unordered_map<eastl::string, RenderPassCreateInfo>& passes,
                              eastl::vector<eastl::string>& ordered_passes,
                              const eastl::unordered_map<eastl::string, eastl::vector<eastl::string>>& resource_to_write_pass,
                              uint32_t depth);

    bool Range::has_writer() const { return first_write_pass <= last_write_pass; }

    bool Range::has_reader() const { return first_read_pass <= last_read_pass; }

    bool Range::is_used() const { return has_writer() || has_reader(); }

    bool Range::can_alias() const {
        // If we read before we have completely written to a resource we need to preserve it, so no alias is possible.
        return !(has_reader() && has_writer() && first_read_pass <= first_write_pass);
    }

    unsigned Range::last_used_pass() const {
        unsigned last_pass = 0;
        if(has_writer()) {
            last_pass = eastl::max(last_pass, last_write_pass);
        }
        if(has_reader()) {
            last_pass = eastl::max(last_pass, last_read_pass);
        }
        return last_pass;
    }

    unsigned Range::first_used_pass() const {
        unsigned first_pass = ~0U;
        if(has_writer()) {
            first_pass = eastl::min(first_pass, first_write_pass);
        }
        if(has_reader()) {
            first_pass = eastl::min(first_pass, first_read_pass);
        }
        return first_pass;
    }

    bool Range::is_disjoint_with(const Range& other) const {
        if(!is_used() || !other.is_used()) {
            return false;
        }
        if(!can_alias() || !other.can_alias()) {
            return false;
        }

        const bool left = last_used_pass() < other.first_used_pass();
        const bool right = other.last_used_pass() < first_used_pass();
        return left || right;
    }

    Result<eastl::vector<RenderPassCreateInfo>> order_passes(const eastl::vector<RenderPassCreateInfo>& passes) {
        MTR_SCOPE("Renderpass", "order_passes");

        NOVA_LOG(DEBUG) << "Executing Pass Scheduler";

        eastl::unordered_map<eastl::string, RenderPassCreateInfo> render_passes_to_order;
        render_passes_to_order.reserve(passes.size());
        for(const RenderPassCreateInfo& create_info : passes) {
            render_passes_to_order.emplace(create_info.name, create_info);
        }

        eastl::vector<eastl::string> ordered_passes;
        ordered_passes.reserve(passes.size());

        /*
         * Build some acceleration structures
         */

        NOVA_LOG(TRACE) << "Collecting passes that write to each resource...";
        // Maps from resource name to pass that writes to that resource, then from resource name to pass that reads from
        // that resource
        auto resource_to_write_pass = eastl::unordered_map<eastl::string, eastl::vector<eastl::string>>{};

        for(const auto& pass : passes) {
            for(const auto& output : pass.texture_outputs) {
                resource_to_write_pass[output.name].push_back(pass.name);
            }

            for(const auto& buffer_output : pass.output_buffers) {
                resource_to_write_pass[buffer_output].push_back(pass.name);
            }
        }

        /*
         * Initial ordering of passes
         */

        NOVA_LOG(TRACE) << "First pass at ordering passes...";
        // The passes, in simple dependency order
        if(resource_to_write_pass.find("Backbuffer") == resource_to_write_pass.end()) {
            NOVA_LOG(ERROR)
                << "This render graph does not write to the backbuffer. Unable to load this shaderpack because it can't render anything";
            return Result<eastl::vector<RenderPassCreateInfo>>(NovaError("Failed to order passes because no backbuffer was found"));
        }

        auto backbuffer_writes = resource_to_write_pass["Backbuffer"];
        ordered_passes.insert(ordered_passes.end(), backbuffer_writes.begin(), backbuffer_writes.end());

        for(const auto& pass_name : backbuffer_writes) {
            add_dependent_passes(pass_name, render_passes_to_order, ordered_passes, resource_to_write_pass, 1);
        }

        reverse(ordered_passes.begin(), ordered_passes.end());

        // We're going to loop through the original list of passes and remove them from the original list of passes
        // We want to keep the original passes around

        // This code taken from `RenderGraph::filter_passes` in the Granite engine
        // It loops through the ordered passes. When it sees the name of a new pass, it writes the pass to
        // ordered_passes and increments the write position. After all the passes are written, we remove all the
        // passes after the last one we wrote to, shrinking the list of ordered passes to only include the exact passes we want
        eastl::unordered_set<eastl::string> seen;

        auto output_itr = ordered_passes.begin();
        for(const auto& pass : ordered_passes) {
            if(seen.count(pass) == 0U) {
                *output_itr = pass;
                seen.insert(pass);
                ++output_itr;
            }
        }
        ordered_passes.erase(output_itr, ordered_passes.end());

        // Granite does some reordering to try and find a submission order that has the fewest pipeline barriers. Not
        // gonna worry about that now

        eastl::vector<RenderPassCreateInfo> passes_in_submission_order;
        passes_in_submission_order.reserve(ordered_passes.size());

        for(const eastl::string& pass_name : ordered_passes) {
            passes_in_submission_order.push_back(render_passes_to_order.at(pass_name));
        }

        return Result(passes_in_submission_order);
    }

    void add_dependent_passes(const eastl::string& pass_name,
                              const eastl::unordered_map<eastl::string, RenderPassCreateInfo>& passes,
                              eastl::vector<eastl::string>& ordered_passes,
                              const eastl::unordered_map<eastl::string, eastl::vector<eastl::string>>& resource_to_write_pass,
                              const uint32_t depth) {
        if(depth > passes.size()) {
            NOVA_LOG(ERROR) << "Circular render graph detected! Please fix your render graph to not have circular dependencies";
        }

        const auto& pass = passes.at(pass_name);

        // Add all the passes that this pass is dependent on
        for(const auto& dependency : pass.dependencies) {
            ordered_passes.push_back(dependency);
            add_dependent_passes(dependency, passes, ordered_passes, resource_to_write_pass, depth + 1);
        }

        for(const auto& texture_name : pass.texture_inputs) {
            if(resource_to_write_pass.find(texture_name) == resource_to_write_pass.end()) {
                // TODO: Ignore the implicitly defined resources
                NOVA_LOG(ERROR) << "Pass " << pass_name.c_str() << " reads from resource " << texture_name.c_str()
                                << ", but nothing writes to it";
            } else {
                const auto& write_passes = resource_to_write_pass.at(texture_name);
                ordered_passes.insert(ordered_passes.end(), write_passes.begin(), write_passes.end());

                for(const auto& write_pass : write_passes) {
                    add_dependent_passes(write_pass, passes, ordered_passes, resource_to_write_pass, depth + 1);
                }
            }
        }

        for(const auto& buffer_name : pass.input_buffers) {
            if(resource_to_write_pass.find(buffer_name) == resource_to_write_pass.end()) {
                NOVA_LOG(ERROR) << "Pass " << pass_name.c_str() << " reads from buffer " << buffer_name.c_str()
                                << ", but no passes write to it";
            } else {
                const auto& write_passes = resource_to_write_pass.at(buffer_name);
                ordered_passes.insert(ordered_passes.end(), write_passes.begin(), write_passes.end());

                for(const auto& write_pass : write_passes) {
                    add_dependent_passes(write_pass, passes, ordered_passes, resource_to_write_pass, depth + 1);
                }
            }
        }
    }

    void determine_usage_order_of_textures(const eastl::vector<RenderPassCreateInfo>& passes,
                                           eastl::unordered_map<eastl::string, Range>& resource_used_range,
                                           eastl::vector<eastl::string>& resources_in_order) {
        uint32_t pass_idx = 0;
        for(const auto& pass : passes) {
            // color attachments
            for(const auto& input : pass.texture_inputs) {
                auto& tex_range = resource_used_range[input];

                if(pass_idx < tex_range.first_write_pass) {
                    tex_range.first_write_pass = pass_idx;
                } else if(pass_idx > tex_range.last_write_pass) {
                    tex_range.last_write_pass = pass_idx;
                }

                if(eastl::find(resources_in_order.begin(), resources_in_order.end(), input) == resources_in_order.end()) {
                    resources_in_order.push_back(input);
                }
            }

            if(!pass.texture_outputs.empty()) {
                for(const auto& output : pass.texture_outputs) {
                    auto& tex_range = resource_used_range[output.name];

                    if(pass_idx < tex_range.first_write_pass) {
                        tex_range.first_write_pass = pass_idx;
                    } else if(pass_idx > tex_range.last_write_pass) {
                        tex_range.last_write_pass = pass_idx;
                    }

                    if(eastl::find(resources_in_order.begin(), resources_in_order.end(), output.name) == resources_in_order.end()) {
                        resources_in_order.push_back(output.name);
                    }
                }
            }

            pass_idx++;
        }
    }

    eastl::unordered_map<eastl::string, eastl::string> determine_aliasing_of_textures(
        const eastl::unordered_map<eastl::string, TextureCreateInfo>& textures,
        const eastl::unordered_map<eastl::string, Range>& resource_used_range,
        const eastl::vector<eastl::string>& resources_in_order) {
        eastl::unordered_map<eastl::string, eastl::string> aliases;
        aliases.reserve(resources_in_order.size());

        for(size_t i = 0; i < resources_in_order.size(); i++) {
            const auto& to_alias_name = resources_in_order[i];
            NOVA_LOG(TRACE) << "Determining if we can alias `" << to_alias_name.c_str() << "`. Does it exist? "
                            << (textures.find(to_alias_name) != textures.end());
            if(to_alias_name == "Backbuffer" || to_alias_name == "backbuffer") {
                // Yay special cases!
                continue;
            }

            const auto& to_alias_format = textures.at(to_alias_name).format;

            // Only try to alias with lower-indexed resources
            for(size_t j = 0; j < i; j++) {
                NOVA_LOG(TRACE) << "Trying to alias it with resource at index " << j << " out of " << resources_in_order.size();
                const eastl::string& try_alias_name = resources_in_order[j];
                if(resource_used_range.at(to_alias_name).is_disjoint_with(resource_used_range.at(try_alias_name))) {
                    // They can be aliased if they have the same format
                    const auto& try_alias_format = textures.at(try_alias_name).format;
                    if(to_alias_format == try_alias_format) {
                        aliases[to_alias_name] = try_alias_name;
                    }
                }
            }
        }

        return aliases;
    }

} // namespace nova::renderer::shaderpack

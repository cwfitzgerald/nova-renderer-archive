/*!
 * \brief Contains a bunch of utility functions which may or may not be actually used anywhere
 *
 * \author David
 * \date 18-May-16.
 */

#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <fstream>

#include "filesystem.hpp"

#if defined(NOVA_EXPORT)
#if defined(__GNUC__)
#pragma message("NOVA_EXPORT defined [GNUC/Clang]")
#define NOVA_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#pragma message("NOVA_EXPORT defined [MSVC]")
#define NOVA_API __declspec(dllexport)
#else
#pragma message("NOVA_EXPORT defined [Unknown compiler]")
#define NOVA_API
#endif
#else
#if defined(__GNUC__)
#pragma message("No definition for NOVA_EXPORT found [GNU/Clang]")
#define NOVA_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#pragma message("No definition for NOVA_EXPORT found [MSVC]")
#define NOVA_API __declspec(dllimport)
#else
#pragma message("No definition for NOVA_EXPORT found [Unknown compiler]")
#define NOVA_API
#endif
#endif

namespace nova::renderer {
    template <int Num>
    struct placeholder;

    /*!
     * \brief Calls the function once for every element in the provided container
     *
     * \param container The container to perform an action for each element in
     * \param thing_to_do The action to perform for each element in the collection
     */
    template <typename Cont, typename Func>
    void foreach(Cont container, Func thing_to_do) {
        eastl::for_each(eastl::cbegin(container), eastl::cend(container), thing_to_do);
    }

    eastl::vector<eastl::string> split(const eastl::string& s, char delim);

    eastl::string join(const eastl::vector<eastl::string>& strings, const eastl::string& joiner);

    eastl::string print_color(unsigned int color);

    eastl::string print_array(int* data, int size);

    bool ends_with(const eastl::string& string, const eastl::string& ending);

    void write_to_file(const eastl::string& data, const fs::path& filepath);

    void write_to_file(const eastl::vector<uint32_t>& data, const fs::path& filepath);
    
#define FORMAT(s, ...) fmt::format(fmt(s), __VA_ARGS__)
} // namespace nova::renderer

#endif // RENDERER_UTILS_H
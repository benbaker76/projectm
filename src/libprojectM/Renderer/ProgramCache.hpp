#pragma once

#include "Renderer/OpenGL.h"

#include <cstddef>
#include <string>

namespace libprojectM {
namespace Renderer {

/**
 * @brief Linked shader programs, shared by every Shader that compiles the same sources.
 *
 * Compiling and linking a preset's shaders is most of what switching to it costs, and the same
 * sources come round again: a preset shown a second time, shaders many presets have in common,
 * and -- what this is for -- a preset loaded ahead of time by another projectM instance on
 * another thread, whose OpenGL context shares objects with the one that will draw it.
 *
 * A program stays while any Shader uses it. Once none does, it is kept among the most recently
 * used idle programs in case it is wanted again, and the oldest idle program beyond that is
 * deleted. Programs are keyed by their exact sources.
 *
 * Thread-safe. A program is deleted by the thread that releases it last, or that pushes it out of
 * the idle programs, which must have a context of the share group current -- as any thread
 * destroying or compiling a Shader does.
 */
class ProgramCache
{
public:
    /**
     * @brief A linked program already made from these sources, now used by one more Shader, or 0.
     */
    static auto Acquire(const std::string& vertexSource, const std::string& fragmentSource) -> GLuint;

    /**
     * @brief Offers a program just linked from these sources, used by the Shader offering it.
     * @return true if the cache holds it now; false if it already had these sources, when the
     *         caller keeps its program to itself and deletes it as before.
     */
    static auto Register(const std::string& vertexSource, const std::string& fragmentSource, GLuint program) -> bool;

    /**
     * @brief One Shader fewer uses the program.
     * @return false if the cache does not hold the program.
     */
    static auto Release(GLuint program) -> bool;

    /**
     * @brief How many programs no Shader uses are kept. The default is 16.
     */
    static void SetIdleCapacity(size_t count);

    /**
     * @brief Counts a program compiled and linked by a Shader, because none was cached.
     */
    static void CountCompiled();

    /**
     * @brief How many programs were compiled, and how many times one was reused instead.
     */
    static void Counts(unsigned int& compiled, unsigned int& reused);
};

} // namespace Renderer
} // namespace libprojectM

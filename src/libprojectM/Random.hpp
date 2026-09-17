#pragma once

#include <cstdint>

namespace libprojectM {

/**
 * @brief Where projectM's random choices get their seeds.
 *
 * Normally every generator is seeded from the system, so no two runs look
 * alike. With a fixed seed set, the seeds handed out follow from it in
 * order instead, so the same calls to projectM draw the same frames --
 * which is what a benchmark or a check that a change leaves the pictures
 * alone needs.
 */
class Random
{
public:
    /**
     * @brief Fixes the seeds handed out from now on, or with 0 goes back to the system.
     */
    static void SetSeed(uint32_t seed);

    /**
     * @brief A seed for a new random generator.
     */
    static auto NewSeed() -> uint32_t;
};

} // namespace libprojectM

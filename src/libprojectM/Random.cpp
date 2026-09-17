#include "Random.hpp"

#include <mutex>
#include <random>

namespace libprojectM {

namespace {
std::mutex& Lock()
{
    static std::mutex lock;
    return lock;
}

bool s_fixed{false};
std::mt19937 s_sequence;
} // namespace

void Random::SetSeed(uint32_t seed)
{
    std::lock_guard<std::mutex> guard(Lock());
    s_fixed = seed != 0;
    s_sequence.seed(seed);
}

auto Random::NewSeed() -> uint32_t
{
    std::lock_guard<std::mutex> guard(Lock());
    if (s_fixed)
    {
        return s_sequence();
    }
    static std::random_device device;
    return device();
}

} // namespace libprojectM

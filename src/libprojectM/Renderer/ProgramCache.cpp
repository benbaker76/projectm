#include "Renderer/ProgramCache.hpp"

#include <cstdint>
#include <list>
#include <mutex>
#include <unordered_map>

namespace libprojectM {
namespace Renderer {

namespace {

struct Entry {
    std::string key;
    GLuint program{};
    int users{};
    uint64_t lastUsed{};
};

struct Cache {
    std::mutex lock;
    std::list<Entry> entries;
    std::unordered_map<std::string, std::list<Entry>::iterator> byKey;
    std::unordered_map<GLuint, std::list<Entry>::iterator> byProgram;
    size_t idleCapacity{16};
    uint64_t clock{};
    unsigned int compiled{};
    unsigned int reused{};
};

auto TheCache() -> Cache&
{
    // Never destroyed: programs cannot be deleted without a context, which is gone by then.
    static auto* cache = new Cache();
    return *cache;
}

auto KeyOf(const std::string& vertexSource, const std::string& fragmentSource) -> std::string
{
    std::string key;
    key.reserve(vertexSource.size() + fragmentSource.size() + 1);
    key += vertexSource;
    key += '\0';
    key += fragmentSource;
    return key;
}

// Deletes the idle programs used longest ago, beyond the capacity. Under the lock.
void TrimIdle(Cache& cache)
{
    for (;;)
    {
        size_t idle = 0;
        auto oldest = cache.entries.end();
        for (auto it = cache.entries.begin(); it != cache.entries.end(); ++it)
        {
            if (it->users > 0)
            {
                continue;
            }
            idle++;
            if (oldest == cache.entries.end() || it->lastUsed < oldest->lastUsed)
            {
                oldest = it;
            }
        }
        if (idle <= cache.idleCapacity || oldest == cache.entries.end())
        {
            return;
        }
        glDeleteProgram(oldest->program);
        cache.byKey.erase(oldest->key);
        cache.byProgram.erase(oldest->program);
        cache.entries.erase(oldest);
    }
}

} // namespace

auto ProgramCache::Acquire(const std::string& vertexSource, const std::string& fragmentSource) -> GLuint
{
    auto& cache = TheCache();
    std::lock_guard<std::mutex> guard(cache.lock);
    auto found = cache.byKey.find(KeyOf(vertexSource, fragmentSource));
    if (found == cache.byKey.end())
    {
        return 0;
    }
    found->second->users++;
    found->second->lastUsed = ++cache.clock;
    cache.reused++;
    return found->second->program;
}

auto ProgramCache::Register(const std::string& vertexSource, const std::string& fragmentSource, GLuint program) -> bool
{
    if (program == 0)
    {
        return false;
    }
    auto& cache = TheCache();
    std::lock_guard<std::mutex> guard(cache.lock);
    auto key = KeyOf(vertexSource, fragmentSource);
    if (cache.byKey.find(key) != cache.byKey.end())
    {
        return false;
    }
    cache.entries.push_front(Entry{key, program, 1, ++cache.clock});
    cache.byKey.emplace(std::move(key), cache.entries.begin());
    cache.byProgram.emplace(program, cache.entries.begin());
    return true;
}

auto ProgramCache::Release(GLuint program) -> bool
{
    auto& cache = TheCache();
    std::lock_guard<std::mutex> guard(cache.lock);
    auto found = cache.byProgram.find(program);
    if (found == cache.byProgram.end())
    {
        return false;
    }
    if (found->second->users > 0)
    {
        found->second->users--;
    }
    found->second->lastUsed = ++cache.clock;
    TrimIdle(cache);
    return true;
}

void ProgramCache::CountCompiled()
{
    auto& cache = TheCache();
    std::lock_guard<std::mutex> guard(cache.lock);
    cache.compiled++;
}

void ProgramCache::Counts(unsigned int& compiled, unsigned int& reused)
{
    auto& cache = TheCache();
    std::lock_guard<std::mutex> guard(cache.lock);
    compiled = cache.compiled;
    reused = cache.reused;
}

void ProgramCache::SetIdleCapacity(size_t count)
{
    auto& cache = TheCache();
    std::lock_guard<std::mutex> guard(cache.lock);
    cache.idleCapacity = count;
    TrimIdle(cache);
}

} // namespace Renderer
} // namespace libprojectM

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_SAMPLE_CACHE_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_SAMPLE_CACHE_HH__

#include "base/cache/associative_cache.hh"

namespace gem5
{

template <typename Entry>
class SampleCache : public AssociativeCache<Entry>
{
  protected:
    typedef replacement_policy::Base BaseReplacementPolicy;
    typedef typename Entry::IndexingPolicy IndexingPolicy;
    typedef typename Entry::KeyType KeyType;

  public:

    SampleCache(std::string_view name, const size_t num_entries,
                     const size_t associativity_,
                     BaseReplacementPolicy *repl_policy,
                     IndexingPolicy *indexing_policy,
                     Entry const &init_val = Entry())
        : AssociativeCache<Entry>(name, num_entries, associativity_, repl_policy, indexing_policy, init_val)
    {
    }

    ~SampleCache()  = default;

    SampleCache(const SampleCache&) = delete;
    SampleCache& operator=(const SampleCache&) = delete;

    virtual Entry*
    findVictim(const KeyType &key)
    {
        auto candidates = this->indexingPolicy->getPossibleEntries(key);

        if (candidates.size() == 0) {
            return nullptr;
        }

        auto victim = static_cast<Entry*>(this->replPolicy->getVictim(candidates));

        if (this->debugFlag && this->debugFlag->tracing() && victim->isValid()) {
            ::gem5::trace::getDebugLogger()->dprintf_flag(
                curTick(), name(), this->debugFlag->name(),
                "Replacing entry: %s\n", victim->print());
        }

        return victim;
    }
};

}

#endif

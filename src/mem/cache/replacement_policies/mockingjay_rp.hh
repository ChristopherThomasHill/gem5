#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_MOCKINGJAY_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_MOCKINGJAY_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "mem/cache/replacement_policies/mockingjay_rp.hh"
#include "mem/cache/replacement_policies/sample_cache.hh"
#include "mem/cache/tags/tagged_entry.hh"

namespace gem5
{

struct MockingjayRPParams;

namespace replacement_policy
{

class Mockingjay : public Base
{
  protected:
    struct MockingjayReplData : ReplacementData
    {
        const uint32_t inf_etr;
        const uint32_t set;

        bool valid;
        int8_t etr;

        MockingjayReplData(const uint32_t inf_etr, const uint32_t set);
        void ageEntry();
    };

    typedef uint64_t MockingjaySignature;

    MockingjaySignature buildMockingjaySignature(const PacketPtr pkt, bool hit);

    class SampleEntry : public TaggedEntry
    {
      public:
        MockingjaySignature signature;
        uint64_t timestamp;

        SampleEntry(TagExtractor ext)
          : TaggedEntry()
        {
            registerTagExtractor(ext);
        }
    };

    // Parameters
    const uint8_t numContexts;

    const uint32_t numCacheSets;

    const uint8_t pcSignatureBitLength;

    const uint8_t reuseDistanceGranularity;

    const uint16_t infiniteReuseDistane;

    const uint16_t maximumReuseDistane;

    const uint16_t maximumTimestamp;

    const double flexminPenalty;

    const uint16_t temporalDifference;

    SampleCache<SampleEntry> sampledCache;

    void trainAccess(const PacketPtr pkt, MockingjaySignature signature, uint32_t set);

    std::vector<int8_t> etrAgingClock;
    std::vector<int16_t> setTimestamp;

    void checkAgeCandidates(uint32_t set, const ReplacementCandidates& candidates);

    std::vector<int16_t> reusePredictor;

  public:
    typedef MockingjayRPParams Params;
    Mockingjay(const Params &p);
    ~Mockingjay() = default;

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Sets its last touch tick as the starting tick.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                                    override;

    /**
     * Touch an entry to update its replacement data.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be touched.
     * @param pkt Packet that generated this hit.
     * @param candidates Replacement candidates, selected by indexing policy.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data,
        const PacketPtr pkt, const ReplacementCandidates& candidates) override;
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
        override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be reset.
     * @param pkt Packet that generated this miss.
     * @param candidates Replacement candidates, selected by indexing policy.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data,
        const PacketPtr pkt, const ReplacementCandidates& candidates) override;
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
        override;

    /**
     * Find replacement victim using LRU timestamps.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @param set the set that this entry belongs to
     * @param way the way that this entry belongs to
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry(uint32_t set, uint32_t way) override;
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_MOCKINGJAY_RP_HH__

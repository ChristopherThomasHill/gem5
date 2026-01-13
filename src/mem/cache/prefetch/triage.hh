#ifndef __MEM_CACHE_PREFETCH_TRIAGE_HH__
#define __MEM_CACHE_PREFETCH_TRIAGE_HH__

#include "mem/cache/prefetch/lookup_based.hh"
#include "mem/packet.hh"


namespace gem5
{

struct TriagePrefetcherParams;

namespace prefetch
{

class Triage : public LookupBased
{
  public:
    Triage(const TriagePrefetcherParams &p);

    void calculatePrefetch(const PrefetchInfo &pfi,
                           std::vector<AddrPriority> &addresses,
                           const CacheAccessor &cache) override;

    void recvTimingResp(PacketPtr pkt) override;
};

} // namespace prefetch
} // namespace gem5

#endif // __MEM_CACHE_PREFETCH_TRIAGE_HH__

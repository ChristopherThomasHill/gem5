#ifndef __MEM_CACHE_PREFETCH_CACHE_ACCESSOR_HH__
#define __MEM_CACHE_PREFETCH_CACHE_ACCESSOR_HH__

#include "mem/cache/prefetch/queued.hh"
#include "mem/packet.hh"
#include "mem/qport.hh"

namespace gem5
{

struct LookupBasedPrefetcherParams;

namespace prefetch
{

class LookupBased : public Queued
{
  protected:

    class RequestPort : public QueuedRequestPort
    {
      private:
        LookupBased *prefetcher;

      public:

        RequestPort(const std::string &_name, LookupBased &_parent);

        bool recvTimingResp(PacketPtr pkt) override;
    };

    RequestPort requestPort;

    ReqPacketQueue reqQueue;
    SnoopRespPacketQueue snoopRespQueue;

  public:
    LookupBased(const LookupBasedPrefetcherParams &p);
    ~LookupBased() = default;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    virtual void recvTimingResp(PacketPtr pkt) = 0;
};

} // namespace prefetch
} // namespace gem5

#endif // __MEM_CACHE_PREFETCH_CACHE_ACCESSOR_HH__
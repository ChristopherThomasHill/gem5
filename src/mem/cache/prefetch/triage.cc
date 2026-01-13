#include "mem/cache/prefetch/triage.hh"

#include "params/TriagePrefetcher.hh"

namespace gem5
{

namespace prefetch
{

Triage::Triage(const TriagePrefetcherParams &p)
  : LookupBased(p)
{
}


void
Triage::calculatePrefetch(const PrefetchInfo &pfi,
                                    std::vector<AddrPriority> &addresses,
                                    const CacheAccessor &cache)
{
    RequestPtr req = std::make_shared<Request>(0x1000, blkSize,
                                                0, requestorId);
    req->taskId(context_switch_task_id::Prefetcher);
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    requestPort.schedTimingReq(pkt, curTick());
}

void 
Triage::recvTimingResp(PacketPtr pkt)
{
    printf("Got Packet %lx\n", pkt->getAddr());
}

} // namespace prefetch
} // namespace gem5

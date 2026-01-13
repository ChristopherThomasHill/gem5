#include "mem/cache/prefetch/lookup_based.hh"

#include "params/LookupBasedPrefetcher.hh"

namespace gem5
{

namespace prefetch
{

LookupBased::RequestPort::RequestPort(const std::string &_name, LookupBased &_parent) :
    QueuedRequestPort(_name, _parent.reqQueue, _parent.snoopRespQueue),
    prefetcher(&_parent)
{
}

bool LookupBased::RequestPort::recvTimingResp(PacketPtr pkt)
{
    prefetcher->recvTimingResp(pkt);
    return true;
}

LookupBased::LookupBased(const LookupBasedPrefetcherParams &p)
    : Queued(p),
      requestPort(p.name + ".mem_side_port", *this),
      reqQueue(*this, requestPort),
      snoopRespQueue(*this, requestPort)
{
}

Port &
LookupBased::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "mem_side") {
        return requestPort;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

} // namespace prefetch
} // namespace gem5

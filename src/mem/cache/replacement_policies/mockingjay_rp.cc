#include "mem/cache/replacement_policies/mockingjay_rp.hh"

#include "params/MockingjayRP.hh"

namespace gem5
{

namespace replacement_policy
{

Mockingjay::MockingjayReplData::MockingjayReplData(const uint32_t inf_etr, const uint32_t set)
  : inf_etr(inf_etr), set(set), valid(false), etr(0)
{
}

void
Mockingjay::MockingjayReplData::ageEntry()
{
    if (std::abs(etr) < inf_etr) {
        etr -= 1;
    }
}

Mockingjay::Mockingjay(const Params &p)
  :  Base(p),
     numContexts(p.num_contexts),
     numCacheSets(p.cache_size / p.cache_assoc / p.cache_block_size),
     pcSignatureBitLength(p.pc_signature_bit_length),
     reuseDistanceGranularity(p.reuse_distance_granularity),
     infiniteReuseDistane(p.infinite_reuse_distance),
     maximumReuseDistane(p.max_reuse_distance),
     maximumTimestamp(p.maximum_timestamp),
     flexminPenalty(p.flexmin_penalty),
     temporalDifference(p.temporal_difference),
     sampledCache("SampledCache",
                     p.sampled_cache_entries,
                     p.sampled_cache_assoc,
                     p.sampled_cache_replacement_policy,
                     p.sampled_cache_indexing_policy,
                     SampleEntry(genTagExtractor(p.sampled_cache_indexing_policy))),
    etrAgingClock(numCacheSets, 0),
    setTimestamp(numCacheSets, 0)
{
    uint32_t totalSignatures = std::exp2(pcSignatureBitLength + ceilLog2(numContexts) + 2 /*demand + hit*/);

    if (numContexts == 0) {
        reusePredictor.assign(totalSignatures, 0);
    } else {
        reusePredictor.assign(totalSignatures, infiniteReuseDistane);
    }
}

Mockingjay::MockingjaySignature
Mockingjay::buildMockingjaySignature(const PacketPtr pkt, bool hit)
{
    MockingjaySignature signature = hit ? 1 : 0;

    panic_if(pkt->req->hasContextId() && pkt->req->contextId() >= numContexts, "Have a context id greater than number of contexts");
    signature <<= ceilLog2(numContexts);
    signature |= pkt->req->hasContextId() ? pkt->req->contextId() : 0 /*no context value*/;

    signature <<= 1;
    signature |= (pkt->req->taskId() == context_switch_task_id::TaskId::Prefetcher) ? 1 : 0;

    // Construct and append fold of program counter
    uint64_t mask = (1ULL << pcSignatureBitLength) - 1;
    uint64_t value = pkt->req->hasPC() ? pkt->req->getPC() : 0 /*no pc value*/;
    uint64_t pcFold = 0;

    while (value > 0) {
        pcFold ^= (value & mask);
        value >>= pcSignatureBitLength;
    }

    signature <<= pcSignatureBitLength;
    signature |= pcFold;

    panic_if(signature >= reusePredictor.size(), "signature isn't within the size of reuse predictor");

    return signature;
}

void
Mockingjay::trainAccess(const PacketPtr pkt, MockingjaySignature accessSignature, uint32_t set)
{
    const TaggedEntry::KeyType key{pkt->getAddr(), pkt->isSecure()};

    SampleEntry *sample_entry = sampledCache.findEntry(key);

    if (sample_entry != nullptr) {
        MockingjaySignature last_signature = sample_entry->signature;
        uint16_t time_elapsed = (setTimestamp[set] + maximumTimestamp - sample_entry->timestamp) % maximumTimestamp;

        if (time_elapsed <= infiniteReuseDistane) {
            if (pkt->req->taskId() == context_switch_task_id::TaskId::Prefetcher) {
                time_elapsed = time_elapsed * flexminPenalty;
            }

            if (reusePredictor[last_signature] > time_elapsed + temporalDifference) {
                reusePredictor[last_signature] -= 1;
            } else if (reusePredictor[last_signature] < time_elapsed - temporalDifference) {
                reusePredictor[last_signature] += 1;
            }
        }

        sample_entry->signature = accessSignature;
        sample_entry->timestamp = setTimestamp[set];
        sampledCache.accessEntry(sample_entry);
    } else {
        sample_entry = sampledCache.findVictim(key);
        if (sample_entry != nullptr) {
            if (sample_entry->isValid()) {
                MockingjaySignature last_signature = sample_entry->signature;
                if (reusePredictor[last_signature] < infiniteReuseDistane) {
                    reusePredictor[last_signature] += 1;
                }
            }
            sampledCache.invalidate(sample_entry);

            sample_entry->signature = accessSignature;
            sample_entry->timestamp = setTimestamp[set];
            sampledCache.insertEntry(key, sample_entry);
        }
    }

    setTimestamp[set] = (setTimestamp[set] + 1) % maximumTimestamp;
}

void
Mockingjay::checkAgeCandidates(uint32_t set, const ReplacementCandidates& candidates)
{
    if (etrAgingClock[set] >= reuseDistanceGranularity) {
        etrAgingClock[set] = 0;
        for (const auto &candidate : candidates) {
            std::shared_ptr<MockingjayReplData> candidate_repl_data =
                std::static_pointer_cast<MockingjayReplData>(
                    candidate->replacementData);
            candidate_repl_data->ageEntry();
        }
    } else {
        etrAgingClock[set] += 1;
    }
}

void
Mockingjay::invalidate(const std::shared_ptr<ReplacementData>& replacement_data) {
    std::shared_ptr<MockingjayReplData> casted_replacement_data = std::static_pointer_cast<MockingjayReplData>(replacement_data);
    casted_replacement_data->valid = false;
    casted_replacement_data->etr = 0;    
}

void
Mockingjay::touch(const std::shared_ptr<ReplacementData>& replacement_data, const PacketPtr pkt, const ReplacementCandidates& candidates)
{
    std::shared_ptr<MockingjayReplData> casted_replacement_data =
        std::static_pointer_cast<MockingjayReplData>(replacement_data);

    panic_if(!pkt->req, "packet doesn't have a request in touch");

    if (pkt->isEviction()) {
        return;
    }

    MockingjaySignature signature = buildMockingjaySignature(pkt, true /*hit*/);

    trainAccess(pkt, signature, casted_replacement_data->set);

    checkAgeCandidates(casted_replacement_data->set, candidates);

    // Set Replacement Data
    uint16_t reuse_prediction = reusePredictor[signature];
    if (reuse_prediction > maximumReuseDistane) {
        casted_replacement_data->etr = infiniteReuseDistane / reuseDistanceGranularity;
    } else {
        casted_replacement_data->etr = reuse_prediction / reuseDistanceGranularity;
    }
    casted_replacement_data->valid = true;
}

void
Mockingjay::touch(const std::shared_ptr<ReplacementData>& replacement_data)
    const
{
    panic("Can't train Mockingjay's predictor without candidates information.");
}

void
Mockingjay::reset(const std::shared_ptr<ReplacementData>& replacement_data, const PacketPtr pkt, const ReplacementCandidates& candidates)
{
    std::shared_ptr<MockingjayReplData> casted_replacement_data =
        std::static_pointer_cast<MockingjayReplData>(replacement_data);
    
    panic_if(!pkt->req, "packet doesn't have a request in reset");

    if (pkt->isEviction()) {
        casted_replacement_data->etr = -1 * (infiniteReuseDistane / reuseDistanceGranularity);
        casted_replacement_data->valid = true;
        return;
    }

    MockingjaySignature signature = buildMockingjaySignature(pkt, false /*miss*/);

    trainAccess(pkt, signature, casted_replacement_data->set);

    checkAgeCandidates(casted_replacement_data->set, candidates);

    // Set Replacement Data
    uint16_t reuse_prediction = reusePredictor[signature];
    if (reuse_prediction > maximumReuseDistane) {
        casted_replacement_data->etr = infiniteReuseDistane / reuseDistanceGranularity;
    } else {
        casted_replacement_data->etr = reuse_prediction / reuseDistanceGranularity;
    }
    casted_replacement_data->valid = true;
}

void
Mockingjay::reset(const std::shared_ptr<ReplacementData>& replacement_data)
    const
{
    panic("Can't train Mockingjay's predictor without candidates information.");
}

ReplaceableEntry*
Mockingjay::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];

    int victim_etr = std::static_pointer_cast<MockingjayReplData>(victim->replacementData)->etr;

    for (const auto& candidate : candidates) {
        std::shared_ptr<MockingjayReplData> candidate_repl_data =
            std::static_pointer_cast<MockingjayReplData>(
                candidate->replacementData);

        // Stop searching for victims if an invalid entry is found
        if (!candidate_repl_data->valid) {
            return candidate;
        }

        // Update victim entry if necessary
        int candidate_etr = candidate_repl_data->etr;
        if (std::abs(candidate_etr) > std::abs(victim_etr) || ((std::abs(candidate_etr) == std::abs(victim_etr)) && (candidate_etr < 0))) {
            victim = candidate;
            victim_etr = candidate_etr;
        }
    }

    return victim;
}

std::shared_ptr<ReplacementData>
Mockingjay::instantiateEntry(uint32_t set, uint32_t way)
{
    return std::shared_ptr<ReplacementData>(new MockingjayReplData(infiniteReuseDistane / reuseDistanceGranularity, set));
}

std::shared_ptr<ReplacementData>
Mockingjay::instantiateEntry()
{
    panic("Can't initialize Mockingjay's Entries without set information.");
}

} // namespace replacement_policy
} // namespace gem5

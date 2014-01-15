#ifndef EBERLE_MERGESORT_LCP_LOSERTREE_H_
#define EBERLE_MERGESORT_LCP_LOSERTREE_H_

#include <iostream>

#include "../utils/types.h"
#include "../utils/utility-functions.h"
#include "../utils/lcp-string-losertree.h"
#include "../utils/eberle-inssort-lcp.h"
#include "../utils/verification-functions.h"

//#define EBERLE_LCP_LOSERTREE_MERGESORT_CHECK_LCPS

namespace eberle_mergesort
{

using namespace eberle_lcp_utils;
using namespace types;

typedef unsigned char* string;

template<unsigned K>
    static inline void
    eberle_mergesort_losertree_lcp_kway(string* strings, const LcpStringPtr& tmp, const LcpStringPtr& output, size_t length)
    {
        if (length <= 2 * K)
        {
            return eberle_inssort_lcp::inssort_lcp(strings, output, length);
        }

        //create ranges of the parts
        pair < size_t, size_t > ranges[K];
        eberle_utils::calculateRanges(ranges, K, length);

        // execute mergesorts for parts
        for (unsigned i = 0; i < K; i++)
        {
            const size_t offset = ranges[i].first;
            eberle_mergesort_losertree_lcp_kway<K>(strings + offset, output + offset, tmp + offset, ranges[i].second);
        }

        //merge
        LcpStringLoserTree<K> *loserTree = new LcpStringLoserTree<K>(tmp, ranges);
        loserTree->writeElementsToStream(output, length);
        delete loserTree;
    }

// K must be a power of two
template<unsigned K>
    static inline void
    eberle_mergesort_losertree_kway(string *strings, size_t n)
    {
        lcp_t* outputLcps = new lcp_t[n];
        string* tmpStrings = new string[n];
        lcp_t* tmpLcps = new lcp_t[n];

        LcpStringPtr output(strings, outputLcps);
        LcpStringPtr tmp(tmpStrings, tmpLcps);

        eberle_mergesort_losertree_lcp_kway<K>(strings, tmp, output, n);

#ifdef EBERLE_LCP_LOSERTREE_MERGESORT_CHECK_LCPS
        //check lcps
        eberle_utils::checkLcps(output, n, 0);
#endif //EBERLE_LCP_LOSERTREE_MERGESORT_CHECK_LCPS

        delete[] outputLcps;
        delete[] tmpStrings;
        delete[] tmpLcps;
    }

void
eberle_mergesort_losertree_4way(string *strings, size_t n)
{
    eberle_mergesort_losertree_kway<4>(strings, n);
}

void
eberle_mergesort_losertree_16way(string *strings, size_t n)
{
    eberle_mergesort_losertree_kway<16>(strings, n);
}

void
eberle_mergesort_losertree_32way(string *strings, size_t n)
{
    eberle_mergesort_losertree_kway<32>(strings, n);
}

void
eberle_mergesort_losertree_64way(string *strings, size_t n)
{
    eberle_mergesort_losertree_kway<64>(strings, n);
}

CONTESTANT_REGISTER(eberle_mergesort_losertree_4way, "eberle/mergesort_lcp_losertree_4way", "Mergesort with lcp aware Losertree by Andreas Eberle")
CONTESTANT_REGISTER(eberle_mergesort_losertree_16way, "eberle/mergesort_lcp_losertree_16way", "Mergesort with lcp aware Losertree by Andreas Eberle")
CONTESTANT_REGISTER(eberle_mergesort_losertree_32way, "eberle/mergesort_lcp_losertree_32way", "Mergesort with lcp aware Losertree by Andreas Eberle")
CONTESTANT_REGISTER(eberle_mergesort_losertree_64way, "eberle/mergesort_lcp_losertree_64way", "Mergesort with lcp aware Losertree by Andreas Eberle")

}
 // namespace eberle_mergesort

#endif // EBERLE_MERGESORT_LCP_LOSERTREE_H_

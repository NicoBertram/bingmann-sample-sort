/******************************************************************************
 * src/sequential/bingmann-radix_sort.h
 *
 * Experiments with sequential radix sort implementations.
 * Based on rantala/msd_c?.h
 *
 ******************************************************************************
 * Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

namespace bingmann_radix_sort {

typedef unsigned char* string;

static inline void
insertion_sort(string* strings, int n, size_t depth)
{
    for (string* i = strings + 1; --n > 0; ++i) {
        string* j = i;
        string tmp = *i;
        while (j > strings) {
            string s = *(j-1)+depth;
            string t = tmp+depth;
            while (*s == *t && *s != 0) ++s, ++t;
            if (*s <= *t) break;
            *j = *(j-1);
            --j;
        }
        *j = tmp;
    }
}

static inline void
insertion_sort(string* str_begin, string* str_end, size_t depth)
{
    for (string* i = str_begin + 1; i != str_end; ++i) {
        string* j = i;
        string tmp = *i;
        while (j > str_begin) {
            string s = *(j-1)+depth;
            string t = tmp+depth;
            while (*s == *t && *s != 0) ++s, ++t;
            if (*s <= *t) break;
            *j = *(j-1);
            --j;
        }
        *j = tmp;
    }
}

static void
msd_CE(string* strings, size_t n, size_t depth)
{
    if (n < 32)
        return insertion_sort(strings, n, depth);

    // count character occurances
    size_t bktsize[256] = { 0 };
    for (size_t i=0; i < n; ++i)
        ++bktsize[strings[i][depth]];

    string* sorted = (string*) malloc(n * sizeof(string));

    // prefix sum
    size_t bktindex[256];
    bktindex[0] = 0;
    for (size_t i=1; i < 256; ++i)
        bktindex[i] = bktindex[i-1]+bktsize[i-1];

    // distribute
    for (size_t i=0; i < n; ++i)
        sorted[ bktindex[strings[i][depth]]++ ] = strings[i];

    memcpy(strings, sorted, n*sizeof(string));
    free(sorted);

    // recursion
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 256; ++i) {
        if (bktsize[i] == 0) continue;
        msd_CE(strings+bsum, bktsize[i], depth+1);
        bsum += bktsize[i];
    }
}

void bingmann_msd_CE(string* strings, size_t n) { return msd_CE(strings,n,0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CE, "bingmann/msd_CE (rantala CE original)")

static void
msd_CE2(string* strings, size_t n, size_t depth)
{
    if (n < 32)
        return insertion_sort(strings, n, depth);

    // count character occurances
    size_t bkt[256+1] = {0};
    for (size_t i=0; i < n; ++i)
        ++bkt[strings[i][depth]];

    string* sorted = (string*)malloc(n*sizeof(string));

    // prefix sum
    for (size_t i=1; i <= 256; ++i)
        bkt[i] += bkt[i-1];

    // distribute
    for (size_t i=0; i < n; ++i)
        sorted[ --bkt[strings[i][depth]] ] = strings[i];

    memcpy(strings, sorted, n*sizeof(string));
    free(sorted);

    // recursion
    for (size_t i=1; i < 256; ++i) {
        if (bkt[i] == bkt[i+1]) continue;
        //if (bkt[i]+1 >= bkt[i+1]) continue;
        msd_CE2(strings+bkt[i], bkt[i+1]-bkt[i], depth+1);
    }
}

void bingmann_msd_CE2(string* strings, size_t n) { return msd_CE2(strings,n,0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CE2, "bingmann/msd_CE2 (CE with reused prefix sum)")

static void
msd_CE3(string* str_begin, string* str_end, size_t depth)
{
    if (str_begin + 32 > str_end)
        return insertion_sort(str_begin, str_end, depth);

    // count character occurances
    size_t bkt[256+1] = { 0 };
    for (string* str = str_begin; str != str_end; ++str)
        ++bkt[(*str)[depth]];

    string* sorted = (string*)malloc((str_end - str_begin) * sizeof(string));

    // prefix sum
    for (size_t i=1; i <= 256; ++i)
        bkt[i] += bkt[i-1];

    // distribute
    for (string* str = str_begin; str != str_end; ++str)
        sorted[ --bkt[(*str)[depth]] ] = *str;

    memcpy(str_begin, sorted, (str_end - str_begin) * sizeof(string));
    free(sorted);

    // recursion
    for (size_t i=1; i < 256; ++i) {
        if (bkt[i] == bkt[i+1]) continue;
        //if (bkt[i]+1 >= bkt[i+1]) continue;
        msd_CE3(str_begin+bkt[i], str_begin+bkt[i+1], depth+1);
    }
}

void bingmann_msd_CE3(string* strings, size_t n) { return msd_CE3(strings,strings+n,0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CE3, "bingmann/msd_CE3 (CE2 with iterators)")

template <typename BucketType>
struct distblock {
    string ptr;
    BucketType bkt;
};

template <typename BucketsizeType>
static void msd_CI(string* strings, size_t n, size_t depth)
{
    if (n < 32) {
        insertion_sort(strings, n, depth);
        return;
    }
    BucketsizeType bktsize[256] = {0};
    string restrict oracle = (string) malloc(n);
    for (size_t i=0; i < n; ++i)
        oracle[i] = strings[i][depth];
    for (size_t i=0; i < n; ++i)
        ++bktsize[oracle[i]];
    static size_t bktindex[256];
    bktindex[0] = bktsize[0];
    BucketsizeType last_bkt_size = bktsize[0];
    for (unsigned i=1; i < 256; ++i) {
        bktindex[i] = bktindex[i-1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }
    for (size_t i=0; i < n-last_bkt_size; ) {
        distblock<uint8_t> tmp = { strings[i], oracle[i] };
        while (1) {
            // Continue until the current bucket is completely in
            // place
            if (--bktindex[tmp.bkt] <= i)
                break;
            // backup all information of the position we are about
            // to overwrite
            size_t backup_idx = bktindex[tmp.bkt];
            distblock<uint8_t> tmp2 = { strings[backup_idx], oracle[backup_idx] };
            // overwrite everything, ie. move the string to correct
            // position
            strings[backup_idx] = tmp.ptr;
            oracle[backup_idx]  = tmp.bkt;
            tmp = tmp2;
        }
        // Commit last pointer to place. We don't need to copy the
        // oracle entry, it's not read after this.
        strings[i] = tmp.ptr;
        i += bktsize[tmp.bkt];
    }
    free(oracle);
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 256; ++i) {
        if (bktsize[i] == 0) continue;
        msd_CI<BucketsizeType>(strings+bsum, bktsize[i], depth+1);
        bsum += bktsize[i];
    }
}

void bingmann_msd_CI(string* strings, size_t n) { msd_CI<size_t>(strings, n, 0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CI, "bingmann/msd_CI (rantala CI original with oracle)")

template <typename BucketsizeType>
static void msd_CI2(string* strings, size_t n, size_t depth)
{
    if (n < 32) {
        insertion_sort(strings, n, depth);
        return;
    }
    BucketsizeType bktsize[256] = {0};
    for (size_t i=0; i < n; ++i)
        ++bktsize[ strings[i][depth] ];
    static size_t bktindex[256];
    bktindex[0] = bktsize[0];
    BucketsizeType last_bkt_size = bktsize[0];
    for (unsigned i=1; i < 256; ++i) {
        bktindex[i] = bktindex[i-1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }
    for (size_t i=0; i < n-last_bkt_size; ) {
        distblock<uint8_t> tmp = { strings[i], strings[i][depth] };
        while (1) {
            // Continue until the current bkt is completely in
            // place
            if (--bktindex[tmp.bkt] <= i)
                break;
            // backup all information of the position we are about
            // to overwrite
            size_t backup_idx = bktindex[tmp.bkt];
            distblock<uint8_t> tmp2 = { strings[backup_idx], strings[backup_idx][depth] };
            // overwrite everything, ie. move the string to correct
            // position
            strings[backup_idx] = tmp.ptr;
            tmp = tmp2;
        }
        // Commit last pointer to place. We don't need to copy the
        // oracle entry, it's not read after this.
        strings[i] = tmp.ptr;
        i += bktsize[tmp.bkt];
    }
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 256; ++i) {
        if (bktsize[i] == 0) continue;
        msd_CI2<BucketsizeType>(strings+bsum, bktsize[i], depth+1);
        bsum += bktsize[i];
    }
}

void bingmann_msd_CI2(string* strings, size_t n) { msd_CI2<size_t>(strings, n, 0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CI2, "bingmann/msd_CI2 (CI without oracle)")

static void
msd_CI3(string* strings, size_t n, size_t depth)
{
    if (n < 32)
        return insertion_sort(strings, n, depth);

    // count character occurances
    size_t bktsize[256] = { 0 };
    for (size_t i=0; i < n; ++i)
        ++bktsize[ strings[i][depth] ];

    // prefix sum
    size_t bktindex[256];
    bktindex[0] = bktsize[0];
    size_t last_bkt_size = bktsize[0];
    for (unsigned i=1; i < 256; ++i) {
        bktindex[i] = bktindex[i-1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }

    // premute in-place
    for (size_t i=0, j; i < n-last_bkt_size; )
    {
        while ( (j = --bktindex[ strings[i][depth] ]) > i )
        {
            std::swap(strings[i], strings[j]);
        }
        i += bktsize[ strings[i][depth] ];
    }

    // recursion
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 256; ++i) {
        if (bktsize[i] == 0) continue;
        msd_CI3(strings+bsum, bktsize[i], depth+1);
        bsum += bktsize[i];
    }
}

void bingmann_msd_CI3(string* strings, size_t n) { msd_CI3(strings, n, 0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CI3, "bingmann/msd_CI3 (CI2 with swap operations)")

// Note: CI in-place variant cannot be done with just one prefix-sum bucket
// array, because during in-place permutation the beginning _and_ end
// boundaries of each bucket must be kept.

static void
msd_CI4(string* strings, size_t n, size_t depth)
{
    if (n < 32)
        return insertion_sort(strings, n, depth);

    // count character occurances
    size_t bktsize[256] = { 0 };
    for (size_t i=0; i < n; ++i)
        ++bktsize[ strings[i][depth] ];

    // inclusive prefix sum
    size_t bkt[256];
    bkt[0] = bktsize[0];
    size_t last_bkt_size = bktsize[0];
    for (unsigned i=1; i < 256; ++i) {
        bkt[i] = bkt[i-1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }

    // premute in-place
    for (size_t i=0, j; i < n-last_bkt_size; )
    {
        string perm = strings[i];
        if (bkt[ perm[depth] ] == i)
        {
            i += bktsize[ perm[depth] ];
            continue;
        }
        while ( (j = --bkt[ perm[depth] ]) > i )
        {
            std::swap(perm, strings[j]);
        }
        assert(j == i);
        strings[i] = perm;
        i += bktsize[ perm[depth] ];
    }

    // recursion
    size_t bsum = bktsize[0];
    for (size_t i=1; i < 256; ++i) {
        if (bktsize[i] == 0) continue;
        msd_CI4(strings+bsum, bktsize[i], depth+1);
        bsum += bktsize[i];
    }
}

void bingmann_msd_CI4(string* strings, size_t n) { msd_CI4(strings, n, 0); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CI4, "bingmann/msd_CI4 (CI3 without swap operations)")

static void
msd_CE_nr(string* strings, size_t n)
{
    if (n < 32)
        return insertion_sort(strings,n,0);

    struct RadixStep
    {
        string* str;
        size_t bkt[256+1];
        size_t idx;

        RadixStep(string* strings, size_t n, size_t depth)
        {
            memset(bkt, 0, sizeof(bkt));

            for (size_t i=0; i < n; ++i)
                ++bkt[ strings[i][depth] ];

            string* sorted = (string*)malloc(n * sizeof(string));

            for (unsigned i=1; i <= 256; ++i)
                bkt[i] += bkt[i-1];

            for (size_t i=0; i < n; ++i)
                sorted[ --bkt[strings[i][depth]] ] = strings[i];

            memcpy(strings, sorted, n * sizeof(string));
            free(sorted);

            str = strings;
            idx = 0;        // will increment to 1 on first process
        }
    };

    std::stack< RadixStep, std::vector<RadixStep> > radixstack;
    radixstack.push( RadixStep(strings,n,0) );
    
    while ( radixstack.size() )
    {
        RadixStep& rs = radixstack.top();

        while ( ++rs.idx < 256 )
        {
            // process the bucket rs.idx

            if (rs.bkt[rs.idx] == rs.bkt[rs.idx+1])
                ;
            else if (rs.bkt[rs.idx] + 32 > rs.bkt[rs.idx+1])
            {
                insertion_sort(rs.str + rs.bkt[ rs.idx ],
                               rs.bkt[ rs.idx+1 ] - rs.bkt[ rs.idx ],
                               radixstack.size());
            }
            else
            {
                radixstack.push( RadixStep(rs.str + rs.bkt[ rs.idx ],
                                           rs.bkt[ rs.idx+1 ] - rs.bkt[ rs.idx ],
                                           radixstack.size()) );
                break;
            }
        }

        if (radixstack.top().idx == 256) {      // rs maybe have been invalidated
            radixstack.pop();
        }
    }
}

void bingmann_msd_CE_nr(string* strings, size_t n) { return msd_CE_nr(strings,n); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CE_nr, "bingmann/msd_CE_nr (CE non-recursive)")

static void
msd_CI_nr(string* strings, size_t n)
{
    if (n < 32)
        return insertion_sort(strings,n,0);

    struct RadixStep
    {
        string* str;
        size_t bkt[256+1];
        size_t idx;

        RadixStep(string* strings, size_t n, size_t depth)
        {
            // count character occurances
            size_t bktsize[256] = { 0 };
            for (size_t i=0; i < n; ++i)
                ++bktsize[ strings[i][depth] ];

            // inclusive prefix sum
            bkt[0] = bktsize[0];
            size_t last_bkt_size = bktsize[0], last_bkt_num = 0;
            for (unsigned i=1; i < 256; ++i) {
                bkt[i] = bkt[i-1] + bktsize[i];
                if (bktsize[i]) {
                    last_bkt_size = bktsize[i];
                    last_bkt_num = i;
                }
            }
            bkt[256] = bkt[255];
            assert(bkt[256] == n);

            // premute in-place
            for (size_t i=0, j; i < n-last_bkt_size; )
            {
                string perm = strings[i];
                if (bkt[ perm[depth] ] == i)
                {
                    i += bktsize[ perm[depth] ];
                    continue;
                }
                while ( (j = --bkt[ perm[depth] ]) > i )
                {
                    std::swap(perm, strings[j]);
                }
                assert(j == i);
                strings[i] = perm;
                i += bktsize[ perm[depth] ];
            }
            // update bkt boundary of last (unpermuted) bucket
            bkt[last_bkt_num] = n-last_bkt_size; 

            str = strings;
            idx = 0;        // will increment to 1 on first process
        }
    };

    std::stack< RadixStep, std::vector<RadixStep> > radixstack;
    radixstack.push( RadixStep(strings,n,0) );
    
    while ( radixstack.size() )
    {
        RadixStep& rs = radixstack.top();

        while ( ++rs.idx < 256 )
        {
            // process the bucket rs.idx

            if (rs.bkt[rs.idx] == rs.bkt[rs.idx+1])
                ;
            else if (rs.bkt[rs.idx] + 32 > rs.bkt[rs.idx+1])
            {
                insertion_sort(rs.str + rs.bkt[ rs.idx ],
                               rs.bkt[ rs.idx+1 ] - rs.bkt[ rs.idx ],
                               radixstack.size());
            }
            else
            {
                radixstack.push( RadixStep(rs.str + rs.bkt[ rs.idx ],
                                           rs.bkt[ rs.idx+1 ] - rs.bkt[ rs.idx ],
                                           radixstack.size()) );
                break;
            }
        }

        if (radixstack.top().idx == 256) {      // rs maybe have been invalidated
            radixstack.pop();
        }
    }
}

void bingmann_msd_CI_nr(string* strings, size_t n) { return msd_CI_nr(strings,n); }
CONTESTANT_REGISTER_UCARRAY(bingmann_msd_CI_nr, "bingmann/msd_CI_nr (CI non-recursive)")

} // namespace bingmann_radix_sort

/*******************************************************************************
 * src/sequential/bingmann-sample_sortBSC.cpp
 *
 * Experiments with sequential Super Scalar String Sample-Sort (S^5).
 *
 * Binary search on splitters with bucket cache. Also with equality branch.
 *
 *******************************************************************************
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
 ******************************************************************************/

#include "bingmann-sample_sort.hpp"
#include "bingmann-sample_sort_tree_builder.hpp"

namespace bingmann_sample_sortBSC_original {

using namespace bingmann_sample_sort;

// ----------------------------------------------------------------------------

/// binary search on splitter array for bucket number
static inline unsigned int
find_bkt_binsearch(const key_type& key, const key_type* splitter, size_t numsplitters)
{
    unsigned int lo = 0, hi = numsplitters;

    while (lo < hi)
    {
        unsigned int mid = (lo + hi) >> 1;
        assert(mid < numsplitters);

        if (key <= splitter[mid])
            hi = mid;
        else    // (key > splitter[mid])
            lo = mid + 1;
    }

#if 0
    // Verify result of binary search:
    int pos = numsplitters - 1;
    while (pos >= 0 && key <= splitter[pos]) --pos;
    pos++;

    //std::cout << "lo " << lo << " hi " << hi << " pos " << pos << "\n";
    assert(lo == pos);
#endif

    size_t b = lo * 2;                                     // < bucket
    if (lo < numsplitters && splitter[lo] == key) b += 1;  // equal bucket

    return b;
}

/// binary search on splitter array for bucket number
inline unsigned int
find_bkt_assembler(const key_type& key, const key_type* splitter, size_t numsplitters)
{
/*
    // straightforward binary search
    unsigned int lo = 0, hi = numsplitters;

    while ( lo < hi )
    {
        unsigned int mid = (lo + hi) >> 1;
        assert(mid < numsplitters);

        if (key <= splitter[mid])
            hi = mid;
        else // (key > splitter[mid])
            lo = mid + 1;
    }
*/
    unsigned int lo;

#if 0
    // assember code of binary search generated by GCC
    asm ("xorl   %%edx, %%edx \n"         // edx = lo
         "movl	%[numsplitters], %%ecx \n"// ecx = hi
         // body of while loop
         ".myL2985: \n"
         "leal   (%%rcx,%%rdx), %%eax \n"
         "shrl	%%eax \n"                 // eax = mid = (lo + hi) >> 1;
         "movl	%%eax, %%edi \n"
         "cmpq	%[key], (%[splitter],%%rax,8) \n"
         "jb	.myL2987 \n"
         ".myL3019: \n"
         "cmpl	%%eax, %%edx \n"          // lo < hi
         "movl	%%eax, %%ecx \n"          // ecx = lo
         "jae	.myL2989 \n"              // if (lo < hi) -> end
         "addl	%%edx, %%eax \n"          // eax = ecx + edx = lo + hi
         "shrl	%%eax \n"                 // eax = (lo+hi) >> 1;
         "movl	%%eax, %%edi \n"
         "cmpq	(%[splitter],%%rdi,8), %[key] \n"
         "jbe	.myL3019 \n"
         ".myL2987: \n"
         "leal	1(%%rax), %%edx \n"
         "cmpl	%%edx, %%ecx \n"
         "ja	.myL2985 \n"
         ".myL2989: \n"
         : "=d" (lo)
         :[numsplitters] "g" (numsplitters), [key] "r" (key), [splitter] "r" (splitter)
         : "eax", "ecx", "edi");
#endif

    // hand-coded assembler binary search with conditional moves
    asm ("xorl   %%ecx, %%ecx \n"         // ecx = lo
         "movl	%[numsplitters], %%edx \n"// edx = hi
         // body of while loop
         "1: \n"
         "leal   (%%rcx,%%rdx), %%eax \n"
         "shrl	%%eax \n"                 // eax = mid = (lo + hi) >> 1;
         "cmpq	(%[splitter],%%rax,8), %[key] \n"
         "cmovbe %%eax, %%edx \n"
         "leal	1(%%rax), %%eax \n"
         "cmova  %%eax, %%ecx \n"
         "cmpl	%%edx, %%ecx \n"          // lo < hi
         "jb	1b \n"                    // if (lo < hi) -> loop
         : "=&c" (lo)
         :[numsplitters] "g" (numsplitters), [key] "r" (key), [splitter] "r" (splitter)
         : "eax", "edx");

#if 0
    // Verify result of binary search:
    int pos = numsplitters - 1;
    while (pos >= 0 && key <= splitter[pos]) --pos;
    pos++;

    std::cout << "lo " << lo << " pos " << pos << "\n";
    if (lo != pos) abort();
#endif

    size_t b = lo * 2;                                     // < bucket
    if (lo < numsplitters && splitter[lo] == key) b += 1;  // equal bucket

    return b;
}

/// Variant of string sample-sort: use binary search on splitters, no caching.
template <unsigned int(* find_bkt) (const key_type&, const key_type*, size_t)>
void sample_sortBSC(string* strings, size_t n, size_t depth)
{
#if 0
    static const size_t numsplitters = 32;
#else
    //static const size_t l2cache = 256*1024;

    // bounding equations:
    // splitters            + bktsize
    // n * sizeof(key_type) + (2*n+1) * sizeof(size_t) <= l2cache

    static const size_t numsplitters = (l2cache - sizeof(size_t)) / (sizeof(key_type) + 2 * sizeof(size_t));

#endif

    if (n < g_samplesort_smallsort)
    {
        return sample_sort_small_sort(strings, n, depth);
    }

    //std::cout << "numsplitters: " << numsplitters << "\n";

    // step 1: select splitters with oversampling

    size_t samplesize = oversample_factor * numsplitters;

    key_type* samples = new key_type[samplesize];

    LCGRandom rng(&samples);

    for (unsigned int i = 0; i < samplesize; ++i)
    {
        samples[i] = get_char<key_type>(strings[rng() % n], depth);
    }

    std::sort(samples, samples + samplesize);

    key_type splitter[numsplitters];
    unsigned char splitter_lcp[numsplitters + 1];

    TreeBuilderPreorder<numsplitters>(
        splitter, splitter_lcp,
        samples, samplesize);

    delete[] samples;

    // step 2: classify all strings and count bucket sizes

    static const size_t bktnum = 2 * numsplitters + 1;

    uint16_t* bktcache = new uint16_t[n];

    for (size_t si = 0; si < n; ++si)
    {
        // binary search in splitter with equal check
        key_type key = get_char<key_type>(strings[si], depth);

        unsigned int b = find_bkt(key, splitter, numsplitters);

        assert(b < bktnum);

        bktcache[si] = b;
    }

    size_t* bktsize = new size_t[bktnum];
    memset(bktsize, 0, bktnum * sizeof(size_t));

    for (size_t si = 0; si < n; ++si)
        ++bktsize[bktcache[si]];

    // step 3: prefix sum

    size_t bktindex[bktnum];
    bktindex[0] = bktsize[0];
    size_t last_bkt_size = bktsize[0];
    for (unsigned int i = 1; i < bktnum; ++i) {
        bktindex[i] = bktindex[i - 1] + bktsize[i];
        if (bktsize[i]) last_bkt_size = bktsize[i];
    }
    assert(bktindex[bktnum - 1] == n);

    // step 4: premute in-place

    for (size_t i = 0, j; i < n - last_bkt_size; )
    {
        string perm = strings[i];
        uint16_t permbkt = bktcache[i];

        while ((j = --bktindex[permbkt]) > i)
        {
            std::swap(perm, strings[j]);
            std::swap(permbkt, bktcache[j]);
        }

        strings[i] = perm;
        i += bktsize[permbkt];
    }

    delete[] bktcache;

    // step 5: recursion

    size_t i = 0, bsum = 0;
    while (i < bktnum - 1)
    {
        // i is even -> bkt[i] is less-than bucket
        if (bktsize[i] > 1)
        {
            if (!g_toplevel_only)
                sample_sortBSC<find_bkt>(strings + bsum, bktsize[i],
                                         depth + splitter_lcp[i / 2]);
        }
        bsum += bktsize[i++];

        // i is odd -> bkt[i] is equal bucket
        if (bktsize[i] > 1)
        {
            if ((splitter[i / 2] & 0xFF) == 0) {
                // equal-bucket has NULL-terminated key, done.
            }
            else {
                if (!g_toplevel_only)
                    sample_sortBSC<find_bkt>(strings + bsum, bktsize[i],
                                             depth + sizeof(key_type));
            }
        }
        bsum += bktsize[i++];
    }
    if (bktsize[i] > 0)
    {
        if (!g_toplevel_only)
            sample_sortBSC<find_bkt>(strings + bsum, bktsize[i], depth);
    }
    bsum += bktsize[i++];
    assert(i == bktnum && bsum == n);

    delete[] bktsize;
}

void bingmann_sample_sortBSC_original(string* strings, size_t n)
{
    sample_sortBSC<find_bkt_binsearch>(strings, n, 0);
}

void bingmann_sample_sortBSCA_original(string* strings, size_t n)
{
    sample_sortBSC<find_bkt_assembler>(strings, n, 0);
}

// ----------------------------------------------------------------------------

/// binary search on splitter array for bucket number
inline unsigned int
find_bkt_equal(const key_type& key, const key_type* splitter, size_t numsplitters)
{
    // straightforward binary search
    unsigned int lo = 0, hi = numsplitters;

    while (lo < hi)
    {
        unsigned int mid = (lo + hi) >> 1;
        assert(mid < numsplitters);

        if (key == splitter[mid])
            return 2 * mid + 1;
        else if (key < splitter[mid])
            hi = mid;
        else       // (key > splitter[mid])
            lo = mid + 1;
    }

    return 2 * lo; // < bucket
}

/// binary search on splitter array for bucket number
inline unsigned int
find_bkt_asmequal(const key_type& key, const key_type* splitter, size_t numsplitters)
{
    unsigned int lo;

    // hand-coded assembler binary search with conditional moves
    asm ("xorl   %%ecx, %%ecx \n"         // ecx = lo
         "movl	%[numsplitters], %%edx \n"// edx = hi
         // body of while loop
         "1: \n"
         "leal   (%%rcx,%%rdx), %%eax \n"
         "shrl	%%eax \n"                 // eax = mid = (lo + hi) >> 1;
         "cmpq	(%[splitter],%%rax,8), %[key] \n"
         "je     2f \n"
         "cmovb  %%eax, %%edx \n"
         "leal	1(%%rax), %%eax \n"
         "cmova  %%eax, %%ecx \n"
         "cmpl	%%edx, %%ecx \n"           // lo < hi
         "jb	1b \n"                     // if (lo < hi) -> loop
         "leal   (%%ecx,%%ecx), %%eax \n"  // return 2 * lo
         "jmp    3f \n"
         "2: \n"
         "leal   1(%%rax,%%rax), %%eax \n" // return 2 * lo + 1
         "3: \n"
         : "=&a" (lo)
         :[numsplitters] "g" (numsplitters), [key] "r" (key), [splitter] "r" (splitter)
         : "ecx", "edx");

    //assert(find_bkt_equal(key,splitter,numsplitters) == lo);

    return lo;
}

void bingmann_sample_sortBSCE_original(string* strings, size_t n)
{
    bingmann_sample_sortBSC_original::sample_sortBSC<find_bkt_equal>(strings, n, 0);
}

void bingmann_sample_sortBSCEA_original(string* strings, size_t n)
{
    bingmann_sample_sortBSC_original::sample_sortBSC<find_bkt_asmequal>(strings, n, 0);
}

// ----------------------------------------------------------------------------

} // namespace bingmann_sample_sortBSC_original

/******************************************************************************/

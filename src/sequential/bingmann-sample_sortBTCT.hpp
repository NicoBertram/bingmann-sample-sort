/*******************************************************************************
 * src/sequential/bingmann-sample_sortBTCT.hpp
 *
 * Experiments with sequential Super Scalar String Sample-Sort (S^5).
 *
 * Binary tree search with bucket cache.
 *
 *******************************************************************************
 * Copyright (C) 2013-2017 Timo Bingmann <tb@panthema.net>
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

#ifndef PSS_SRC_SEQUENTIAL_BINGMANN_SAMPLE_SORTBTCT_HEADER
#define PSS_SRC_SEQUENTIAL_BINGMANN_SAMPLE_SORTBTCT_HEADER

#include "bingmann-sample_sort.hpp"
#include "bingmann-sample_sort_tree_builder.hpp"
#include "../tools/stringset.hpp"

namespace bingmann_sample_sort {

template <size_t TreeBits>
class ClassifyTreeCalcSimple
{
public:
    static const size_t treebits = TreeBits;
    static const size_t numsplitters = (1 << treebits) - 1;

    key_type splitter_tree[numsplitters + 1];

    //! binary search on splitter array for bucket number
    unsigned int find_bkt(const key_type& key) const
    {
        // binary tree traversal without left branch

        unsigned int i = 1;

        while (i <= numsplitters)
        {
#if 0
            // in gcc-4.6.3 this produces a SETA, LEA sequence
            i = 2 * i + (key <= splitter_tree[i] ? 0 : 1);
#else
            // in gcc-4.6.3 this produces two LEA and a CMOV sequence, which is
            // slightly faster
            if (key <= splitter_tree[i])
                i = 2 * i + 0;
            else    // (key > splitter_tree[i])
                i = 2 * i + 1;
#endif
        }

        i -= numsplitters + 1;

        size_t b = i * 2;                                        // < bucket
        if (i < numsplitters && get_splitter(i) == key) b += 1;  // equal bucket

        return b;
    }

    //! classify all strings in area by walking tree and saving bucket id
    template <typename StringSet>
    void classify(
        const StringSet& strset,
        typename StringSet::Iterator begin, typename StringSet::Iterator end,
        uint16_t* bktout, size_t depth) const
    {
        while (begin != end)
        {
            key_type key = strset.get_uint64(*begin++, depth);
            *bktout++ = find_bkt(key);
        }
    }

    //! classify all strings in area by walking tree and saving bucket id
    void classify(string* strB, string* strE, uint16_t* bktout,
                  size_t depth)
    {
        return classify(
            parallel_string_sorting::UCharStringSet(strB, strE),
            strB, strE, bktout, depth);
    }

    //! return a splitter
    key_type get_splitter(unsigned int i) const
    {
        return splitter_tree[
            TreeCalculations < treebits > ::pre_to_levelorder(i + 1)];
    }

    //! build tree and splitter array from sample
    void build(key_type* samples, size_t samplesize,
               unsigned char* splitter_lcp)
    {
        bingmann_sample_sort::TreeBuilderLevelOrder<numsplitters>(
            splitter_tree, splitter_lcp, samples, samplesize);
    }
};

template <size_t TreeBits>
class ClassifyTreeCalcUnroll
{
public:
    static const size_t treebits = TreeBits;
    static const size_t numsplitters = (1 << treebits) - 1;

    key_type splitter_tree[numsplitters + 1];

    //! binary search on splitter array for bucket number
    __attribute__ ((optimize("unroll-all-loops")))
    unsigned int find_bkt(const key_type& key) const
    {
        // binary tree traversal without left branch

        unsigned int i = 1;

        for (size_t l = 0; l < treebits; ++l)
        {
#if 0
            // in gcc-4.6.3 this produces a SETA, LEA sequence
            i = 2 * i + (key <= splitter_tree[i] ? 0 : 1);
#else
            // in gcc-4.6.3 this produces two LEA and a CMOV sequence, which is
            // slightly faster
            if (key <= splitter_tree[i])
                i = 2 * i + 0;
            else    // (key > splitter_tree[i])
                i = 2 * i + 1;
#endif
        }

        i -= numsplitters + 1;

        size_t b = i * 2;                                        // < bucket
        if (i < numsplitters && get_splitter(i) == key) b += 1;  // equal bucket

        return b;
    }

    //! classify all strings in area by walking tree and saving bucket id
    template <typename StringSet>
    void classify(
        const StringSet& strset,
        typename StringSet::Iterator begin, typename StringSet::Iterator end,
        uint16_t* bktout, size_t depth) const
    {
        while (begin != end)
        {
            key_type key = strset.get_uint64(*begin++, depth);
            *bktout++ = find_bkt(key);
        }
    }

    //! classify all strings in area by walking tree and saving bucket id
    void classify(string* strB, string* strE, uint16_t* bktout,
                  size_t depth)
    {
        return classify(
            parallel_string_sorting::UCharStringSet(strB, strE),
            strB, strE, bktout, depth);
    }

    //! return a splitter
    key_type get_splitter(unsigned int i) const
    {
        return splitter_tree[
            TreeCalculations < treebits > ::pre_to_levelorder(i + 1)];
    }

    //! build tree and splitter array from sample
    void build(
        key_type* samples, size_t samplesize, unsigned char* splitter_lcp)
    {
        bingmann_sample_sort::TreeBuilderLevelOrder<numsplitters>(
            splitter_tree, splitter_lcp,
            samples, samplesize);
    }
};

template <size_t TreeBits, unsigned Rollout>
class ClassifyTreeCalcUnrollInterleave : public ClassifyTreeCalcSimple<TreeBits>
{
public:
    static const size_t treebits = TreeBits;
    static const size_t numsplitters = (1 << treebits) - 1;

    using Super = ClassifyTreeCalcSimple<TreeBits>;
    using Super::splitter_tree;

    __attribute__ ((optimize("unroll-all-loops")))
    void find_bkt_unroll_one(
        unsigned int i[Rollout], const key_type key[Rollout])
    {
        for (unsigned u = 0; u < Rollout; ++u)
        {
#if 0
            // in gcc-4.6.3 this produces a SETA, LEA sequence
            i[u] = 2 * i[u] + (key[u] <= splitter_tree[i[u]] ? 0 : 1);
#else
            // in gcc-4.6.3 this produces two LEA and a CMOV sequence, which is
            // slightly faster
            if (key[u] <= splitter_tree[i[u]])
                i[u] = 2 * i[u] + 0;
            else    // (key > splitter_tree[i[u]])
                i[u] = 2 * i[u] + 1;
#endif
        }
    }

    //! search in splitter tree for bucket number, unrolled for Rollout keys at
    //! once.
    __attribute__ ((optimize("unroll-all-loops")))
    void find_bkt_unroll(const key_type key[Rollout], uint16_t obkt[Rollout])
    {
        // binary tree traversal without left branch

        unsigned int i[Rollout];
        std::fill(i + 0, i + Rollout, 1);

        switch (treebits)
        {
        default:
            abort();
        case 16:
            find_bkt_unroll_one(i, key);
        case 15:
            find_bkt_unroll_one(i, key);
        case 14:
            find_bkt_unroll_one(i, key);
        case 13:
            find_bkt_unroll_one(i, key);
        case 12:
            find_bkt_unroll_one(i, key);
        case 11:
            find_bkt_unroll_one(i, key);
        case 10:
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);

            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
            find_bkt_unroll_one(i, key);
        }

        for (unsigned u = 0; u < Rollout; ++u)
            i[u] -= numsplitters + 1;

        for (unsigned u = 0; u < Rollout; ++u) {
            // < bucket
            obkt[u] = i[u] * 2;
        }

        for (unsigned u = 0; u < Rollout; ++u) {
            // equal bucket
            if (i[u] < numsplitters && get_splitter(i[u]) == key[u])
                obkt[u] += 1;
        }
    }

    //! classify all strings in area by walking tree and saving bucket id
    __attribute__ ((optimize("unroll-all-loops")))
    void classify(string* strB, string* strE, uint16_t* bktout, size_t depth)
    {
        for (string* str = strB; str != strE; )
        {
            if (str + Rollout < strE)
            {
                key_type key[Rollout];
                for (unsigned u = 0; u < Rollout; ++u)
                    key[u] = get_char<key_type>(str[u], depth);

                find_bkt_unroll(key, bktout);

                str += Rollout;
                bktout += Rollout;
            }
            else
            {
                key_type key = get_char<key_type>(*str++, depth);
                *bktout++ = ClassifyTreeCalcSimple<TreeBits>::find_bkt(key);
            }
        }
    }
};

} // namespace bingmann_sample_sort

#endif // !PSS_SRC_SEQUENTIAL_BINGMANN_SAMPLE_SORTBTCT_HEADER

/******************************************************************************/
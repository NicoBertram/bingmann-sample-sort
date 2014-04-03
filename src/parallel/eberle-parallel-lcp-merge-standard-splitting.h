/******************************************************************************
 * src/parallel/eberle-parallel-lcp-merge.h
 *
 * Parallel LCP aware merge implementation.
 *
 ******************************************************************************
 * Copyright (C) 2014 Andreas Eberle <email@andreas-eberle.com>
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

#ifndef EBERLE_PARALLEL_LCP_MERGE_STANDARD_SPLITTING_H_
#define EBERLE_PARALLEL_LCP_MERGE_STANDARD_SPLITTING_H_

#include <limits>
#include <utility>

#include "eberle-parallel-lcp-merge.h"

#include "../sequential/eberle-mergesort-lcp.h"
#include "../tools/eberle-lcp-losertree.h"

#include "../tools/jobqueue.h"
#include "../tools/stringtools.h"

#include "../tools/debug.h"
#undef DBGX
#define DBGX DBGX_OMP

namespace eberle_parallel_lcp_merge
{
using std::numeric_limits;

using namespace eberle_lcp_utils;

using namespace jobqueue;
using namespace stringtools;

//due to ambigous symbol
using stringtools::string;

//debugging constants


//constants

static const bool debug = false;


//method definitions

static inline void
createJobsWithStandardSplitting(JobQueue &jobQueue, const LcpCacheStringPtr* inputStreams,
        unsigned numInputs, string* output, size_t numberOfElements);


//structs defining the jobs
template <unsigned K>
struct MergeJobStandardSplitting : public Job
{
    LcpStringLoserTree<K> loserTree;

    string* output;
    size_t length;

    MergeJobStandardSplitting(const LcpCacheStringPtr* inputs, unsigned numInputs, string* output, size_t length)
        :  loserTree(inputs, numInputs), output(output), length(length)
    {
        DBG(debug, "MergeJobStandardSplitting<" << K << "> (output: " << (output - g_outputBase) << ", length: " << length << ")");
    }

    /*
     * returns true if all elements have been written to output
     * false if the merge has been stopped to free work.
     */
    inline bool
    mergeToOutput(JobQueue& jobQueue)
    {
        for (size_t lastLength = length; length >= MERGE_BULK_SIZE; length -= MERGE_BULK_SIZE, output += MERGE_BULK_SIZE)
        {
            if (g_lengthOfLongestJob == lastLength)
                g_lengthOfLongestJob = length;

            if (g_lengthOfLongestJob < length)
                g_lengthOfLongestJob = length; // else if to prevent work sharing when we just increased g_lengthOfLongestJob
            else if (USE_WORK_SHARING &&
                     jobQueue.has_idle() &&
                     length > SHARE_WORK_THRESHOLD &&
                     g_lengthOfLongestJob == length)
                return false;

            loserTree.writeElementsToStream(output, MERGE_BULK_SIZE);
            lastLength = length;
        }

        loserTree.writeElementsToStream(output, length);

        return true;
    }

    virtual bool
    run(JobQueue& jobQueue)
    {
        loserTree.initTree(0);

        // merge

        if (!mergeToOutput(jobQueue))
        {
            // share work by splitting remaining streams

            createJobsWithStandardSplitting(jobQueue, loserTree.getRemaining(), K, output, length);

            if (g_lengthOfLongestJob == length)
                g_lengthOfLongestJob = 0;
        }

        return true;
    }
};

struct InitialStandardSplitJob : public Job
{
    const LcpCacheStringPtr* input;
    unsigned numInputs;
    string* output;
    size_t length;

    InitialStandardSplitJob(const LcpCacheStringPtr* input, unsigned numInputs, string* output, size_t length)
        : input(input), numInputs(numInputs), output(output), length(length)
    {
        g_lengthOfLongestJob = length; // prevents that the first MergeJob immediately starts splitting itself

        g_outputBase = output;
    }

    virtual bool
    run(JobQueue& jobQueue)
    {
        createJobsWithStandardSplitting(jobQueue, input, numInputs, output, length);

        g_lengthOfLongestJob = 0;
        return true;
    }
};

static inline void
enqueueJob(JobQueue &jobQueue, const LcpCacheStringPtr* inputs, unsigned numInputs, string* output, size_t jobLength)
{
    if(numInputs <= 2)
        jobQueue.enqueue(new MergeJobStandardSplitting<2>(inputs, numInputs, output, jobLength));

    else if(numInputs <= 4)
        jobQueue.enqueue(new MergeJobStandardSplitting<4>(inputs, numInputs, output, jobLength));

    else if(numInputs <= 8)
        jobQueue.enqueue(new MergeJobStandardSplitting<8>(inputs, numInputs, output, jobLength));

    else if(numInputs <= 16)
        jobQueue.enqueue(new MergeJobStandardSplitting<16>(inputs, numInputs, output, jobLength));

    else if(numInputs <= 32)
        jobQueue.enqueue(new MergeJobStandardSplitting<32>(inputs, numInputs, output, jobLength));

    else if(numInputs <= 64)
        jobQueue.enqueue(new MergeJobStandardSplitting<64>(inputs, numInputs, output, jobLength));

    else
    {
        DBG(1, "Can't create job with that many streams. Add more cases.");
        abort();
    }
}

static inline void
createJobsWithStandardSplitting(JobQueue &jobQueue, const LcpCacheStringPtr* inputStreams, unsigned numInputs, string* output, size_t numberOfElements)
{
    DBG(1, "CREATING JOBS for numberOfElements: " << numberOfElements);

    const unsigned numSplittersPerStream = 20;
    const unsigned numSplitters = numSplittersPerStream * numInputs;

    string splitters[numSplitters];
    LcpCacheStringPtr streams[numSplitters];

    for(unsigned i = 0; i < numInputs; i++)
    {
        streams[i] = inputStreams[i];
        if(!streams[i].empty())
        {
            const unsigned offset = i * numSplittersPerStream;
            size_t stepWidth = streams[i].size / (numSplittersPerStream + 1);

            for(unsigned n = 0; n < numSplittersPerStream; n++)
            {
                splitters[offset + n] = streams[i].strings[(n + 1) * stepWidth];
            }
        }
        else
        {
            const unsigned offset = i * numSplittersPerStream;

            for(unsigned n = 0; n < numSplittersPerStream; n++)
            {
                splitters[offset + n] = (unsigned char*)"";
            }
        }
    }

    eberle_mergesort_lcp::eberle_lcp_mergesort(splitters, numSplitters);

    for(unsigned job = 0; job < numSplitters; job++)
    {
        string splitterString = splitters[job];

        if(splitterString[0] == '\0') // skip empty strings used as default value
            continue;


DBG(debug, "Job: " << job << ", splitterString: " << splitterString);

        LcpCacheStringPtr jobStreams[numInputs];
        unsigned nonEmptyCtr = 0;
        unsigned jobLength = 0;

        for(unsigned i = 0; i < numInputs; i++)
        {
            LcpCacheStringPtr stream = streams[i];

            if(!stream.empty())
            {
                size_t idx;
                size_t l = 0;
                size_t r = stream.size - 1;

                if(scmp(splitterString, stream.strings[0]) <= 0)
                {
                    idx = 0;
                }
                else if(scmp(splitterString, stream.strings[r]) > 0)
                {
                    idx = stream.size;
                }
                else
                {
                    while ((r - l) > 1)
                    {
                        size_t m = (l + r) / 2;

                        if(scmp(splitterString, stream.strings[m]) <= 0)
                            r = m;
                        else
                            l = m;
                    }
                    idx = r;
                }


                jobStreams[nonEmptyCtr] = stream.sub(0, idx);
                nonEmptyCtr++;
                jobLength += idx;

                streams[i] = stream.sub(idx, stream.size - idx);

DBG(debug, "Found at [" << idx << "]: ");
            }
        }

        enqueueJob(jobQueue, jobStreams, nonEmptyCtr, output, jobLength);
        output += jobLength;
    }

    // create job for the last part with elements bigger than the biggest splitter
    LcpCacheStringPtr jobStreams[numInputs];
    unsigned nonEmptyCtr = 0;
    unsigned jobLength = 0;

    for(unsigned i = 0; i < numInputs; i++)
    {
        if(!streams[i].empty())
        {
            jobStreams[nonEmptyCtr] = streams[i];
            nonEmptyCtr++;
            jobLength += streams[i].size;
        }
    }
    enqueueJob(jobQueue, jobStreams, nonEmptyCtr, output, jobLength);
}



static inline void
parallelLcpMergeStandardSplitting(const LcpCacheStringPtr* input, unsigned numInputs, string* output, size_t length)
{
	ClockTimer timer;
	timer.start();

    g_outputBase = output;

    JobQueue jobQueue;
    DBG(debug_merge_start_message, "doing parallel lcp merge for " << numInputs << " input streams using " << omp_get_max_threads() << " threads with standard splitting");
    jobQueue.enqueue(new InitialStandardSplitJob(input, numInputs, output, length));
    jobQueue.numaLoop(-1, omp_get_max_threads());
	
	g_stats >> "parallelTopLevelMergeStandardSplitting" << timer.elapsed();
}


} // namespace eberle_parallel_lcp_merge

#endif // EBERLE_PARALLEL_LCP_MERGE_STANDARD_SPLITTING_H_
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chainparams.h"
#include "core.h"
#include "main.h"
#include "uint256.h"

#include "bignum.h" //Necessary for KGW

static CBigNum bnProofOfWorkLimit(~uint256(0) >> 20); //Necessary for KGW

// static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

// Thanks: Balthazar for suggesting the following fix
// https://bitcointalk.org/index.php?topic=182430.msg1904506#msg1904506
static const int64_t nReTargetHistoryFact = 4; // look at 4 times the retarget
                                             // interval into the block history

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const CBlockHeader *pblock, uint64_t TargetBlocksSpacingSeconds, uint64_t PastBlocksMin, uint64_t PastBlocksMax) {
    const CBlockIndex  *BlockLastSolved				= pindexLast;
    const CBlockIndex  *BlockReading				= pindexLast;
    const CBlockHeader *BlockCreating				= pblock;
                        BlockCreating				= BlockCreating;
    uint64_t				PastBlocksMass				= 0;
    int64_t				PastRateActualSeconds		= 0;
    int64_t				PastRateTargetSeconds		= 0;
    double				PastRateAdjustmentRatio		= double(1);
    CBigNum				PastDifficultyAverage;
    CBigNum				PastDifficultyAveragePrev;
    double				EventHorizonDeviation;
    double				EventHorizonDeviationFast;
    double				EventHorizonDeviationSlow;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) { return Params().bnProofOfWorkLimit.GetCompact(); }

	 int64_t LatestBlockTime = BlockLastSolved->GetBlockTime();

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        PastBlocksMass++;

        if (i == 1)	{ PastDifficultyAverage.SetCompact(BlockReading->nBits); }
        else		{ PastDifficultyAverage = ((CBigNum().SetCompact(BlockReading->nBits) - PastDifficultyAveragePrev) / i) + PastDifficultyAveragePrev; }
        PastDifficultyAveragePrev = PastDifficultyAverage;

		  if (LatestBlockTime < BlockReading->GetBlockTime()) {
            if (BlockReading->nHeight > 29000) {
                LatestBlockTime = BlockReading->GetBlockTime();
            }
        }

		  PastRateActualSeconds = LatestBlockTime - BlockReading->GetBlockTime(); //KGW patch
        PastRateTargetSeconds			= TargetBlocksSpacingSeconds * PastBlocksMass;
        PastRateAdjustmentRatio			= double(1);
        
 		  if (BlockReading->nHeight > 29000) {
            if (PastRateActualSeconds < 1) {
                PastRateActualSeconds = 1;
            }
        } else {
            if (PastRateActualSeconds < 0) {
                PastRateActualSeconds = 0;
            }
        }       
        
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        PastRateAdjustmentRatio			= double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        }
        EventHorizonDeviation			= 1 + (0.7084 * pow((double(PastBlocksMass)/double(144)), -1.228));
        EventHorizonDeviationFast		= EventHorizonDeviation;
        EventHorizonDeviationSlow		= 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
            if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast)) { assert(BlockReading); break; }
        }
        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    CBigNum bnNew(PastDifficultyAverage);
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        bnNew *= PastRateActualSeconds;
        bnNew /= PastRateTargetSeconds;
    }
    if (bnNew > bnProofOfWorkLimit) { bnNew = bnProofOfWorkLimit; }

    /// debug print
    LogPrintf("Difficulty Retarget - Kimoto Gravity Well\n");
    LogPrintf("PastRateAdjustmentRatio = %g\n", PastRateAdjustmentRatio);
//    LogPrintf("Before: %08x  %s\n", BlockLastSolved->nBits, CBigNum().SetCompact(BlockLastSolved->nBits).getuint256().ToString());
//    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString());

    return bnNew.GetCompact();
}

unsigned int static GetNextWorkRequired_V2(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    static const int64_t	BlocksTargetSpacing			= 2.5 * 60; // 2.5 minute
    unsigned int		TimeDaySeconds				= 60 * 60 * 24;
    int64_t				PastSecondsMin				= TimeDaySeconds * 0.25;
    int64_t				PastSecondsMax				= TimeDaySeconds * 7;
    uint64_t				PastBlocksMin				= PastSecondsMin / BlocksTargetSpacing;
    uint64_t				PastBlocksMax				= PastSecondsMax / BlocksTargetSpacing;

    return KimotoGravityWell(pindexLast, pblock, BlocksTargetSpacing, PastBlocksMin, PastBlocksMax);
}


unsigned int static GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % Params().Interval() != 0)
    {
        if (Params().AllowMinDifficultyBlocks())
        {
        	   // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + Params().TargetSpacing()*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % Params().Interval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Lycancoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = Params().Interval()-1;
    if ((pindexLast->nHeight+1) != Params().Interval())
       blockstogoback = Params().Interval();
    if (pindexLast->nHeight > COINFIX1_BLOCK) {
       blockstogoback = nReTargetHistoryFact * Params().Interval();
    }

    // Go back by what we want to be nReTargetHistoryFact*nInterval blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = 0;
    if (pindexLast->nHeight > COINFIX1_BLOCK)
        // obtain average actual timespan
        nActualTimespan = (pindexLast->GetBlockTime() - pindexFirst->GetBlockTime())/nReTargetHistoryFact;
    else
        nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d before bounds\n", nActualTimespan);
    if (nActualTimespan < Params().TargetTimespan()/4)
        nActualTimespan = Params().TargetTimespan()/4;
    if (nActualTimespan > Params().TargetTimespan()*4)
        nActualTimespan = Params().TargetTimespan()*4;

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= Params().TargetTimespan();

    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("nTargetTimespan = %d   nActualTimespan = %d\n", Params().TargetTimespan(), nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    int DiffMode = 1;
    if (Params().AllowMinDifficultyBlocks()) {
        if (pindexLast->nHeight+1 >= FIX_RETARGET_HEIGHT){ DiffMode=2; }
    }
    else {
        if (pindexLast->nHeight+1 >= FIX_RETARGET_HEIGHT){ DiffMode=2; }
    }

    if		(DiffMode == 1) { return GetNextWorkRequired_V1(pindexLast, pblock); }
    else if	(DiffMode == 2) { return GetNextWorkRequired_V2(pindexLast, pblock); }
    return GetNextWorkRequired_V2(pindexLast, pblock);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;
    
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
//    if (hash > bnTarget)
//        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int ComputeMinWork(unsigned int nBase, int64_t nTime)
{
    const uint256 &bnLimit = Params().ProofOfWorkLimit();
    // Testnet has min-difficulty blocks
    // after nTargetSpacing*2 time between blocks:
    if (Params().AllowMinDifficultyBlocks() && nTime > Params().TargetSpacing()*2)
        return bnLimit.GetCompact();

    uint256 bnResult;
    bnResult.SetCompact(nBase);
    while (nTime > 0 && bnResult < bnLimit)
    {
        // Maximum 400% adjustment...
        bnResult *= 4;
        // ... in best-case exactly 4-times-normal target time
        nTime -= Params().TargetTimespan()*4;
    }
    if (bnResult > bnLimit)
        bnResult = bnLimit;
    return bnResult.GetCompact();
}
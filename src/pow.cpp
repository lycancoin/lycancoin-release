// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "main.h" //Necessary for KGW - Move later
#include "uint256.h"
#include "util.h"

#include "bignum.h" //Necessary for KGW

//static CBigNum bnProofOfWorkLimit(~uint256(0) >> 20); //Necessary for KGW
static const CBigNum bnProofOfWorkLimit(uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

// static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

// Thanks: Balthazar for suggesting the following fix
// https://bitcointalk.org/index.php?topic=182430.msg1904506#msg1904506
static const int64_t nReTargetHistoryFact = 4; // look at 4 times the retarget
                                             // interval into the block history

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const CBlockHeader *pblock, uint64_t TargetBlocksSpacingSeconds, uint64_t PastBlocksMin, uint64_t PastBlocksMax, const Consensus::Params& params)
{
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

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) { return UintToArith256(params.powLimit).GetCompact(); }

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

//Initial DigiShield Implementation compatible with code

unsigned int static DigiShield(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{

 unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

 int blockstogoback = 0;

 //set default to pre-v2.0 values
 int64_t retargetTimespan = 150; //Make sure we retarget every block at 2.5 minutes
 int64_t retargetSpacing = 150; //2.5 minutes
 int64_t retargetInterval = retargetTimespan / retargetSpacing;
 // Genesis block
 if (pindexLast == NULL) return nProofOfWorkLimit;

 // Only change once per interval
 if ((pindexLast->nHeight+1) % retargetInterval != 0)
 {
 			return pindexLast->nBits;
 }

 // DigiByte: This fixes an issue where a 51% attack can change difficulty at will.
 // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
 blockstogoback = retargetInterval-1;
 if ((pindexLast->nHeight+1) != retargetInterval) 
 			blockstogoback = retargetInterval;

 // Go back by what we want to be 14 days worth of blocks
 const CBlockIndex* pindexFirst = pindexLast;
 for (int i = 0; pindexFirst && i < blockstogoback; i++)
 			pindexFirst = pindexFirst->pprev;
 //int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
 //assert(nHeightFirst >= 0);
 //const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
 assert(pindexFirst);

 // Limit adjustment step
 int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
 LogPrintf("  nActualTimespan = %d before bounds\n", nActualTimespan);

 if (nActualTimespan < (retargetTimespan - (retargetTimespan/4)) ) nActualTimespan = (retargetTimespan - (retargetTimespan/4));
 if (nActualTimespan > (retargetTimespan + (retargetTimespan/2)) ) nActualTimespan = (retargetTimespan + (retargetTimespan/2));

 // Retarget
   const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
	arith_uint256 bnNew;
	arith_uint256 bnBefore;
	bnNew.SetCompact(pindexLast->nBits);
	bnBefore=bnNew;
	bnNew *= nActualTimespan;
	bnNew /= retargetTimespan;
	
   if (bnNew > bnPowLimit)
       bnNew = bnPowLimit;	

	// debug print
	LogPrintf("DigiShield RETARGET \n");
	LogPrintf("nTargetTimespan = %d    nActualTimespan = %d\n", retargetTimespan, nActualTimespan);
	LogPrintf("Before: %08x  %s\n", pindexLast->nBits, ArithToUint256(bnBefore).ToString());
	LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), ArithToUint256(bnNew).ToString());

	return bnNew.GetCompact();
}

//End DigiShield Core Code


unsigned int static GetNextWorkRequired_V2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    static const int64_t	BlocksTargetSpacing			= 2.5 * 60; // 2.5 minute
    unsigned int		TimeDaySeconds				= 60 * 60 * 24;
    int64_t				PastSecondsMin				= TimeDaySeconds * 0.25;
    int64_t				PastSecondsMax				= TimeDaySeconds * 7;
    uint64_t				PastBlocksMin				= PastSecondsMin / BlocksTargetSpacing;
    uint64_t				PastBlocksMax				= PastSecondsMax / BlocksTargetSpacing;

    return KimotoGravityWell(pindexLast, pblock, BlocksTargetSpacing, PastBlocksMin, PastBlocksMax, params);
}


unsigned int static GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
        	   // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Lycancoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = params.DifficultyAdjustmentInterval()-1;
    if ((pindexLast->nHeight+1) != params.DifficultyAdjustmentInterval())
       blockstogoback = params.DifficultyAdjustmentInterval();
    if (pindexLast->nHeight > COINFIX1_BLOCK) {
       blockstogoback = nReTargetHistoryFact * params.DifficultyAdjustmentInterval();
    }

    // Go back by what we want to be nReTargetHistoryFact*nInterval blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (pindexLast->nHeight > COINFIX1_BLOCK)
        // obtain average actual timespan
        nActualTimespan = (pindexLast->GetBlockTime() - nFirstBlockTime)/nReTargetHistoryFact;
    else
        nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    LogPrintf("  nActualTimespan = %d before bounds\n", nActualTimespan);
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("nTargetTimespan = %d   nActualTimespan = %d\n", params.nPowTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    int DiffMode = 1;
    if (params.fPowAllowMinDifficultyBlocks) {
        if (pindexLast->nHeight+1 >= FIX_RETARGET_HEIGHT){ DiffMode=2; }
    }
    else 
    {
        if (pindexLast->nHeight+1 >= FIX_RETARGET_HEIGHT && pindexLast->nHeight+1 < 1569800) { DiffMode=2; }
        else if (pindexLast->nHeight+1 >= 1569800) { DiffMode = 3; }
    }

    if		(DiffMode == 1) { return GetNextWorkRequired_V1(pindexLast, pblock, params); }
    else if	(DiffMode == 2) { return GetNextWorkRequired_V2(pindexLast, pblock, params); }
    else if (DiffMode == 3) { return DigiShield(pindexLast, pblock, params); }
    return DigiShield(pindexLast, pblock, params);

}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;
    
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
//   if (UintToArith256(hash) > bnTarget)
//        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
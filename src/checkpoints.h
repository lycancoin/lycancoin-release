// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKPOINT_H
#define BITCOIN_CHECKPOINT_H

#include "uint256.h"

#include <map>

class CBlockIndex;

/** Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{
typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    const MapCheckpoints *mapCheckpoints;
    int64_t nTimeLastCheckpoint;
    int64_t nTransactionsLastCheckpoint;
    double fTransactionsPerDay;
};

// Returns true if block passes checkpoint checks
bool CheckBlock(int nHeight, const uint256& hash);

// Return conservative estimate of total number of blocks, 0 if unknown
int GetTotalBlocksEstimate();

// Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex* GetLastCheckpoint();

double GuessVerificationProgress(CBlockIndex* pindex, bool fSigchecks = true);

extern bool fEnabled;

} //namespace Checkpoints

#endif // BITCOIN_CHECKPOINT_H
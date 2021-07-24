
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <stdint.h>

class CBlockIndex;
class CBlockHeader;
class uint256;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);
/** Check the work is more than the minimum a received block needs, without knowing its direct parent */
bool CheckMinWork(unsigned int nBits, unsigned int nBase, int64_t deltaTime);

void UpdateTime(CBlockHeader* block, const CBlockIndex* pindexPrev);

uint256 GetProofIncrement(unsigned int nBits);

#endif